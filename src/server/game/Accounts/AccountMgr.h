#ifndef ACCOUNT_MANAGER_H
#define ACCOUNT_MANAGER_H

#include <string>
#include "Define.h"

enum AccountOpResult
{
    AOR_OK,
    AOR_NAME_TOO_LONG,
    AOR_PASS_TOO_LONG,
    AOR_EMAIL_TOO_LONG,
    AOR_NAME_ALREADY_EXIST,
    AOR_NAME_NOT_EXIST,
    AOR_DB_INTERNAL_ERROR
};

#define MAX_ACCOUNT_STR 17
#define MAX_PASS_STR 16
#define MAX_EMAIL_STR 255

class AccountMgr
{
    AccountMgr();
    ~AccountMgr();

public:
    static AccountMgr* instance();
    static bool CheckPassword(uint32 accountId, std::string password);

    static uint32 GetId(std::string const& username);
    static bool IsSuperuser(uint32 accountId);
    static bool IsStaff(uint32 accountId);
    static bool GetName(uint32 accountId, std::string& name);
    static uint32 GetCharactersCount(uint32 accountId);
};

#define sAccountMgr AccountMgr::instance()

#endif
