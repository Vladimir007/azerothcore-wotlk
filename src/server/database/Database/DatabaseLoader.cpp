#include "DatabaseLoader.h"

#include <thread>

#include "Config.h"
#include "DatabaseEnv.h"
#include "Duration.h"
#include "Log.h"

#define DATABASE_INFO_DEFAULT "postgresql://nordcore:nordcore@127.0.0.1:5432/postgres"

DatabaseLoader::DatabaseLoader(std::string const& logger): _logger(logger) { }

template <class T>
DatabaseLoader& DatabaseLoader::AddDatabase(DatabaseWorkerPool<T>& pool, std::string const& name)
{
    _open.push([this, name, &pool]() -> bool
    {
        auto const& defaultDatabaseInfo = std::string(DATABASE_INFO_DEFAULT);
        std::string const dbString = sConfigMgr->GetOption<std::string>("DatabaseInfo", defaultDatabaseInfo);
        if (dbString.empty())
        {
            LOG_ERROR(_logger, "Database {} not specified in configuration file!", name);
            return false;
        }

        pool.SetConnectionStr(dbString);

        if (!pool.Open())
        {
            bool failed = true;
            // Try reconnect
            uint8 const attempts = sConfigMgr->GetOption<uint8>("Database.Reconnect.Attempts", 20);
            const auto reconnectSeconds = Seconds(sConfigMgr->GetOption<uint8>("Database.Reconnect.Seconds", 15));
            uint8 reconnectCount = 0;

            while (reconnectCount < attempts)
            {
                LOG_WARN(_logger, "> Retrying after {} seconds", static_cast<uint32>(reconnectSeconds.count()));
                std::this_thread::sleep_for(reconnectSeconds);

                if (!pool.Open())
                    reconnectCount++;
                else
                {
                    failed = false;
                    break;
                }
            }
            if (failed)
            {
                LOG_ERROR(_logger, "DatabasePool {} NOT opened. Check your log file for specific errors", name);
                return false;
            }
        }
        // Add the close operation
        _close.push([&pool]
        {
            pool.Close();
        });

        return true;
    });

    _prepare.push([this, name, &pool]() -> bool
    {
        if (!pool.PrepareStatements())
        {
            LOG_ERROR(_logger, "Could not prepare statements of the {} database, see log for details.", name);
            return false;
        }

        return true;
    });

    return *this;
}

bool DatabaseLoader::Load()
{
    if (!Process(_open))
        return false;
    if (!Process(_prepare))
        return false;
    return true;
}

bool DatabaseLoader::Process(std::queue<Predicate>& queue)
{
    while (!queue.empty())
    {
        if (!queue.front()())
        {
            // Close all open databases which have a registered close operation
            while (!_close.empty())
            {
                _close.top()();
                _close.pop();
            }

            return false;
        }

        queue.pop();
    }

    return true;
}

template AC_DATABASE_API
DatabaseLoader& DatabaseLoader::AddDatabase<LoginDatabaseConnection>(DatabaseWorkerPool<LoginDatabaseConnection>&, std::string const&);
template AC_DATABASE_API
DatabaseLoader& DatabaseLoader::AddDatabase<CharacterDatabaseConnection>(DatabaseWorkerPool<CharacterDatabaseConnection>&, std::string const&);
template AC_DATABASE_API
DatabaseLoader& DatabaseLoader::AddDatabase<WorldDatabaseConnection>(DatabaseWorkerPool<WorldDatabaseConnection>&, std::string const&);
