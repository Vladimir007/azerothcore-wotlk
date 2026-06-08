#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <string>
#include "Optional.h"

namespace Acore::Asio
{
    /**
     Hack to make it possible to forward declare resolver (one of its template arguments is a typedef to something super long and using nested classes)
    */
    class Resolver
    {
    public:
        explicit Resolver(IoContext& ioContext) : _impl(ioContext) { }

        Optional<tcp::endpoint> Resolve(const tcp& protocol, const std::string& host, const std::string& service)
        {
            boost::system::error_code ec;
            constexpr boost::asio::ip::resolver_base::flags flagsResolver = boost::asio::ip::resolver_base::all_matching;
            const tcp::resolver::results_type results = _impl.resolve(protocol, host, service, flagsResolver, ec);
            if (results.begin() == results.end() || ec)
                return {};

            return results.begin()->endpoint();
        }

    private:
        tcp::resolver _impl;
    };
}

#endif
