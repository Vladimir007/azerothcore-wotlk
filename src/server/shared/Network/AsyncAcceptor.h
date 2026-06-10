#ifndef ASYNC_ACCEPTOR_H
#define ASYNC_ACCEPTOR_H

#include "IpAddress.h"
#include "Log.h"
#include "Socket.h"
#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <functional>

#include "IoContext.h"

using boost::asio::ip::tcp;

constexpr auto NCORE_MAX_LISTEN_CONNECTIONS = boost::asio::socket_base::max_listen_connections;

class AsyncAcceptor
{
public:
    typedef void(*AcceptCallback)(IoContextTcpSocket& socket);

    AsyncAcceptor(Acore::Asio::IoContext& ioContext, std::string const& bindIp, const uint16 port) : // NOLINT(*-pro-type-member-init)
        _acceptor(ioContext), _endpoint(Acore::Net::make_address(bindIp), port),
        _socket(ioContext), _closed(false), _socketFactory([this](){ return DefaultSocketFactory(); })
    {
    }

    template<class T>
    void AsyncAccept();

    template<AcceptCallback acceptCallback>
    void AsyncAcceptWithCallback()
    {
        auto socket = _socketFactory();
        _acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& error)
        {
            if (!error)
            {
                try
                {
                    socket->non_blocking(true);
                    acceptCallback(*socket);
                }
                catch (boost::system::system_error const& err)
                {
                    LOG_INFO("network", "Failed to initialize client's socket {}", err.what());
                }
            }

            if (!_closed)
                this->AsyncAcceptWithCallback<acceptCallback>();
        });
    }

    bool Bind() // NOLINT(*-convert-member-functions-to-static)
    {
        boost::system::error_code errorCode;
        // With socket activation the acceptor is already open and bound
        if (!_acceptor.is_open())
        {
            _acceptor.open(_endpoint.protocol(), errorCode);
            if (errorCode)
            {
                LOG_INFO("network", "Failed to open acceptor {}", errorCode.message());
                return false;
            }

            _acceptor.set_option(tcp::acceptor::reuse_address(true), errorCode);
            if (errorCode)
            {
                LOG_INFO("network", "Failed to set reuse_address option on acceptor {}", errorCode.message());
                return false;
            }

            _acceptor.bind(_endpoint, errorCode);
            if (errorCode)
            {
                LOG_INFO("network", "Could not bind to {}:{} {}", _endpoint.address().to_string(), _endpoint.port(), errorCode.message());
                return false;
            }
        }

        _acceptor.listen(NCORE_MAX_LISTEN_CONNECTIONS, errorCode);
        if (errorCode)
        {
            LOG_INFO("network", "Failed to start listening on {}:{} {}", _endpoint.address().to_string(), _endpoint.port(), errorCode.message());
            return false;
        }

        return true;
    }

    void Close() // NOLINT(*-convert-member-functions-to-static)
    {
        if (_closed.exchange(true))
            return;
        boost::system::error_code err;
        _acceptor.close(err);
    }

    void SetSocketFactory(std::function<IoContextTcpSocket*()> func) { _socketFactory = std::move(func); }

private:
    IoContextTcpSocket* DefaultSocketFactory() { return &_socket; }

    boost::asio::basic_socket_acceptor<tcp, IoContextTcpSocket::executor_type> _acceptor;
    tcp::endpoint _endpoint;
    IoContextTcpSocket _socket;
    std::atomic<bool> _closed;
    std::function<IoContextTcpSocket*()> _socketFactory;
};

template<class T>
void AsyncAcceptor::AsyncAccept()
{
    _acceptor.async_accept(_socket, [this](const boost::system::error_code& error)
    {
        if (!error)
        {
            try
            {
                // this-> is required here to fix a segmentation fault in gcc 4.7.2 - reason is lambdas in a templated class
                std::make_shared<T>(std::move(this->_socket))->Start();
            }
            catch (boost::system::system_error const& err)
            {
                LOG_INFO("network", "Failed to retrieve client's remote address {}", err.what());
            }
        }

        // Let's slap some more this-> on this so we can fix this bug with gcc 4.7.2 throwing internals in your face
        if (!_closed)
            this->AsyncAccept<T>();
    });
}

#endif
