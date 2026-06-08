#include "OpenSSLCrypto.h"
#include <openssl/crypto.h> // NOTE: this import is NEEDED (even though some IDEs report it as unused)
#include <openssl/provider.h>

OSSL_PROVIDER* LegacyProvider;
OSSL_PROVIDER* DefaultProvider;

void OpenSSLCrypto::threadsSetup()
{
    LegacyProvider = OSSL_PROVIDER_load(nullptr, "legacy");
    DefaultProvider = OSSL_PROVIDER_load(nullptr, "default");
}

void OpenSSLCrypto::threadsCleanup()
{
    OSSL_PROVIDER_unload(LegacyProvider);
    OSSL_PROVIDER_unload(DefaultProvider);
    OSSL_PROVIDER_set_default_search_path(nullptr, nullptr);
}
