#ifndef IO_CONTEXT_HPP
#define IO_CONTEXT_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#define IoContextBaseNamespace boost::asio
#define IoContextBase io_context

namespace Acore::Asio
{
    class IoContext
    {
    public:
        IoContext() { }
        explicit IoContext(const int concurrency_hint) : _impl(concurrency_hint) { }

        operator IoContextBaseNamespace::IoContextBase&() { return _impl; }
        operator IoContextBaseNamespace::IoContextBase const&() const { return _impl; }

        std::size_t run() { return _impl.run(); }
        void stop() { _impl.stop(); }

        boost::asio::io_context::executor_type get_executor() noexcept { return _impl.get_executor(); }

    private:
        IoContextBaseNamespace::IoContextBase _impl;
    };

    template<typename T>
    decltype(auto) post(IoContextBaseNamespace::IoContextBase& ioContext, T&& t)
    {
        return boost::asio::post(ioContext, std::forward<T>(t));
    }

    template<typename T>
    boost::asio::io_context& get_io_context(T&& ioObject)
    {
        return static_cast<boost::asio::io_context&>(ioObject.get_executor().context());
    }
}

#endif
