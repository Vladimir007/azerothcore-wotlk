#ifndef SOCKET_H
#define SOCKET_H

#include <atomic>
#include <memory>
#include <queue>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "Log.h"
#include "MessageBuffer.h"

#define READ_BLOCK_SIZE 4096
#ifdef BOOST_ASIO_HAS_IOCP
#define NC_SOCKET_USE_IOCP
#endif

// Specialize boost socket for io_context executor instead of type-erased any_io_executor
// This avoids the type-erasure overhead of any_io_executor
using IoContextTcpSocket = boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type>;

enum class SocketReadCallbackResult
{
    KeepReading,
    Stop
};

enum class SocketState : uint8
{
    Open = 0,
    Closing = 1,
    Closed = 2
};

enum ProxyHeaderAddressFamilyAndProtocol {
    PROXY_HEADER_ADDRESS_FAMILY_AND_PROTOCOL_TCP_V4 = 0x11,
    PROXY_HEADER_ADDRESS_FAMILY_AND_PROTOCOL_TCP_V6 = 0x21,
};

template<class T>
class Socket : public std::enable_shared_from_this<T>
{
public:
    explicit Socket(IoContextTcpSocket&& socket) : _socket(std::move(socket)), _remoteAddress(_socket.remote_endpoint().address()),
        _remotePort(_socket.remote_endpoint().port()), _state(SocketState::Open), _isWritingAsync(false)
    {
        _readBuffer.Resize(READ_BLOCK_SIZE);
    }

    virtual ~Socket()
    {
        _state = SocketState::Closed;
        boost::system::error_code error;
        _socket.close(error);
    }

    virtual void Start() = 0;

    virtual bool Update()
    {
        const SocketState state = _state.load();
        if (state == SocketState::Closed)
            return false;

#ifndef NC_SOCKET_USE_IOCP
        if (_isWritingAsync || (_writeQueue.empty() && state != SocketState::Closing))
            return true;

        while (HandleQueue()) {}
#endif

        return true;
    }

    [[nodiscard]] boost::asio::ip::address GetRemoteIpAddress() const
    {
        return _remoteAddress;
    }

    [[nodiscard]] uint16 GetRemotePort() const
    {
        return _remotePort;
    }

    void AsyncRead()
    {
        if (!IsOpen())
            return;

        _readBuffer.Normalize();
        _readBuffer.EnsureFreeSpace();
        _socket.async_read_some(boost::asio::buffer(_readBuffer.GetWritePointer(), _readBuffer.GetRemainingSpace()),
            std::bind(&Socket<T>::ReadHandlerInternal, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void AsyncReadProxyHeader()
    {
        if (!IsOpen())
            return;

        _readBuffer.Normalize();
        _readBuffer.EnsureFreeSpace();
        _socket.async_read_some(boost::asio::buffer(_readBuffer.GetWritePointer(), _readBuffer.GetRemainingSpace()),
            std::bind(&Socket::ProxyReadHeaderHandler, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void AsyncReadWithCallback(void (T::*callback)(boost::system::error_code, std::size_t))
    {
        if (!IsOpen())
            return;

        _readBuffer.Normalize();
        _readBuffer.EnsureFreeSpace();

        _socket.async_read_some(boost::asio::buffer(_readBuffer.GetWritePointer(), _readBuffer.GetRemainingSpace()),
            std::bind(callback, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void QueuePacket(MessageBuffer&& buffer)
    {
        _writeQueue.push(std::move(buffer));

#ifdef NC_SOCKET_USE_IOCP
        AsyncProcessQueue();
#endif
    }

    [[nodiscard]] bool IsOpen() const { return _state.load() == SocketState::Open; }

    void CloseSocket()
    {
        auto expected = SocketState::Open;
        if (!_state.compare_exchange_strong(expected, SocketState::Closed))
        {
            // If it was Closing, try to transition to Closed
            expected = SocketState::Closing;
            if (!_state.compare_exchange_strong(expected, SocketState::Closed))
                return; // Already closed
        }

        boost::system::error_code shutdownError;
        _socket.shutdown(boost::asio::socket_base::shutdown_send, shutdownError);

        if (shutdownError)
            LOG_DEBUG("network", "Socket::CloseSocket: {} errored when shutting down socket: {} ({})", GetRemoteIpAddress().to_string(),
                shutdownError.value(), shutdownError.message());

        OnClose();
    }

    /// Marks the socket for closing after write buffer becomes empty
    void DelayedCloseSocket()
    {
        auto expected = SocketState::Open;
        _state.compare_exchange_strong(expected, SocketState::Closing);
    }

    MessageBuffer& GetReadBuffer() { return _readBuffer; }

protected:
    virtual void OnClose() { }
    virtual SocketReadCallbackResult ReadHandler() = 0;

private:
    void AsyncProcessQueue()
    {
        if (_isWritingAsync)
            return;

        _isWritingAsync = true;

#ifdef NC_SOCKET_USE_IOCP
        MessageBuffer& buffer = _writeQueue.front();
        _socket.async_write_some(boost::asio::buffer(buffer.GetReadPointer(), buffer.GetActiveSize()), std::bind(&Socket<T>::WriteHandler,
            this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
#else
        _socket.async_wait(boost::asio::socket_base::wait_write, [self = this->shared_from_this()](boost::system::error_code error)
        {
            self->WriteHandlerWrapper(error, 0);
        });
#endif
    }

    void ReadHandlerInternal(const boost::system::error_code& error, const std::size_t transferredBytes)
    {
        if (error)
        {
            CloseSocket();
            return;
        }

        _readBuffer.WriteCompleted(transferredBytes);
        if (ReadHandler() == SocketReadCallbackResult::KeepReading)
            AsyncRead();
    }

    void ProxyReadHeaderHandler(const boost::system::error_code& error, const std::size_t transferredBytes)
    {
        if (error)
        {
            CloseSocket();
            return;
        }

        _readBuffer.WriteCompleted(transferredBytes);

        MessageBuffer& packet = GetReadBuffer();

        // minimumProxyProtocolV2Size = 28
        if (packet.GetActiveSize() < 28)
        {
            AsyncReadProxyHeader();
            return;
        }

        uint8* readPointer = packet.GetReadPointer();

        constexpr uint8 signatureSize = 12;
        const uint8 expectedSignature[signatureSize] = {0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A};
        if (memcmp(packet.GetReadPointer(), expectedSignature, signatureSize) != 0)
        {
            LOG_ERROR("network", "Socket::ProxyReadHeaderHandler: received bad PROXY Protocol v2 signature for {}", GetRemoteIpAddress().to_string());
            return;
        }

        const uint8 version = (readPointer[signatureSize] & 0xF0) >> 4;
        const uint8 command = (readPointer[signatureSize] & 0xF);

        if (version != 2)
        {
            LOG_ERROR("network", "Socket::ProxyReadHeaderHandler: received bad PROXY Protocol v2 signature for {}", GetRemoteIpAddress().to_string());
            return;
        }

        const uint8 addressFamily = readPointer[13];
        const uint16 len = (readPointer[14] << 8) | readPointer[15];
        if (static_cast<size_t>(len+16) > packet.GetActiveSize())
        {
            AsyncReadProxyHeader();
            return;
        }

        // Connection created by a proxy itself (health checks?), ignore and do nothing.
        if (command == 0)
        {
            packet.ReadCompleted(len+16);
            return;
        }

        const auto remainingLen = packet.GetActiveSize() - 16;
        readPointer += 16; // Skip strait to address.

        switch (addressFamily) {
            case PROXY_HEADER_ADDRESS_FAMILY_AND_PROTOCOL_TCP_V4:
            {
                if (remainingLen < 12)
                {
                    AsyncReadProxyHeader();
                    return;
                }

                boost::asio::ip::address_v4::bytes_type b;
                constexpr auto addressSize = sizeof(b);

                std::copy_n(readPointer, addressSize, b.begin());
                _remoteAddress = boost::asio::ip::address_v4(b);

                readPointer += 2 * addressSize; // Skip server address.
                _remotePort = (readPointer[0] << 8) | readPointer[1];

                break;
            }

            case PROXY_HEADER_ADDRESS_FAMILY_AND_PROTOCOL_TCP_V6:
            {
                if (remainingLen < 36)
                {
                    AsyncReadProxyHeader();
                    return;
                }

                boost::asio::ip::address_v6::bytes_type b;
                constexpr auto addressSize = sizeof(b);

                std::copy_n(readPointer, addressSize, b.begin());
                _remoteAddress = boost::asio::ip::address_v6(b);

                readPointer += 2 * addressSize; // Skip server address.
                _remotePort = (readPointer[0] << 8) | readPointer[1];

                break;
            }

            default:
                LOG_ERROR("network", "Socket::ProxyReadHeaderHandler: unsupported address family type {}", GetRemoteIpAddress().to_string());
                return;
        }

        packet.ReadCompleted(len+16);
    }

#ifdef NC_SOCKET_USE_IOCP
    void WriteHandler(const boost::system::error_code& error, const std::size_t transferredBytes)
    {
        if (!error)
        {
            _isWritingAsync = false;
            _writeQueue.front().ReadCompleted(transferredBytes);

            if (!_writeQueue.front().GetActiveSize())
                _writeQueue.pop();

            if (!_writeQueue.empty())
                AsyncProcessQueue();
            else if (_state.load() == SocketState::Closing)
                CloseSocket();
        }
        else
            CloseSocket();
    }
#else

    void WriteHandlerWrapper(const boost::system::error_code& /*error*/, const std::size_t /*transferredBytes*/)
    {
        _isWritingAsync = false;
        HandleQueue();
    }

    bool HandleQueue()
    {
        if (_writeQueue.empty())
            return false;

        MessageBuffer& queuedMessage = _writeQueue.front();

        const std::size_t bytesToSend = queuedMessage.GetActiveSize();

        boost::system::error_code error;
        const std::size_t bytesSent = _socket.write_some(boost::asio::buffer(queuedMessage.GetReadPointer(), bytesToSend), error);

        if (error)
        {
            if (error == boost::asio::error::would_block || error == boost::asio::error::try_again)
            {
                AsyncProcessQueue();
                return false;
            }

            _writeQueue.pop();

            if (_state.load() == SocketState::Closing && _writeQueue.empty())
                CloseSocket();

            return false;
        }
        if (bytesSent == 0)
        {
            _writeQueue.pop();

            if (_state.load() == SocketState::Closing && _writeQueue.empty())
                CloseSocket();

            return false;
        }
        if (bytesSent < bytesToSend) // now n > 0
        {
            queuedMessage.ReadCompleted(bytesSent);
            AsyncProcessQueue();
            return false;
        }

        _writeQueue.pop();

        if (_state.load() == SocketState::Closing && _writeQueue.empty())
            CloseSocket();

        return !_writeQueue.empty();
    }
#endif

    IoContextTcpSocket _socket;

    boost::asio::ip::address _remoteAddress;
    uint16 _remotePort;

    MessageBuffer _readBuffer;
    std::queue<MessageBuffer> _writeQueue;

    std::atomic<SocketState> _state;

    bool _isWritingAsync;
};

#endif
