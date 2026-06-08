#ifndef NCORE_AUTH_CRYPT_H
#define NCORE_AUTH_CRYPT_H

#include "ARC4.h"
#include "AuthDefines.h"

class AuthCrypt
{
public:
    AuthCrypt() = default;

    void Init(SessionKey const& K);
    void DecryptRecv(uint8* data, std::size_t len);
    void EncryptSend(uint8* data, std::size_t len);

    bool IsInitialized() const { return _initialized; }

private:
    Acore::Crypto::ARC4 _clientDecrypt;
    Acore::Crypto::ARC4 _serverEncrypt;
    bool _initialized{ false };
};
#endif
