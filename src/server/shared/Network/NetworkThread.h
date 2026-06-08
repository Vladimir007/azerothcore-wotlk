#ifndef NETWORK_THREAD_H
#define NETWORK_THREAD_H

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "Define.h"
#include "Errors.h"
#include "IoContext.h"
#include "Log.h"
#include "Socket.h"

using boost::asio::ip::tcp;

template<class SocketType>
class NetworkThread
{
public:
    NetworkThread(): _ioContext(1), _acceptSocket(_ioContext), _updateTimer(_ioContext) { }

    virtual ~NetworkThread()
    {
        Stop();
        if (_thread)
            Wait();
    }

    void Stop()
    {
        _stopped = true;
        _ioContext.stop();
    }

    bool Start()
    {
        if (_thread)
            return false;

        _thread = std::make_unique<std::thread>([this] { Run(); });
        return true;
    }

    void Wait()
    {
        ASSERT(_thread);

        if (_thread->joinable())
            _thread->join();

        _thread.reset();
    }

    [[nodiscard]] int32 GetConnectionCount() const
    {
        return _connections;
    }

    virtual void AddSocket(std::shared_ptr<SocketType> sock)
    {
        std::lock_guard lock(_newSocketsLock);

        ++_connections;
        _newSockets.emplace_back(sock);
        SocketAdded(sock);
    }

    IoContextTcpSocket* GetSocketForAccept() { return &_acceptSocket; }

protected:
    virtual void SocketAdded(std::shared_ptr<SocketType> const& /*sock*/) { }
    virtual void SocketRemoved(std::shared_ptr<SocketType> const& /*sock*/) { }

    void AddNewSockets()
    {
        std::lock_guard lock(_newSocketsLock);

        if (_newSockets.empty())
            return;

        for (std::shared_ptr<SocketType> sock: _newSockets)
        {
            if (!sock->IsOpen())
            {
                SocketRemoved(sock);
                --_connections;
                continue;
            }

            _sockets.emplace_back(sock);
            sock->Start();
        }
        _newSockets.clear();
    }

    void Run()
    {
        LOG_DEBUG("misc", "Network Thread Starting");

        _updateTimer.expires_at(std::chrono::steady_clock::now() + std::chrono::milliseconds(1));
        _updateTimer.async_wait([this](boost::system::error_code const&) { Update(); });
        _ioContext.run();

        LOG_DEBUG("misc", "Network Thread exits");
        _newSockets.clear();
        _sockets.clear();
    }

    void Update()
    {
        if (_stopped)
            return;

        _updateTimer.expires_at(std::chrono::steady_clock::now() + std::chrono::milliseconds(1));
        _updateTimer.async_wait([this](boost::system::error_code const&) { Update(); });

        AddNewSockets();

        _sockets.erase(std::remove_if(_sockets.begin(), _sockets.end(), [this](std::shared_ptr<SocketType> sock)
        {
            if (!sock->Update())
            {
                if (sock->IsOpen())
                    sock->CloseSocket();

                this->SocketRemoved(sock);

                --this->_connections;
                return true;
            }

            return false;
        }), _sockets.end());
    }

private:
    using SocketContainer = std::vector<std::shared_ptr<SocketType>>;

    std::atomic<int32> _connections{};
    std::atomic<bool> _stopped{};

    std::unique_ptr<std::thread> _thread = nullptr;

    SocketContainer _sockets{};

    std::mutex _newSocketsLock;
    SocketContainer _newSockets{};

    Acore::Asio::IoContext _ioContext;
    IoContextTcpSocket _acceptSocket;
    boost::asio::steady_timer _updateTimer;
};

#endif
