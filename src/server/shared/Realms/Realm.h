#ifndef REALM_H
#define REALM_H

#include <memory> // NOTE: this import is NEEDED (even though some IDEs report it as unused)
#include "AsioHacksFwd.h"
#include "Common.h"

typedef boost::asio::ip::address IPAddress;

enum RealmFlags
{
    REALM_FLAG_NONE             = 0x0,
    REALM_FLAG_VERSION_MISMATCH = 0x1,
    REALM_FLAG_OFFLINE          = 0x2,
};

// Storage object for a realm
struct Realm
{
    uint32 ID;
    std::unique_ptr<IPAddress> ExternalAddress;
    std::unique_ptr<IPAddress> LocalAddress;
    std::unique_ptr<IPAddress> LocalSubnetMask;
    uint16 Port;
    std::string Name;
    RealmFlags Flags;
    uint8 Timezone;

    [[nodiscard]] boost::asio::ip::tcp_endpoint GetAddressForClient(const IPAddress& clientAddr) const;

};

#endif
