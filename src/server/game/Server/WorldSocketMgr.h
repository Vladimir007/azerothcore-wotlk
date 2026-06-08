#ifndef WORLD_SOCKET_MGR_H
#define WORLD_SOCKET_MGR_H

#include <boost/system/error_code.hpp>
#include "NetworkThread.h"
#include "ScriptMgr.h"
#include "SocketMgr.h"
#include "WorldSocket.h"

class WorldSocket;

class WorldSocketThread : public NetworkThread<WorldSocket>
{
protected:
    void SocketAdded(std::shared_ptr<WorldSocket> const& sock) override
    {
        sScriptMgr->OnSocketOpen(sock);
    }

    void SocketRemoved(std::shared_ptr<WorldSocket> const& sock) override
    {
        sScriptMgr->OnSocketClose(sock);
    }
};

class WorldSocketMgr : public SocketMgr<WorldSocket>
{
    typedef SocketMgr BaseSocketMgr;

public:
    static WorldSocketMgr& Instance()
    {
        static WorldSocketMgr instance;
        return instance;
    }

    bool StartNetwork(Acore::Asio::IoContext& ioContext, std::string const& bindIp, uint16 port) override
    {
        if (!BaseSocketMgr::StartNetwork(ioContext, bindIp, port))
            return false;
        _acceptor->AsyncAcceptWithCallback<&WorldSocketMgr::OnSocketAccept>();
        sScriptMgr->OnNetworkStart();
        return true;
    }

    void StopNetwork() override
    {
        BaseSocketMgr::StopNetwork();
        sScriptMgr->OnNetworkStop();
    }

    void OnSocketOpen(IoContextTcpSocket& sock) override
    {
        // Set TCP_NODELAY.
        boost::system::error_code err;
        sock.set_option(tcp::no_delay(true), err);

        if (err)
        {
            LOG_ERROR("network", "WorldSocketMgr::OnSocketOpen sock.set_option(boost::asio::ip::tcp::no_delay) err = {}", err.message());
            return;
        }

        BaseSocketMgr::OnSocketOpen(sock);
    }

protected:
    [[nodiscard]] NetworkThread<WorldSocket>* CreateThread() const override
    {
        return new WorldSocketThread();
    }

    static void OnSocketAccept(IoContextTcpSocket& sock)
    {
        Instance().OnSocketOpen(sock);
    }
};

#define sWorldSocketMgr WorldSocketMgr::Instance()

#endif
