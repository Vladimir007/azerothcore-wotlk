#ifndef AUTH_SOCKET_MGR_H
#define AUTH_SOCKET_MGR_H

#include "AuthSession.h"
#include "SocketMgr.h"

class AuthSocketMgr : public SocketMgr<AuthSession>
{
public:
    static AuthSocketMgr& Instance()
    {
        static AuthSocketMgr instance;
        return instance;
    }

    bool StartNetwork(Acore::Asio::IoContext& ioContext, std::string const& bindIp, const uint16 port) override
    {
        if (!SocketMgr::StartNetwork(ioContext, bindIp, port))
            return false;
        _acceptor->AsyncAcceptWithCallback<&AuthSocketMgr::OnSocketAccept>();
        return true;
    }

protected:
    [[nodiscard]] NetworkThread<AuthSession>* CreateThread() const override
    {
        return new NetworkThread<AuthSession>();
    }

    static void OnSocketAccept(IoContextTcpSocket& sock)
    {
        Instance().OnSocketOpen(sock);
    }
};

#define sAuthSocketMgr AuthSocketMgr::Instance()

#endif
