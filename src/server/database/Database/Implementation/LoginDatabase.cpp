#include "LoginDatabase.h"

void LoginDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_LOGIN_DATABASE_STATEMENTS);

    PrepareStatement(LOGIN_SEL_LOGON_CHALLENGE,
        "SELECT id, UPPER(username), is_active, is_game_master, salt, verifier FROM accounts WHERE UPPER(username) = UPPER($1)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_RECONNECT_CHALLENGE,
        "SELECT id, UPPER(username), is_active, is_game_master, session_key FROM accounts WHERE UPPER(username) = UPPER($1) AND session_key IS NOT NULL", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_ACCOUNT_INFO_BY_NAME,
        "SELECT id, session_key, is_active, is_game_master, locale, total_time FROM accounts WHERE UPPER(username) = UPPER($1) AND session_key IS NOT NULL", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_REALM_INFO, "SELECT id, name, address, local_address, local_subnet_mask, port, flag, timezone FROM realm LIMIT 1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_UPD_LOGON_PROOF, "UPDATE accounts SET session_key=$1 locale=$2 WHERE UPPER(username)=$3", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BY_ID, "SELECT 1 FROM accounts WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_UPD_CHARACTERS_COUNT, "UPDATE accounts SET characters=$1 WHERE id=$2;", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_CHARACTERS_COUNT, "SELECT characters FROM accounts WHERE id=$1", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_ACCOUNT_ONLINE, "UPDATE accounts SET online=$1 WHERE id=$2", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_UPTIME_PLAYERS, "UPDATE realm_uptime SET uptime=$1, max_players=$2 WHERE start_time=$3", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_OLD_LOGS, "DELETE FROM realm_logs WHERE (time + $1) < $2", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME, "SELECT id FROM accounts WHERE UPPER(username)=UPPER($1)", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_GET_ACCOUNT_GM_LEVEL, "SELECT is_game_master FROM accounts WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_GET_ACCOUNT_STAFF_LEVEL, "SELECT is_staff FROM accounts WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_GET_USERNAME_BY_ID, "SELECT username FROM accounts WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_CHECK_PASSWORD, "SELECT salt, verifier FROM accounts WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_CHECK_PASSWORD_BY_NAME, "SELECT salt, verifier FROM accounts WHERE UPPER(username)=UPPER($1)", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_GM_ACCOUNTS, "SELECT username FROM accounts WHERE is_active=TRUE AND is_game_master=TRUE", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_STAFF_ACCESS, "SELECT is_active AND is_staff AS staff_access FROM accounts WHERE UPPER(username)=$1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_WHOIS, "SELECT username FROM accounts WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_INS_LOG, "INSERT INTO realm_logs (time, type, level, message) VALUES ($1, $2, $3, $4)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_INS_UPTIME, "INSERT INTO realm_uptime (start_time, max_players, uptime) VALUES ($1, 0, 0)", CONNECTION_ASYNC);
}

LoginDatabaseConnection::LoginDatabaseConnection(const std::string& connectionStr) : PSQLConnection(connectionStr) { }

LoginDatabaseConnection::LoginDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* queue, const std::string& connectionStr):
    PSQLConnection(queue, connectionStr) { }

LoginDatabaseConnection::~LoginDatabaseConnection() { }
