#include <csignal>
#include <filesystem>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include "AppenderDB.h"
#include "AuthSocketMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "IoContext.h"
#include "Log.h"
#include "OpenSSLCrypto.h"
#include "ProcessPriority.h"
#include "RealmList.h"
#include "SharedDefines.h"
#include "SteadyTimer.h"
#include "Util.h"

#ifndef NCORE_REALM_CONFIG
#define NCORE_REALM_CONFIG "authserver.conf"
#endif

using boost::asio::ip::tcp;
using namespace boost::program_options;
namespace fs = std::filesystem;

bool StartDB();
void StopDB();
void SignalHandler(const std::weak_ptr<Acore::Asio::IoContext>& ioContextRef, boost::system::error_code const& error, int signalNumber);
void KeepDatabaseAliveHandler(std::weak_ptr<boost::asio::steady_timer> dbPingTimerRef, int32 dbPingInterval, boost::system::error_code const& error);
bool GetConsoleArguments(int argc, char** argv, fs::path& configFile);

/// Launch the auth server
int main(const int argc, char** argv)
{
    Acore::Impl::CurrentServerProcessHolder::_type = SERVER_PROCESS_AUTHSERVER;
    signal(SIGABRT, &Acore::AbortHandler);

    // Command line parsing
    auto configFile = fs::path(sConfigMgr->GetConfigPath()) / NCORE_REALM_CONFIG;
    if (!GetConsoleArguments(argc, argv, configFile))
        return 0;

    // Add file and args in config
    sConfigMgr->Configure(configFile.generic_string());

    if (!sConfigMgr->LoadAppConfigs())
        return 1;

    // Init logging
    sLog->RegisterAppender<AppenderDB>();
    sLog->Initialize();

    OpenSSLCrypto::threadsSetup();

    std::shared_ptr<void> opensslHandle(nullptr, [](void*) { OpenSSLCrypto::threadsCleanup(); });

    // Initialize the database connection
    if (!StartDB())
        return 1;

    std::shared_ptr<void> dbHandle(nullptr, [](void*) { StopDB(); });

    const auto ioContext = std::make_shared<Acore::Asio::IoContext>();

    // Get the list of realms for the server
    sRealmList->Initialize(*ioContext, sConfigMgr->GetOption<int32>("RealmsStateUpdateDelay", 20));

    std::shared_ptr<void> sRealmListHandle(nullptr, [](void*) { sRealmList->Close(); });

    if (!sRealmList->GetRealm())
    {
        LOG_ERROR("server.authserver", "No valid realms specified.");
        return 1;
    }

    // Stop auth server if dry run
    if (sConfigMgr->isDryRun())
    {
        LOG_INFO("server.authserver", "Dry run completed, terminating.");
        return 0;
    }

    // Start the listening port (acceptor) for auth connections
    int32 port = sConfigMgr->GetOption<int32>("RealmServerPort", 3724);
    if (port < 0 || port > 0xFFFF)
    {
        LOG_ERROR("server.authserver", "Specified port out of allowed range (1-65535)");
        return 1;
    }

    std::string bindIp = sConfigMgr->GetOption<std::string>("BindIP", "0.0.0.0");

    if (!sAuthSocketMgr.StartNetwork(*ioContext, bindIp, port))
    {
        LOG_ERROR("server.authserver", "Failed to initialize network");
        return 1;
    }

    std::shared_ptr<void> sAuthSocketMgrHandle(nullptr, [](void*) { sAuthSocketMgr.StopNetwork(); });

    // Set signal handlers
    boost::asio::signal_set signals(*ioContext, SIGINT, SIGTERM);
    signals.async_wait(std::bind(&SignalHandler, std::weak_ptr(ioContext), std::placeholders::_1, std::placeholders::_2));

    // Set process priority according to configuration settings
    SetProcessPriority("server.authserver", sConfigMgr->GetOption<bool>(CONFIG_HIGH_PRIORITY, false));

    // Enabled a timed callback for handling the database keep alive ping
    int32 dbPingInterval = sConfigMgr->GetOption<int32>("MaxPingTime", 30);
    std::shared_ptr<boost::asio::steady_timer> dbPingTimer = std::make_shared<boost::asio::steady_timer>(*ioContext);

    dbPingTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(dbPingInterval * MINUTE));
    dbPingTimer->async_wait(std::bind(&KeepDatabaseAliveHandler, std::weak_ptr<boost::asio::steady_timer>(dbPingTimer), dbPingInterval, std::placeholders::_1));

    // Start the io service worker loop
    ioContext->run();
    dbPingTimer->cancel();

    LOG_INFO("server.authserver", "Halting process...");

    signals.cancel();

    return 0;
}

/// Initialize connection to the database
bool StartDB()
{
    // Load databases
    // NOTE: While authserver is single-threaded you should keep synch_threads == 1.
    // Increasing it is just silly since only 1 will be used ever.
    DatabaseLoader loader("server.authserver");
    loader.AddDatabase(LoginDatabase, "Login");

    if (!loader.Load())
        return false;

    LOG_INFO("server.authserver", "Started auth database connection pool.");
    return true;
}

/// Close the connection to the database
void StopDB()
{
    LoginDatabase.Close();
}

void SignalHandler(const std::weak_ptr<Acore::Asio::IoContext>& ioContextRef, boost::system::error_code const& error, int /*signalNumber*/)
{
    if (!error)
        if (const std::shared_ptr<Acore::Asio::IoContext> ioContext = ioContextRef.lock())
            ioContext->stop();
}

void KeepDatabaseAliveHandler(std::weak_ptr<boost::asio::steady_timer> dbPingTimerRef, int32 dbPingInterval, boost::system::error_code const& error)
{
    if (!error)
    {
        if (std::shared_ptr<boost::asio::steady_timer> dbPingTimer = dbPingTimerRef.lock())
        {
            LOG_DEBUG("sql.driver", "Ping PostgreSQL to keep connection alive");
            LoginDatabase.KeepAlive();

            dbPingTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(dbPingInterval));
            dbPingTimer->async_wait(std::bind(&KeepDatabaseAliveHandler, dbPingTimerRef, dbPingInterval, std::placeholders::_1));
        }
    }
}

bool GetConsoleArguments(const int argc, char** argv, fs::path& configFile)
{
    const fs::path defaultConfigFile(configFile);
    options_description all("Allowed options");
    all.add_options()
        ("help,h", "print usage message")
        ("dry-run,d", "Dry run")
        ("config,c", value<fs::path>(&configFile)->default_value(defaultConfigFile), "use <arg> as configuration file");

    variables_map variablesMap;

    try
    {
        store(command_line_parser(argc, argv).options(all).allow_unregistered().run(), variablesMap);
        notify(variablesMap);
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << "\n";
        return false;
    }

    if (variablesMap.contains("help"))
    {
        std::cout << all << "\n";
        return false;
    }
    if (variablesMap.contains("dry-run"))
        sConfigMgr->setDryRun(true);

    return true;
}
