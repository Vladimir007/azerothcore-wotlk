#ifndef NCORE_OPENSSL_CRYPTO_H
#define NCORE_OPENSSL_CRYPTO_H

/**
* A group of functions which setup openssl crypto module to work properly in multithreaded environment
* If not setup properly - it will crash
*/
namespace OpenSSLCrypto
{
    /// Needs to be called before threads using openssl are spawned
    void threadsSetup();

    /// Needs to be called after threads using openssl are despawned
    void threadsCleanup();
}

#endif
