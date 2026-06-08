#ifndef SOCKET_MGR_H
#define SOCKET_MGR_H

#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include "AsyncAcceptor.h"
#include "Config.h"
#include "Errors.h"
#include "NetworkThread.h"

using boost::asio::ip::tcp;

template<class SocketType>
class SocketMgr
{
public:
    virtual ~SocketMgr()
    {
        ASSERT(!_thread && !_acceptor, "StopNetwork must be called prior to SocketMgr destruction");
    }

    virtual bool StartNetwork(Acore::Asio::IoContext& ioContext, std::string const& bindIp, uint16 port)
    {
        std::unique_ptr<AsyncAcceptor> acceptor;
        try
        {
            acceptor = std::make_unique<AsyncAcceptor>(ioContext, bindIp, port);
        }
        catch (const boost::system::system_error& err)
        {
            LOG_ERROR("network", "Exception caught in SocketMgr.StartNetwork ({}:{}): {}", bindIp, port, err.what());
            return false;
        }

        if (!acceptor->Bind())
        {
            LOG_ERROR("network", "StartNetwork failed to bind socket acceptor");
            return false;
        }

        _acceptor = std::move(acceptor);
        _thread = std::unique_ptr<NetworkThread<SocketType>>(CreateThread());

        ASSERT(_thread);
        _thread->Start();

        _acceptor->SetSocketFactory([this] { return this->_thread->GetSocketForAccept(); });
        return true;
    }

    virtual void StopNetwork()
    {
        _acceptor->Close();
        _thread->Stop();

        Wait();

        _acceptor.reset();
        _thread.reset();
    }

    void Wait()
    {
        _thread->Wait();
    }

    virtual void OnSocketOpen(IoContextTcpSocket& sock)
    {
        try
        {
            std::shared_ptr<SocketType> newSocket = std::make_shared<SocketType>(std::move(sock));
            _thread->AddSocket(newSocket);
        }
        catch (const boost::system::system_error& err)
        {
            LOG_WARN("network", "Failed to retrieve client's remote address {}", err.what());
        }
    }

protected:
    SocketMgr() = default;

    virtual NetworkThread<SocketType>* CreateThread() const = 0;

    std::unique_ptr<AsyncAcceptor> _acceptor{};
    std::unique_ptr<NetworkThread<SocketType>> _thread{};
};

#endif
