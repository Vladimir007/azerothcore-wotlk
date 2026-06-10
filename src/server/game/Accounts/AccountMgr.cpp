#include "AccountMgr.h"

#include <ranges>

#include "Common.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "SRP6.h"
#include "ScriptMgr.h"
#include "Util.h"

AccountMgr::AccountMgr() = default;
AccountMgr::~AccountMgr() = default;

AccountMgr* AccountMgr::instance()
{
    static AccountMgr instance;
    return &instance;
}

uint32 AccountMgr::GetId(const std::string& username)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME);
    stmt->SetData(0, username);
    const QueryResult result = LoginDatabase.Query(stmt);
    return result ? (*result)[0].Get<uint32>() : 0;
}

bool AccountMgr::IsSuperuser(const uint32 accountId)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_IS_SUPERUSER);
    stmt->SetData(0, accountId);
    const QueryResult result = LoginDatabase.Query(stmt);
    return result ? (*result)[0].Get<bool>() : false;
}

bool AccountMgr::IsStaff(const uint32 accountId)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_IS_STAFF);
    stmt->SetData(0, accountId);
    const QueryResult result = LoginDatabase.Query(stmt);
    return result ? (*result)[0].Get<bool>() : false;
}

bool AccountMgr::GetName(const uint32 accountId, std::string& name)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_USERNAME_BY_ID);
    stmt->SetData(0, accountId);

    if (const QueryResult result = LoginDatabase.Query(stmt))
    {
        name = (*result)[0].Get<std::string>();
        return true;
    }

    return false;
}

bool AccountMgr::CheckPassword(const uint32 accountId, std::string password)
{
    std::string username;

    if (!GetName(accountId, username))
        return false;

    Utf8ToUpperOnlyLatin(username);
    Utf8ToUpperOnlyLatin(password);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_CHECK_PASSWORD);
    stmt->SetData(0, accountId);
    if (const QueryResult result = LoginDatabase.Query(stmt))
    {
        const Acore::Crypto::SRP6::Salt salt = (*result)[0].Get<Binary, Acore::Crypto::SRP6::SALT_LENGTH>();
        const Acore::Crypto::SRP6::Verifier verifier = (*result)[1].Get<Binary, Acore::Crypto::SRP6::VERIFIER_LENGTH>();
        if (Acore::Crypto::SRP6::CheckLogin(username, password, salt, verifier))
            return true;
    }

    return false;
}

uint32 AccountMgr::GetCharactersCount(const uint32 accountId)
{
    // Get character count
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
    stmt->SetData(0, accountId);
    const QueryResult result = CharacterDatabase.Query(stmt);

    return result ? (*result)[0].Get<uint64>() : 0;
}
