#ifndef IP_ADDRESS_HPP
#define IP_ADDRESS_HPP

#include <boost/asio/ip/address.hpp>
#include "Define.h"

namespace Acore::Net
{
    using boost::asio::ip::make_address;
    using boost::asio::ip::make_address_v4;
    inline uint32 address_to_uint(boost::asio::ip::address_v4 const& address) { return address.to_uint(); }
}

#endif
