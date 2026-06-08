#include "AppenderDB.h"
#include "DatabaseEnv.h"
#include "LogMessage.h"
#include "PreparedStatement.h"

AppenderDB::AppenderDB(const uint8 id, const std::string& name, const LogLevel level, AppenderFlags /*flags*/) : Appender(id, name, level) { }

AppenderDB::~AppenderDB() { }

void AppenderDB::_write(const LogMessage* message)
{
    // Avoid infinite loop, Execute triggers Logging with "sql.sql" type
    if (message->type.find("sql") != std::string::npos)
        return;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_LOG);
    stmt->SetData(0, message->mtime.count());
    stmt->SetData(1, message->type);
    stmt->SetData(2, static_cast<uint8>(message->level));
    stmt->SetData(3, message->text);
    LoginDatabase.Execute(stmt);
}
