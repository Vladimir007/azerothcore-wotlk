#include "AuthSession.h"

#include <boost/lexical_cast.hpp>

#include "AES.h"
#include "AuthCodes.h"
#include "Common.h"
#include "CryptoHash.h"
#include "CryptoRandom.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "RealmList.h"
#include "StringConvert.h"
#include "Util.h"

enum eAuthCmd
{
    AUTH_LOGON_CHALLENGE = 0x00,
    AUTH_LOGON_PROOF = 0x01,
    AUTH_RECONNECT_CHALLENGE = 0x02,
    AUTH_RECONNECT_PROOF = 0x03,
    REALM_LIST = 0x10
};

#pragma pack(push, 1)

typedef struct AUTH_LOGON_CHALLENGE_C // NOLINT(*-pro-type-member-init)
{
    [[maybe_unused]] uint8 cmd;
    [[maybe_unused]] uint8 error;
    uint16 size;
    [[maybe_unused]] uint8 gameName[4];
    [[maybe_unused]] uint8 version1;
    [[maybe_unused]] uint8 version2;
    [[maybe_unused]] uint8 version3;
    uint16 build;
    [[maybe_unused]] uint8 platform[4];
    [[maybe_unused]] uint8 os[4];
    uint8   country[4];
    [[maybe_unused]] uint32 timezone_bias;
    [[maybe_unused]] uint32 ip;
    uint8 I_len;
    uint8 I[1];
} sAuthLogonChallenge_C;
static_assert(sizeof(sAuthLogonChallenge_C) == 1 + 1 + 2 + 4 + 1 + 1 + 1 + 2 + 4 + 4 + 4 + 4 + 4 + 1 + 1);

typedef struct AUTH_LOGON_PROOF_C // NOLINT(*-pro-type-member-init)
{
    [[maybe_unused]] uint8 cmd;
    Acore::Crypto::SRP6::EphemeralKey A;
    Acore::Crypto::SHA1::Digest clientM;
    [[maybe_unused]] Acore::Crypto::SHA1::Digest crc_hash;
    [[maybe_unused]] uint8 number_of_keys;
    uint8 securityFlags;
} sAuthLogonProof_C;
static_assert(sizeof(sAuthLogonProof_C) == (1 + 32 + 20 + 20 + 1 + 1));

typedef struct AUTH_LOGON_PROOF_S
{
    uint8   cmd;
    uint8   error;
    Acore::Crypto::SHA1::Digest M2;
    uint32  AccountFlags;
    uint32  SurveyId;
    uint16  LoginFlags;
} sAuthLogonProof_S;
static_assert(sizeof(sAuthLogonProof_S) == 1 + 1 + 20 + 4 + 4 + 2);

typedef struct AUTH_RECONNECT_PROOF_C // NOLINT(*-pro-type-member-init)
{
    [[maybe_unused]] uint8 cmd;
    uint8 R1[16];
    Acore::Crypto::SHA1::Digest R2;
    [[maybe_unused]] Acore::Crypto::SHA1::Digest R3;
    [[maybe_unused]] uint8 number_of_keys;
} sAuthReconnectProof_C;
static_assert(sizeof(sAuthReconnectProof_C) == 1 + 16 + 20 + 20 + 1);

#pragma pack(pop)

std::array<uint8, 16> VersionChallenge = { { 0xBA, 0xA3, 0x1E, 0x99, 0xA0, 0x0B, 0x21, 0x57, 0xFC, 0x37, 0x3F, 0xB3, 0x69, 0xCD, 0xD2, 0xF1 } };

#define MAX_ACCEPTED_CHALLENGE_SIZE (sizeof(AUTH_LOGON_CHALLENGE_C) + 16)

#define AUTH_LOGON_CHALLENGE_INITIAL_SIZE 4
#define REALM_LIST_PACKET_SIZE 5

std::unordered_map<uint8, AuthHandler> AuthSession::InitHandlers()
{
    std::unordered_map<uint8, AuthHandler> handlers;

    handlers[AUTH_LOGON_CHALLENGE] =     { STATUS_CHALLENGE,       AUTH_LOGON_CHALLENGE_INITIAL_SIZE, &AuthSession::HandleLogonChallenge };
    handlers[AUTH_LOGON_PROOF] =         { STATUS_LOGON_PROOF,     sizeof(AUTH_LOGON_PROOF_C),        &AuthSession::HandleLogonProof };
    handlers[AUTH_RECONNECT_CHALLENGE] = { STATUS_CHALLENGE,       AUTH_LOGON_CHALLENGE_INITIAL_SIZE, &AuthSession::HandleReconnectChallenge };
    handlers[AUTH_RECONNECT_PROOF] =     { STATUS_RECONNECT_PROOF, sizeof(AUTH_RECONNECT_PROOF_C),    &AuthSession::HandleReconnectProof };
    handlers[REALM_LIST] =               { STATUS_AUTHED,          REALM_LIST_PACKET_SIZE,            &AuthSession::HandleRealmList };

    return handlers;
}

std::unordered_map<uint8, AuthHandler> const Handlers = AuthSession::InitHandlers();

void AccountInfo::LoadResult(const Field* fields)
{
    Id = fields[0].Get<uint32>();
    Login = fields[1].Get<std::string>();
    IsActive = fields[2].Get<bool>();
    IsStaff = fields[3].Get<bool>();
    IsSuperuser = fields[4].Get<bool>();
}

AuthSession::AuthSession(IoContextTcpSocket&& socket): Socket(std::move(socket)), _status(STATUS_CHALLENGE), _build(0) { }

void AuthSession::Start()
{
    AsyncRead();
}

bool AuthSession::Update()
{
    if (!AuthSocket::Update())
        return false;

    _queryProcessor.ProcessReadyCallbacks();

    return true;
}

SocketReadCallbackResult AuthSession::ReadHandler()
{
    MessageBuffer& packet = GetReadBuffer();

    while (packet.GetActiveSize())
    {
        uint8 cmd = packet.GetReadPointer()[0];
        auto itr = Handlers.find(cmd);
        if (itr == Handlers.end())
        {
            // Well we don't handle this, lets just ignore it
            packet.Reset();
            break;
        }

        if (_status != itr->second.status)
        {
            CloseSocket();
            return SocketReadCallbackResult::Stop;
        }

        auto size = static_cast<uint16>(itr->second.packetSize);
        if (packet.GetActiveSize() < size)
            break;

        if (cmd == AUTH_LOGON_CHALLENGE || cmd == AUTH_RECONNECT_CHALLENGE)
        {
            const auto* challenge = reinterpret_cast<sAuthLogonChallenge_C*>(packet.GetReadPointer());
            size += challenge->size;
            if (size > MAX_ACCEPTED_CHALLENGE_SIZE)
            {
                CloseSocket();
                return SocketReadCallbackResult::Stop;
            }
        }

        if (packet.GetActiveSize() < size)
            break;

        if (!(this->*itr->second.handler)())
        {
            CloseSocket();
            return SocketReadCallbackResult::Stop;
        }

        packet.ReadCompleted(size);
    }

    return SocketReadCallbackResult::KeepReading;
}

void AuthSession::SendPacket(ByteBuffer& packet)
{
    if (!IsOpen())
        return;

    if (!packet.empty())
    {
        MessageBuffer buffer(packet.size());
        buffer.Write(packet.contents(), packet.size());
        QueuePacket(std::move(buffer));
    }
}

bool AuthSession::HandleLogonChallenge()
{
    _status = STATUS_CLOSED;

    const auto* challenge = reinterpret_cast<sAuthLogonChallenge_C*>(GetReadBuffer().GetReadPointer());
    if (challenge->size - (sizeof(sAuthLogonChallenge_C) - AUTH_LOGON_CHALLENGE_INITIAL_SIZE - 1) != challenge->I_len)
        return false;

    std::string login(reinterpret_cast<char const*>(challenge->I), challenge->I_len);
    LOG_DEBUG("server.authserver", "[AuthChallenge] '{}'", login);

    _build = challenge->build;

    _localizationName.resize(4);
    for (int i = 0; i < 4; ++i)
        _localizationName[i] = challenge->country[4 - i - 1];

    // Get the account details from the account table
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_LOGON_CHALLENGE);
    stmt->SetData(0, login);

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(
        std::bind(&AuthSession::LogonChallengeCallback, this, std::placeholders::_1)
    ));
    return true;
}

void AuthSession::LogonChallengeCallback(const QueryResult& result)
{
    ByteBuffer pkt;
    pkt << static_cast<uint8>(AUTH_LOGON_CHALLENGE);
    pkt << static_cast<uint8>(0);

    if (!result)
    {
        pkt << static_cast<uint8>(WOW_FAIL_UNKNOWN_ACCOUNT);
        SendPacket(pkt);
        return;
    }

    const Field* fields = result->Fetch();

    _accountInfo.LoadResult(fields);

    _srp6.emplace(_accountInfo.Login,
        fields[5].Get<Binary, Acore::Crypto::SRP6::SALT_LENGTH>(),
        fields[6].Get<Binary, Acore::Crypto::SRP6::VERIFIER_LENGTH>());

    if (_build != ACCEPTED_CLIENT_BUILD)
    {
        pkt << static_cast<uint8>(WOW_FAIL_VERSION_INVALID);
        SendPacket(pkt);
        return;
    }

    pkt << static_cast<uint8>(WOW_SUCCESS);

    pkt.append(_srp6->B);
    pkt << static_cast<uint8>(1);
    pkt.append(_srp6->g);
    pkt << static_cast<uint8>(32);
    pkt.append(_srp6->N);
    pkt.append(_srp6->s);
    pkt.append(VersionChallenge.data(), VersionChallenge.size());
    pkt << static_cast<uint8>(0); // Security flags

    LOG_DEBUG(
        "server.authserver", "'{}:{}' [AuthChallenge] account {} is using '{}' locale ({})",
        GetRemoteIpAddress().to_string(), GetRemotePort(), _accountInfo.Login,
        _localizationName, GetLocaleByName(_localizationName)
    );

    _status = STATUS_LOGON_PROOF;
    SendPacket(pkt);
}

// Logon Proof command handler
bool AuthSession::HandleLogonProof()
{
    LOG_DEBUG("server.authserver", "Entering _HandleLogonProof");
    _status = STATUS_CLOSED;

    // Read the packet
    const auto* logonProof = reinterpret_cast<sAuthLogonProof_C*>(GetReadBuffer().GetReadPointer());

    // Check if SRP6 results match (password is correct), else send an error
    if (const Optional<SessionKey> K = _srp6->VerifyChallengeResponse(logonProof->A, logonProof->clientM))
    {
        _sessionKey = *K;
        if (logonProof->securityFlags & 0x04)
        {
            ByteBuffer packet;
            packet << static_cast<uint8>(AUTH_LOGON_PROOF);
            packet << static_cast<uint8>(WOW_FAIL_UNKNOWN_ACCOUNT);
            packet << static_cast<uint16>(0); // LoginFlags
            SendPacket(packet);
            return true;
        }

        LOG_DEBUG("server.authserver", "'{}:{}' User '{}' successfully authenticated",
            GetRemoteIpAddress().to_string(), GetRemotePort(), _accountInfo.Login);

        // Update the SessionKey, last_ip, last login time and reset number of failed logins in the account table for this account
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LOGON_PROOF);
        stmt->SetData(0, _sessionKey);
        stmt->SetData(1, GetLocaleByName(_localizationName));
        stmt->SetData(2, _accountInfo.Login);
        _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(
            [this, M2 = Acore::Crypto::SRP6::GetSessionVerifier(logonProof->A, logonProof->clientM, _sessionKey)](QueryResult const&)
            {
                // Finish SRP6 and send the final result to the client
                sAuthLogonProof_S proof;
                proof.M2 = M2;
                proof.cmd = AUTH_LOGON_PROOF;
                proof.error = 0;
                proof.AccountFlags = _accountInfo.IsStaff ? ACCOUNT_FLAG_GM : 0;
                proof.SurveyId = 0;
                proof.LoginFlags = 0;

                ByteBuffer packet;
                packet.resize(sizeof(proof));
                std::memcpy(packet.contents(), &proof, sizeof(proof));

                SendPacket(packet);
                _status = STATUS_AUTHED;
            }
        ));
    }
    else
    {
        ByteBuffer packet;
        packet << static_cast<uint8>(AUTH_LOGON_PROOF);
        packet << static_cast<uint8>(WOW_FAIL_UNKNOWN_ACCOUNT);
        packet << static_cast<uint16>(0); // LoginFlags
        SendPacket(packet);

        LOG_INFO("server.authserver.hack", "'{}:{}' [AuthChallenge] account {} tried to login with invalid password!",
            GetRemoteIpAddress().to_string(), GetRemotePort(), _accountInfo.Login);
    }

    return true;
}

bool AuthSession::HandleReconnectChallenge()
{
    _status = STATUS_CLOSED;

    const auto* challenge = reinterpret_cast<sAuthLogonChallenge_C*>(GetReadBuffer().GetReadPointer());
    if (challenge->size - (sizeof(sAuthLogonChallenge_C) - AUTH_LOGON_CHALLENGE_INITIAL_SIZE - 1) != challenge->I_len)
        return false;

    std::string login(reinterpret_cast<char const*>(challenge->I), challenge->I_len);
    LOG_DEBUG("server.authserver", "[ReconnectChallenge] '{}'", login);

    _build = challenge->build;
    if (_build != ACCEPTED_CLIENT_BUILD)
    {
        ByteBuffer pkt;
        pkt << static_cast<uint8>(AUTH_RECONNECT_CHALLENGE);
        pkt << static_cast<uint8>(WOW_FAIL_VERSION_INVALID);
        SendPacket(pkt);
        return false;
    }

    _localizationName.resize(4);
    for (int i = 0; i < 4; ++i)
        _localizationName[i] = challenge->country[4 - i - 1];

    // Get the account details from the account table
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_RECONNECT_CHALLENGE);
    stmt->SetData(0, login);

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(
        std::bind(&AuthSession::ReconnectChallengeCallback, this, std::placeholders::_1)
    ));
    return true;
}

void AuthSession::ReconnectChallengeCallback(const QueryResult& result)
{
    ByteBuffer pkt;
    pkt << static_cast<uint8>(AUTH_RECONNECT_CHALLENGE);

    if (!result)
    {
        pkt << static_cast<uint8>(WOW_FAIL_UNKNOWN_ACCOUNT);
        SendPacket(pkt);
        return;
    }

    const Field* fields = result->Fetch();

    _accountInfo.LoadResult(fields);
    _sessionKey = fields[5].Get<Binary, SESSION_KEY_LENGTH>();
    Acore::Crypto::GetRandomBytes(_reconnectProof);
    _status = STATUS_RECONNECT_PROOF;

    pkt << static_cast<uint8>(WOW_SUCCESS);
    pkt.append(_reconnectProof);
    pkt.append(VersionChallenge.data(), VersionChallenge.size());

    SendPacket(pkt);
}

bool AuthSession::HandleReconnectProof()
{
    LOG_DEBUG("server.authserver", "Entering _HandleReconnectProof");
    _status = STATUS_CLOSED;

    const auto* reconnectProof = reinterpret_cast<sAuthReconnectProof_C*>(GetReadBuffer().GetReadPointer());

    if (_accountInfo.Login.empty())
        return false;

    Acore::Crypto::SHA1 sha;
    sha.UpdateData(_accountInfo.Login);
    sha.UpdateData(reconnectProof->R1, 16);
    sha.UpdateData(_reconnectProof);
    sha.UpdateData(_sessionKey);
    sha.Finalize();

    if (sha.GetDigest() == reconnectProof->R2)
    {
        // Sending response
        ByteBuffer pkt;
        pkt << static_cast<uint8>(AUTH_RECONNECT_PROOF);
        pkt << static_cast<uint8>(WOW_SUCCESS);
        pkt << static_cast<uint16>(0); // LoginFlags
        SendPacket(pkt);
        _status = STATUS_AUTHED;
        return true;
    }
    LOG_ERROR("server.authserver.hack", "'{}:{}' [ERROR] user {} tried to login, but session is invalid.",
        GetRemoteIpAddress().to_string(), GetRemotePort(), _accountInfo.Login);
    return false;
}

bool AuthSession::HandleRealmList()
{
    LOG_DEBUG("server.authserver", "Entering _HandleRealmList");

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_CHARACTERS_COUNT);
    stmt->SetData(0, _accountInfo.Id);

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(
        std::bind(&AuthSession::RealmListCallback, this, std::placeholders::_1)
    ));
    _status = STATUS_WAITING_FOR_REALM_LIST;
    return true;
}

void AuthSession::RealmListCallback(const QueryResult& result)
{
    uint8 characterCount = 0;
    if (result)
        characterCount = (*result)[0].Get<uint8>();

    // Circle through realms in the RealmList and construct the return packet (including # of user characters in each realm)
    ByteBuffer pkt;

    std::size_t realmListSize = 0;
    if (const auto realm = sRealmList->GetRealm())
    {
        pkt << static_cast<uint8>(0); // Realm Type: Normal
        pkt << static_cast<uint8>(0); // If 1, then realm locked
        pkt << static_cast<uint8>(realm->Flags);
        pkt << realm->Name;
        pkt << boost::lexical_cast<std::string>(realm->GetAddressForClient(GetRemoteIpAddress()));
        pkt << 0.0f; // Population level
        pkt << characterCount;
        pkt << realm->Timezone; // Realm category
        pkt << static_cast<uint8>(realm->ID);

        ++realmListSize;
    }

    pkt << static_cast<uint8>(0x10);
    pkt << static_cast<uint8>(0x00);

    // Make a ByteBuffer which stores the RealmList's size
    ByteBuffer RealmListSizeBuffer;
    RealmListSizeBuffer << static_cast<uint32>(0);
    RealmListSizeBuffer << static_cast<uint16>(realmListSize);

    ByteBuffer hdr;
    hdr << static_cast<uint8>(REALM_LIST);
    hdr << static_cast<uint16>(pkt.size() + RealmListSizeBuffer.size());
    hdr.append(RealmListSizeBuffer); // Append RealmList's size buffer
    hdr.append(pkt);                 // Append realms in the RealmList
    SendPacket(hdr);

    _status = STATUS_AUTHED;
}
