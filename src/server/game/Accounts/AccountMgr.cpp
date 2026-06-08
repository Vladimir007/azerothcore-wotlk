/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AccountMgr.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SRP6.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "WorldSession.h"

namespace AccountMgr
{
    uint32 GetId(std::string const& username)
    {
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME);
        stmt->SetData(0, username);
        QueryResult result = LoginDatabase.Query(stmt);

        return (result) ? (*result)[0].Get<uint32>() : 0;
    }

    bool IsGameMaster(const uint32 accountId)
    {
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_GM_LEVEL);
        stmt->SetData(0, accountId);
        const QueryResult result = LoginDatabase.Query(stmt);
        return result ? (*result)[0].Get<bool>() : false;
    }

    bool IsStaff(const uint32 accountId)
    {
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_STAFF_LEVEL);
        stmt->SetData(0, accountId);
        const QueryResult result = LoginDatabase.Query(stmt);
        return result ? (*result)[0].Get<bool>() : false;
    }

    bool GetName(uint32 accountId, std::string& name)
    {
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_USERNAME_BY_ID);
        stmt->SetData(0, accountId);
        QueryResult result = LoginDatabase.Query(stmt);

        if (result)
        {
            name = (*result)[0].Get<std::string>();
            return true;
        }

        return false;
    }

    bool CheckPassword(uint32 accountId, std::string password)
    {
        std::string username;

        if (!GetName(accountId, username))
            return false;

        Utf8ToUpperOnlyLatin(username);
        Utf8ToUpperOnlyLatin(password);

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_CHECK_PASSWORD);
        stmt->SetData(0, accountId);
        if (QueryResult result = LoginDatabase.Query(stmt))
        {
            Acore::Crypto::SRP6::Salt salt = (*result)[0].Get<Binary, Acore::Crypto::SRP6::SALT_LENGTH>();
            Acore::Crypto::SRP6::Verifier verifier = (*result)[1].Get<Binary, Acore::Crypto::SRP6::VERIFIER_LENGTH>();
            if (Acore::Crypto::SRP6::CheckLogin(username, password, salt, verifier))
                return true;
        }

        return false;
    }

    uint32 GetCharactersCount(uint32 accountId)
    {
        // check character count
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
        stmt->SetData(0, accountId);
        QueryResult result = CharacterDatabase.Query(stmt);

        return (result) ? (*result)[0].Get<uint64>() : 0;
    }

    bool IsPlayerAccount(uint32 gmlevel)
    {
        return gmlevel == SEC_PLAYER;
    }

    bool IsAdminAccount(uint32 gmlevel)
    {
        return gmlevel >= SEC_ADMINISTRATOR && gmlevel <= SEC_CONSOLE;
    }

    bool IsConsoleAccount(uint32 gmlevel)
    {
        return gmlevel == SEC_CONSOLE;
    }

} // Namespace AccountMgr
