#include "BigNumber.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <openssl/bn.h>
#include "Errors.h"

BigNumber::BigNumber() : _bn(BN_new())
{ }

BigNumber::BigNumber(BigNumber const& bn) : _bn(BN_dup(bn.BN()))
{ }

BigNumber::~BigNumber()
{
    BN_free(_bn);
}

void BigNumber::SetDword(const int32 val)
{
    SetDword(static_cast<uint32>(std::abs(val)));
    if (val < 0)
        BN_set_negative(_bn, 1);
}

void BigNumber::SetDword(const uint32 val)
{
    BN_set_word(_bn, val);
}

void BigNumber::SetQword(const uint64 val)
{
    BN_set_word(_bn, static_cast<uint32>(val >> 32));
    BN_lshift(_bn, _bn, 32);
    BN_add_word(_bn, static_cast<uint32>(val & 0xFFFFFFFF));
}

void BigNumber::SetBinary(uint8 const* bytes, const int32 len, const bool littleEndian)
{
    if (littleEndian)
        BN_lebin2bn(bytes, len, _bn);
    else
        BN_bin2bn(bytes, len, _bn);
}

bool BigNumber::SetHexStr(char const* str)
{
    const int n = BN_hex2bn(&_bn, str);
    return n > 0;
}

void BigNumber::SetRand(const int32 numbits)
{
    BN_rand(_bn, numbits, 0, 1);
}

BigNumber& BigNumber::operator=(BigNumber const& bn)
{
    if (this == &bn)
        return *this;

    BN_copy(_bn, bn._bn);
    return *this;
}

BigNumber& BigNumber::operator+=(BigNumber const& bn)
{
    BN_add(_bn, _bn, bn._bn);
    return *this;
}

BigNumber& BigNumber::operator-=(BigNumber const& bn)
{
    BN_sub(_bn, _bn, bn._bn);
    return *this;
}

BigNumber& BigNumber::operator*=(BigNumber const& bn)
{
    BN_CTX* bnCtx = BN_CTX_new();
    BN_mul(_bn, _bn, bn._bn, bnCtx);
    BN_CTX_free(bnCtx);
    return *this;
}

BigNumber& BigNumber::operator/=(BigNumber const& bn)
{
    BN_CTX* bnCtx = BN_CTX_new();
    BN_div(_bn, nullptr, _bn, bn._bn, bnCtx);
    BN_CTX_free(bnCtx);

    return *this;
}

BigNumber& BigNumber::operator%=(BigNumber const& bn)
{
    BN_CTX* bnCtx = BN_CTX_new();
    BN_mod(_bn, _bn, bn._bn, bnCtx);
    BN_CTX_free(bnCtx);
    return *this;
}

BigNumber& BigNumber::operator<<=(const int n)
{
    BN_lshift(_bn, _bn, n);
    return *this;
}

int BigNumber::CompareTo(BigNumber const& bn) const
{
    return BN_cmp(_bn, bn._bn);
}

BigNumber BigNumber::Exp(BigNumber const& bn) const
{
    BigNumber ret;

    BN_CTX* bnCtx = BN_CTX_new();
    BN_exp(ret._bn, _bn, bn._bn, bnCtx);
    BN_CTX_free(bnCtx);

    return ret;
}

BigNumber BigNumber::ModExp(BigNumber const& bn1, BigNumber const& bn2) const
{
    BigNumber ret;

    BN_CTX* bnCtx = BN_CTX_new();
    BN_mod_exp(ret._bn, _bn, bn1._bn, bn2._bn, bnCtx);
    BN_CTX_free(bnCtx);

    return ret;
}

int32 BigNumber::GetNumBytes() const
{
    return BN_num_bytes(_bn);
}

uint32 BigNumber::AsDword() const
{
    return static_cast<uint32>(BN_get_word(_bn));
}

bool BigNumber::IsZero() const
{
    return BN_is_zero(_bn);
}

bool BigNumber::IsNegative() const
{
    return BN_is_negative(_bn);
}

void BigNumber::GetBytes(uint8* buf, std::size_t bufSize, const bool littleEndian) const
{
    const int res = littleEndian ? BN_bn2lebinpad(_bn, buf, bufSize) : BN_bn2binpad(_bn, buf, bufSize);
    ASSERT(res > 0, "Buffer of size {} is too small to hold bignum with {} bytes.\n", bufSize, BN_num_bytes(_bn));
}

std::vector<uint8> BigNumber::ToByteVector(const int32 minSize, const bool littleEndian) const
{
    const std::size_t length = std::max(GetNumBytes(), minSize);
    std::vector<uint8> v;
    v.resize(length);
    GetBytes(v.data(), length, littleEndian);
    return v;
}

std::string BigNumber::AsHexStr() const
{
    char* ch = BN_bn2hex(_bn);
    std::string ret = ch;
    OPENSSL_free(ch);
    return ret;
}

std::string BigNumber::AsDecStr() const
{
    char* ch = BN_bn2dec(_bn);
    std::string ret = ch;
    OPENSSL_free(ch);
    return ret;
}
