#ifndef NCORE_HMAC_H
#define NCORE_HMAC_H

#include <array>
#include <string>
#include <string_view>
#include "CryptoConstants.h"
#include "CryptoHash.h"
#include "Errors.h"

class BigNumber;

namespace Acore::Impl
{
    template <GenericHashImpl::HashCreator HashCreator, std::size_t DigestLength>
    class GenericHMAC
    {
        public:
            static constexpr std::size_t DIGEST_LENGTH = DigestLength;
            using Digest = std::array<uint8, DIGEST_LENGTH>;

            template <typename Container>
            static Digest GetDigestOf(Container const& seed, uint8 const* data, std::size_t len)
            {
                GenericHMAC hash(seed);
                hash.UpdateData(data, len);
                hash.Finalize();
                return hash.GetDigest();
            }

            template <typename Container, typename... Ts>
            static auto GetDigestOf(Container const& seed, Ts&&... pack) -> std::enable_if_t<!(std::is_integral_v<std::decay_t<Ts>> || ...), Digest>
            {
                GenericHMAC hash(seed);
                (hash.UpdateData(std::forward<Ts>(pack)), ...);
                hash.Finalize();
                return hash.GetDigest();
            }

            GenericHMAC(uint8 const* seed, const std::size_t len) : _ctx(GenericHashImpl::MakeCTX()), _key(EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, seed, len))
            {
                const int result = EVP_DigestSignInit(_ctx, nullptr, HashCreator(), nullptr, _key);
                ASSERT(result == 1);
            }

            template <typename Container>
            explicit GenericHMAC(Container const& container) : GenericHMAC(std::data(container), std::size(container)) {}

            GenericHMAC(GenericHMAC const& right) : _ctx(GenericHashImpl::MakeCTX())
            {
                *this = right;
            }

            GenericHMAC(GenericHMAC&& right) noexcept
            {
                *this = std::move(right);
            }

            ~GenericHMAC()
            {
                GenericHashImpl::DestroyCTX(_ctx);
                _ctx = nullptr;
                EVP_PKEY_free(_key);
                _key = nullptr;
            }

            GenericHMAC& operator=(GenericHMAC const& right)
            {
                if (this == &right)
                    return *this;

                int result = EVP_MD_CTX_copy_ex(_ctx, right._ctx);
                ASSERT(result == 1);
                _key = right._key;      // EVP_PKEY uses reference counting internally, just copy the pointer
                EVP_PKEY_up_ref(_key);  // Bump reference count for PKEY, as every instance of this class holds two references to PKEY and destructor decrements it twice
                _digest = right._digest;
                return *this;
            }

            GenericHMAC& operator=(GenericHMAC&& right) noexcept
            {
                if (this == &right)
                    return *this;

                _ctx = std::exchange(right._ctx, GenericHashImpl::MakeCTX());
                _key = std::exchange(right._key, EVP_PKEY_new());
                _digest = std::exchange(right._digest, Digest{});
                return *this;
            }

            void UpdateData(uint8 const* data, const std::size_t len) const
            {
                const int result = EVP_DigestSignUpdate(_ctx, data, len);
                ASSERT(result == 1);
            }

            void UpdateData(const std::string_view str) const
            {
                UpdateData(reinterpret_cast<uint8 const*>(str.data()), str.size());
            }
            void UpdateData(std::string const& str) { UpdateData(std::string_view(str)); } /* explicit overload to avoid using the container template */
            void UpdateData(char const* str) { UpdateData(std::string_view(str)); } /* explicit overload to avoid using the container template */

            template <typename Container>
            void UpdateData(Container const& c) { UpdateData(std::data(c), std::size(c)); }

            void Finalize()
            {
                std::size_t length = DIGEST_LENGTH;
                const int result = EVP_DigestSignFinal(_ctx, _digest.data(), &length);
                ASSERT(result == 1);
                ASSERT(length == DIGEST_LENGTH);
            }

            Digest const& GetDigest() const { return _digest; }
        private:
            EVP_MD_CTX* _ctx{};
            EVP_PKEY* _key{};
            Digest _digest{};
    };
}

namespace Acore::Crypto
{
    using HMAC_SHA1 = Impl::GenericHMAC<EVP_sha1, Constants::SHA1_DIGEST_LENGTH_BYTES>;
    using HMAC_SHA256 = Impl::GenericHMAC<EVP_sha256, Constants::SHA256_DIGEST_LENGTH_BYTES>;
}

#endif
