#ifndef STRAND_HPP
#define STRAND_HPP

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include "IoContext.h"

namespace Acore::Asio
{
    /// Hack to make it possible to forward declare strand (which is an inner class)
    class Strand : public IoContextBaseNamespace::IoContextBase::strand
    {
    public:
        explicit Strand(IoContext& ioContext) : IoContextBaseNamespace::IoContextBase::strand(ioContext) { }
    };

    using boost::asio::bind_executor;
}

#endif
