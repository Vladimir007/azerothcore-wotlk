#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "AsyncCallbackProcessor.h"
#include "BigNumber.h"
#include "ByteBuffer.h"
#include "Optional.h"
#include "QueryResult.h"
#include "SRP6.h"
#include "Socket.h"
#include <boost/asio/ip/tcp.hpp>

using boost::asio::ip::tcp;

class Field;
struct AuthHandler;

enum AuthStatus
{
    STATUS_CHALLENGE = 0,
    STATUS_LOGON_PROOF,
    STATUS_RECONNECT_PROOF,
    STATUS_AUTHED,
    STATUS_WAITING_FOR_REALM_LIST,
    STATUS_CLOSED
};

struct AccountInfo
{
    void LoadResult(const Field* fields);

    uint32 Id = 0;
    std::string Login;
    bool IsActive = false;
    bool IsGameMaster = false;
};

class AuthSession final : public Socket<AuthSession>
{
    typedef Socket AuthSocket;

public:
    static std::unordered_map<uint8, AuthHandler> InitHandlers();

    AuthSession(IoContextTcpSocket&& socket);

    void Start() override;
    bool Update() override;

    void SendPacket(ByteBuffer& packet);

protected:
    SocketReadCallbackResult ReadHandler() override;

private:
    bool HandleLogonChallenge();
    bool HandleLogonProof();
    bool HandleReconnectChallenge();
    bool HandleReconnectProof();
    bool HandleRealmList();

    void LogonChallengeCallback(const QueryResult& result);
    void ReconnectChallengeCallback(const QueryResult& result);
    void RealmListCallback(const QueryResult& result);

    Optional<Acore::Crypto::SRP6> _srp6 = {};
    SessionKey _sessionKey = {};
    std::array<uint8, 16> _reconnectProof = {};

    AuthStatus _status;
    AccountInfo _accountInfo;
    std::string _localizationName;
    uint16 _build;

    QueryCallbackProcessor _queryProcessor;
};

#pragma pack(push, 1)
struct AuthHandler
{
    AuthStatus status;
    std::size_t packetSize;
    bool (AuthSession::*handler)();
};
#pragma pack(pop)

#endif
