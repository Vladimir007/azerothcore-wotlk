#include <csignal>
#include <filesystem>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>

#include "AppenderDB.h"
#include "AsyncAcceptor.h"
#include "BattlegroundMgr.h"
#include "BigNumber.h"
#include "Common.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "IoContext.h"
#include "MapMgr.h"
#include "ModuleMgr.h"
#include "OpenSSLCrypto.h"
#include "OutdoorPvPMgr.h"
#include "ProcessPriority.h"
#include "RealmList.h"
#include "Resolver.h"
#include "ScriptLoader.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SteadyTimer.h"
#include "World.h"
#include "WorldSessionMgr.h"
#include "WorldSocketMgr.h"

#ifndef NCORE_CORE_CONFIG
#define NCORE_CORE_CONFIG "worldserver.conf"
#endif

using namespace boost::program_options;
namespace fs = std::filesystem;

class FreezeDetector
{
public:
    FreezeDetector(Acore::Asio::IoContext& ioContext, const uint32 maxCoreStuckTime):
        _timer(ioContext), _worldLoopCounter(0), _lastChangeMsTime(getMSTime()), _maxCoreStuckTimeInMs(maxCoreStuckTime) {}

    static void Start(std::shared_ptr<FreezeDetector> const& freezeDetector)
    {
        freezeDetector->_timer.expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(5));
        freezeDetector->_timer.async_wait(std::bind(&FreezeDetector::Handler, std::weak_ptr(freezeDetector), std::placeholders::_1));
    }

    static void Handler(std::weak_ptr<FreezeDetector> freezeDetectorRef, boost::system::error_code const& error);

private:
    boost::asio::steady_timer _timer;
    uint32 _worldLoopCounter;
    uint32 _lastChangeMsTime;
    uint32 _maxCoreStuckTimeInMs;
};

void SignalHandler(boost::system::error_code const& error, int signalNumber);
void ClearOnlineAccounts();
bool StartDB();
void StopDB();
bool LoadRealmInfo(Acore::Asio::IoContext& ioContext);
void WorldUpdateLoop();
bool GetConsoleArguments(int argc, char** argv, fs::path& configFile);

int main(const int argc, char** argv)
{
    Acore::Impl::CurrentServerProcessHolder::_type = SERVER_PROCESS_WORLDSERVER;
    signal(SIGABRT, &Acore::AbortHandler);

    // Command line parsing
    auto configFile = fs::path(sConfigMgr->GetConfigPath()) / NCORE_CORE_CONFIG;

    if (!GetConsoleArguments(argc, argv, configFile))
        return 0;

    // Add file and args in config
    sConfigMgr->Configure(configFile.generic_string());

    if (!sConfigMgr->LoadAppConfigs())
        return 1;

    auto ioContext = std::make_shared<Acore::Asio::IoContext>();

    // Init all logs
    sLog->RegisterAppender<AppenderDB>();
    sLog->Initialize();

    OpenSSLCrypto::threadsSetup();

    std::shared_ptr<void> opensslHandle(nullptr, [](void*) { OpenSSLCrypto::threadsCleanup(); });

    // Seed the OpenSSL's PRNG here.
    // That way it won't auto-seed when calling BigNumber::SetRand and slow down the first world login
    BigNumber seed;
    seed.SetRand(16 * 8);

    // Set signal handlers (this must be done before starting IoContext threads, because otherwise they would unblock and exit)
    boost::asio::signal_set signals(*ioContext, SIGINT, SIGTERM);

    signals.async_wait(SignalHandler);

    // Start the Boost based thread pool
    int numThreads = sConfigMgr->GetOption<int32>("ThreadPool", 2);
    if (numThreads < 1)
        numThreads = 1;

    std::shared_ptr<std::vector<std::thread>> threadPool(new std::vector<std::thread>(), [ioContext](std::vector<std::thread>* del)
    {
        ioContext->stop();
        for (std::thread& thr : *del)
            thr.join();
        delete del;
    });

    for (int i = 0; i < numThreads; ++i)
        threadPool->push_back(std::thread([ioContext] { ioContext->run(); }));

    // Set process priority according to configuration settings
    SetProcessPriority("server.worldserver", sConfigMgr->GetOption<bool>(CONFIG_HIGH_PRIORITY, true));

    sScriptMgr->SetScriptLoader(AddScripts);
    std::shared_ptr<void> sScriptMgrHandle(nullptr, [](void*) { sScriptMgr->Unload(); });
    LOG_INFO("server.loading", "Initializing Scripts...");
    sScriptMgr->Initialize();

    // Start the databases
    if (!StartDB())
        return 1;
    std::shared_ptr<void> dbHandle(nullptr, [](void*) { StopDB(); });

    if (!LoadRealmInfo(*ioContext))
        return 1;

    // Set server offline (not connectable)
    LoginDatabase.DirectExecute("UPDATE realm SET flag=$1 WHERE id=$2", REALM_FLAG_OFFLINE, realm.ID);
    realm.Flags = REALM_FLAG_OFFLINE;

    Acore::Module::SetEnableModulesList("");

    ///- Initialize the World
    sWorld->SetInitialWorldSettings();

    std::shared_ptr<void> mapManagementHandle(nullptr, [](void*)
    {
        // unload battleground templates before different singletons destroyed
        sBattlegroundMgr->DeleteAllBattlegrounds();

        sOutdoorPvPMgr->Die();                     // unload it before MapMgr
        sMapMgr->UnloadAll();                      // unload all grids (including locked in memory)

        sScriptMgr->OnAfterUnloadAllMaps();
    });

    // Launch the worldserver listener socket
    uint16 worldPort = uint16(sWorld->getIntConfig(CONFIG_PORT_WORLD));
    std::string worldListener = sConfigMgr->GetOption<std::string>("BindIP", "0.0.0.0");

    if (!sWorldSocketMgr.StartNetwork(*ioContext, worldListener, worldPort))
    {
        LOG_ERROR("server.worldserver", "Failed to initialize network");
        World::StopNow(ERROR_EXIT_CODE);
        return 1;
    }

    std::shared_ptr<void> sWorldSocketMgrHandle(nullptr, [](void*)
    {
        sWorldSessionMgr->KickAll();         // save and kick all players
        sWorldSessionMgr->UpdateSessions(1); // real players unload required UpdateSessions call

        sWorldSocketMgr.StopNetwork();

        ///- Clean database before leaving
        ClearOnlineAccounts();
    });

    // Set server online (allow connecting now)
    LoginDatabase.DirectExecute("UPDATE realm SET flag=$1 WHERE id=$2", REALM_FLAG_NONE, realm.ID);
    realm.Flags = REALM_FLAG_NONE;

    // Start the freeze check callback cycle in 5 seconds (cycle itself is 1 sec)
    std::shared_ptr<FreezeDetector> freezeDetector;
    if (int32 coreStuckTime = sConfigMgr->GetOption<int32>("MaxCoreStuckTime", 60))
    {
        freezeDetector = std::make_shared<FreezeDetector>(*ioContext, coreStuckTime * 1000);
        FreezeDetector::Start(freezeDetector);
        LOG_INFO("server.worldserver", "Starting up anti-freeze thread ({} seconds max stuck time)...", coreStuckTime);
    }

    LOG_INFO("server.worldserver", "NordCore (worldserver-daemon) ready...");

    sScriptMgr->OnStartup();

    WorldUpdateLoop();

    // Shutdown starts here
    threadPool.reset();

    sScriptMgr->OnShutdown();

    // Set server offline
    LoginDatabase.DirectExecute("UPDATE realm SET flag=$1 WHERE id=$2", REALM_FLAG_OFFLINE, realm.ID);

    LOG_INFO("server.worldserver", "Halting process...");

    // 0 - normal shutdown
    // 1 - shutdown at error
    // 2 - restart command used, this code can be used by restarter for restart AzerothCore

    return World::GetExitCode();
}

/// Initialize connection to the databases
bool StartDB()
{
    // Load databases
    DatabaseLoader loader("server.worldserver");
    loader
        .AddDatabase(LoginDatabase, "Login")
        .AddDatabase(CharacterDatabase, "Character")
        .AddDatabase(WorldDatabase, "World");

    if (!loader.Load())
        return false;

    LOG_INFO("server.loading", "Loading World Information...");

    // Clean the database before starting
    ClearOnlineAccounts();

    sScriptMgr->OnAfterDatabasesLoaded();

    return true;
}

void StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();
}

/// Clear 'online' status for all accounts with characters in this realm
void ClearOnlineAccounts()
{
    // Reset online status for all accounts
    LoginDatabase.DirectExecute("UPDATE accounts SET online=FALSE");

    // Reset online status for all characters
    CharacterDatabase.DirectExecute("UPDATE characters SET online = 0 WHERE online <> 0");
}

void WorldUpdateLoop()
{
    const uint32 minUpdateDiff = static_cast<uint32>(sConfigMgr->GetOption<int32>("MinWorldUpdateTime", 1));
    uint32 realCurrTime = 0;
    uint32 realPrevTime = getMSTime();

    uint32 maxCoreStuckTime = static_cast<uint32>(sConfigMgr->GetOption<int32>("MaxCoreStuckTime", 60)) * 1000;
    uint32 halfMaxCoreStuckTime = maxCoreStuckTime / 2;
    if (!halfMaxCoreStuckTime)
        halfMaxCoreStuckTime = std::numeric_limits<uint32>::max();

    // While we have not World::m_stopEvent, update the world
    while (!World::IsStopped())
    {
        ++World::m_worldLoopCounter;
        realCurrTime = getMSTime();

        const uint32 diff = getMSTimeDiff(realPrevTime, realCurrTime);
        if (diff < minUpdateDiff)
        {
            uint32 sleepTime = minUpdateDiff - diff;
            if (sleepTime >= halfMaxCoreStuckTime)
                LOG_ERROR("server.worldserver", "WorldUpdateLoop() waiting for {} ms with MaxCoreStuckTime set to {} ms", sleepTime, maxCoreStuckTime);
            // sleep until enough time passes that we can update all timers
            std::this_thread::sleep_for(Milliseconds(sleepTime));
            continue;
        }

        sWorld->Update(diff);
        realPrevTime = realCurrTime;
    }
}

void SignalHandler(boost::system::error_code const& error, int /*signalNumber*/)
{
    if (!error)
        World::StopNow(SHUTDOWN_EXIT_CODE);
}

void FreezeDetector::Handler(std::weak_ptr<FreezeDetector> freezeDetectorRef, boost::system::error_code const& error)
{
    if (!error)
    {
        if (std::shared_ptr<FreezeDetector> freezeDetector = freezeDetectorRef.lock())
        {
            uint32 curtime = getMSTime();

            uint32 worldLoopCounter = World::m_worldLoopCounter;
            if (freezeDetector->_worldLoopCounter != worldLoopCounter)
            {
                freezeDetector->_lastChangeMsTime = curtime;
                freezeDetector->_worldLoopCounter = worldLoopCounter;
            }
            // possible freeze
            else
            {
                uint32 msTimeDiff = getMSTimeDiff(freezeDetector->_lastChangeMsTime, curtime);
                if (msTimeDiff > freezeDetector->_maxCoreStuckTimeInMs)
                {
                    LOG_ERROR("server.worldserver", "World Thread hangs for {} ms, forcing a crash!", msTimeDiff);
                    ABORT("World Thread hangs for {} ms, forcing a crash!", msTimeDiff);
                }
            }

            freezeDetector->_timer.expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(1));
            freezeDetector->_timer.async_wait(std::bind(&FreezeDetector::Handler, freezeDetectorRef, std::placeholders::_1));
        }
    }
}

bool LoadRealmInfo(Acore::Asio::IoContext& ioContext)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALM_INFO);
    const QueryResult result = LoginDatabase.Query(stmt);
    if (!result)
        return false;

    Acore::Asio::Resolver resolver(ioContext);

    Field* fields = result->Fetch();
    realm.ID = fields[0].Get<uint32>();
    realm.Name = fields[1].Get<std::string>();

    Optional<tcp::endpoint> externalAddress = resolver.Resolve(tcp::v4(), fields[2].Get<std::string>(), "");
    if (!externalAddress)
    {
        LOG_ERROR("server.worldserver", "Could not resolve address {}", fields[2].Get<std::string>());
        return false;
    }
    realm.ExternalAddress = std::make_unique<boost::asio::ip::address>(externalAddress->address());

    Optional<tcp::endpoint> localAddress = resolver.Resolve(tcp::v4(), fields[3].Get<std::string>(), "");
    if (!localAddress)
    {
        LOG_ERROR("server.worldserver", "Could not resolve address {}", fields[3].Get<std::string>());
        return false;
    }
    realm.LocalAddress = std::make_unique<boost::asio::ip::address>(localAddress->address());

    Optional<tcp::endpoint> localSubMask = resolver.Resolve(tcp::v4(), fields[4].Get<std::string>(), "");
    if (!localSubMask)
    {
        LOG_ERROR("server.worldserver", "Could not resolve address {}", fields[4].Get<std::string>());
        return false;
    }
    realm.LocalSubnetMask = std::make_unique<boost::asio::ip::address>(localSubMask->address());

    realm.Port = fields[5].Get<uint16>();
    realm.Flags = static_cast<RealmFlags>(fields[6].Get<uint8>());
    realm.Timezone = fields[7].Get<uint8>();
    return true;
}

bool GetConsoleArguments(const int argc, char** argv, fs::path& configFile)
{
    const fs::path defaultConfigFile(configFile);

    options_description all("Allowed options");
    all.add_options()
        ("help,h", "print usage message")
        ("dry-run,d", "Dry run")
        ("config,c", value<fs::path>(&configFile)->default_value(defaultConfigFile), "use <arg> as configuration file");

    variables_map vm;

    try
    {
        store(command_line_parser(argc, argv).options(all).allow_unregistered().run(), vm);
        notify(vm);
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << "\n";
        return false;
    }

    if (vm.contains("help"))
    {
        std::cout << all << "\n";
        return false;
    }

    if (vm.contains("dry-run"))
        sConfigMgr->setDryRun(true);

    return true;
}
