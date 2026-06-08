#include "RealmList.h"

#include <memory>
#include <boost/asio/ip/tcp.hpp>

#include "DatabaseEnv.h"
#include "Log.h"
#include "QueryResult.h"
#include "Resolver.h"
#include "SteadyTimer.h"
#include "Util.h"

using boost::asio::ip::tcp;

RealmList::RealmList() { }

RealmList* RealmList::Instance()
{
    static RealmList instance;
    return &instance;
}

// Load the realm list from the database
void RealmList::Initialize(Acore::Asio::IoContext& ioContext, const uint32 updateInterval)
{
    _updateInterval = updateInterval;
    _updateTimer = std::make_unique<boost::asio::steady_timer>(ioContext);
    _resolver = std::make_unique<Acore::Asio::Resolver>(ioContext);

    // Get the content of the realm table in the database
    UpdateRealmLoop(boost::system::error_code());
}

void RealmList::Close() const
{
    _updateTimer->cancel();
}

bool RealmList::UpdateRealm(const uint32& realmID, std::string const& name,
    const std::string& externalAddrStr, const std::string& localAddrStr, const std::string& localSubMaskStr,
    const uint16 port, const RealmFlags flag, const uint8 realmTimezone)
{
    Optional<tcp::endpoint> externalAddress = _resolver->Resolve(tcp::v4(), externalAddrStr, "");
    if (!externalAddress)
    {
        LOG_ERROR("server.authserver", "Could not resolve address {} for realm \"{}\"", externalAddrStr, name);
        return false;
    }
    Optional<tcp::endpoint> localAddress = _resolver->Resolve(tcp::v4(), localAddrStr, "");
    if (!localAddress)
    {
        LOG_ERROR("server.authserver", "Could not resolve localAddress {} for realm \"{}\"", localAddrStr, name);
        return false;
    }

    Optional<tcp::endpoint> localSubMask = _resolver->Resolve(tcp::v4(), localSubMaskStr, "");
    if (!localSubMask)
    {
        LOG_ERROR("server.authserver", "Could not resolve localSubnetMask {} for realm \"{}\"", localSubMaskStr, name);
        return false;
    }

    if (!_realm)
    {
        LOG_DEBUG("server.authserver", "Updating realm \"{}\" at {}:{}.", name, externalAddrStr, port);
    }
    else
    {
        LOG_INFO("server.authserver", "Added realm \"{}\" at {}:{}.", name, externalAddrStr, port);
        _realm = std::make_unique<Realm>();
    }

    _realm->ID = realmID;
    _realm->Name = name;
    _realm->Flags = flag;
    _realm->Timezone = realmTimezone;

    if (boost::asio::ip::address addr = externalAddress->address(); !_realm->ExternalAddress || *_realm->ExternalAddress != addr)
        _realm->ExternalAddress = std::make_unique<boost::asio::ip::address>(std::move(addr));

    if (boost::asio::ip::address addr = localAddress->address(); !_realm->LocalAddress || *_realm->LocalAddress != addr)
        _realm->LocalAddress = std::make_unique<boost::asio::ip::address>(std::move(addr));

    if (boost::asio::ip::address addr = localSubMask->address(); !_realm->LocalSubnetMask || *_realm->LocalSubnetMask != addr)
        _realm->LocalSubnetMask = std::make_unique<boost::asio::ip::address>(std::move(addr));

    _realm->Port = port;
    return true;
}

void RealmList::UpdateRealmLoop(const boost::system::error_code& error)
{
    if (error)
        return;  // Skip update if have errors

    LOG_DEBUG("server.authserver", "Updating Realm...");

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALM_INFO);

    bool success = false;
    if (const QueryResult result = LoginDatabase.Query(stmt))
    {
        const auto fields = result->Fetch();
        try
        {
            success = UpdateRealm(
                fields[0].Get<uint32>(),
                fields[1].Get<std::string>(),
                fields[2].Get<std::string>(),
                fields[3].Get<std::string>(),
                fields[4].Get<std::string>(),
                fields[5].Get<uint16>(),
                static_cast<RealmFlags>(fields[6].Get<uint32>()),
                fields[7].Get<uint8>());
        }
        catch (const std::exception& exc)
        {
            LOG_ERROR("server.authserver", "RealmList::UpdateRealm has thrown an exception: {}", exc.what());
            ABORT();
        }
    }

    if (!success && _realm)
    {
        LOG_INFO("server.authserver", "Removed realm \"{}\".", _realm->Name);
        _realm.reset();
    }

    if (_updateInterval)
    {
        _updateTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(_updateInterval));
        _updateTimer->async_wait([this](const boost::system::error_code& errorCode){ UpdateRealmLoop(errorCode); });
    }
}
