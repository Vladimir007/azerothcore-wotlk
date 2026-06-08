#include "AES.h"
#include <limits>
#include "Errors.h"

Acore::Crypto::AES::AES(const bool encrypting) : _ctx(EVP_CIPHER_CTX_new()), _encrypting(encrypting)
{
    EVP_CIPHER_CTX_init(_ctx);
    const int status = EVP_CipherInit_ex(_ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr, _encrypting ? 1 : 0);
    ASSERT(status);
}

Acore::Crypto::AES::~AES()
{
    EVP_CIPHER_CTX_free(_ctx);
}

void Acore::Crypto::AES::Init(Key const& key)
{
    const int status = EVP_CipherInit_ex(_ctx, nullptr, nullptr, key.data(), nullptr, -1);
    ASSERT(status);
}

bool Acore::Crypto::AES::Process(IV const& iv, uint8* data, const std::size_t length, Tag& tag)
{
    ASSERT(length <= static_cast<size_t>(std::numeric_limits<int>::max()));
    int len = static_cast<int>(length);
    if (!EVP_CipherInit_ex(_ctx, nullptr, nullptr, nullptr, iv.data(), -1))
        return false;

    int outLen;
    if (!EVP_CipherUpdate(_ctx, data, &outLen, data, len))
        return false;

    len -= outLen;

    if (!_encrypting && !EVP_CIPHER_CTX_ctrl(_ctx, EVP_CTRL_GCM_SET_TAG, sizeof(tag), tag))
        return false;

    if (!EVP_CipherFinal_ex(_ctx, data + outLen, &outLen))
        return false;

    ASSERT(len == outLen);

    if (_encrypting && !EVP_CIPHER_CTX_ctrl(_ctx, EVP_CTRL_GCM_GET_TAG, sizeof(tag), tag))
        return false;

    return true;
}
