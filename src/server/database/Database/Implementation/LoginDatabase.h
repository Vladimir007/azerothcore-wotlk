#ifndef LOGIN_DATABASE_H
#define LOGIN_DATABASE_H

#include "PSQLConnection.h"

enum LoginDatabaseStatements : uint32
{
    LOGIN_SEL_REALM_INFO,
    LOGIN_UPD_LOGON_PROOF,
    LOGIN_SEL_LOGON_CHALLENGE,
    LOGIN_SEL_RECONNECT_CHALLENGE,
    LOGIN_SEL_ACCOUNT_INFO_BY_NAME,
    LOGIN_SEL_ACCOUNT_BY_ID,
    LOGIN_UPD_CHARACTERS_COUNT,
    LOGIN_SEL_CHARACTERS_COUNT,
    LOGIN_UPD_ACCOUNT_ONLINE,
    LOGIN_UPD_UPTIME_PLAYERS,
    LOGIN_DEL_OLD_LOGS,
    LOGIN_GET_ACCOUNT_ID_BY_USERNAME,
    LOGIN_GET_ACCOUNT_GM_LEVEL,
    LOGIN_GET_ACCOUNT_IS_SUPERUSER,
    LOGIN_GET_ACCOUNT_IS_STAFF,
    LOGIN_GET_USERNAME_BY_ID,
    LOGIN_SEL_CHECK_PASSWORD,
    LOGIN_SEL_CHECK_PASSWORD_BY_NAME,
    LOGIN_SEL_GM_ACCOUNTS,
    LOGIN_SEL_ACCOUNT_STAFF_ACCESS,
    LOGIN_SEL_ACCOUNT_WHOIS,
    LOGIN_DEL_ACCOUNT,
    LOGIN_INS_LOG,
    LOGIN_INS_UPTIME,

    MAX_LOGIN_DATABASE_STATEMENTS
};

class LoginDatabaseConnection : public PSQLConnection
{
public:
    typedef LoginDatabaseStatements Statements;

    //- Constructors for sync and async connections
    explicit LoginDatabaseConnection(const std::string& connectionStr);
    LoginDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* queue, const std::string& connectionStr);
    ~LoginDatabaseConnection() override;

protected:
    //- Loads database type specific prepared statements
    void DoPrepareStatements() override;
};

#endif
