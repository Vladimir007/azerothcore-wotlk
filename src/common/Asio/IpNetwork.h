#ifndef IP_NETWORK_HPP
#define IP_NETWORK_HPP

#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include "Define.h"
#include "IpAddress.h"

namespace Acore::Net
{
    inline bool IsInNetwork(boost::asio::ip::address_v4 const& networkAddress, boost::asio::ip::address_v4 const& mask, boost::asio::ip::address_v4 const& clientAddress)
    {
        const boost::asio::ip::network_v4 network = boost::asio::ip::make_network_v4(networkAddress, mask);
        const boost::asio::ip::address_v4_range hosts = network.hosts();
        return hosts.find(clientAddress) != hosts.end();
    }

    inline boost::asio::ip::address_v4 GetDefaultNetmaskV4(boost::asio::ip::address_v4 const& networkAddress)
    {
        if ((address_to_uint(networkAddress) & 0x80000000) == 0)
            return boost::asio::ip::address_v4(0xFF000000);
        if ((address_to_uint(networkAddress) & 0xC0000000) == 0x80000000)
            return boost::asio::ip::address_v4(0xFFFF0000);
        if ((address_to_uint(networkAddress) & 0xE0000000) == 0xC0000000)
            return boost::asio::ip::address_v4(0xFFFFFF00);
        return boost::asio::ip::address_v4(0xFFFFFFFF);
    }

    inline bool IsInNetwork(boost::asio::ip::address_v6 const& networkAddress, const uint16 prefixLength, boost::asio::ip::address_v6 const& clientAddress)
    {
        const boost::asio::ip::network_v6 network = boost::asio::ip::make_network_v6(networkAddress, prefixLength);
        const boost::asio::ip::address_v6_range hosts = network.hosts();
        return hosts.find(clientAddress) != hosts.end();
    }
}

#endif
