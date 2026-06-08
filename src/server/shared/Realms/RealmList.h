#ifndef REALMLIST_H
#define REALMLIST_H

#include <memory> // NOTE: this import is NEEDED (even though some IDEs report it as unused)
#include <boost/asio/steady_timer.hpp>

#include "Define.h"
#include "Realm.h"

namespace Acore::Asio
{
    class IoContext;
}

namespace boost::system
{
    class error_code;
}

class RealmList
{
public:
    static RealmList* Instance();

    void Initialize(Acore::Asio::IoContext& ioContext, uint32 updateInterval);
    void Close() const;

    [[nodiscard]] const Realm* GetRealm() const { return _realm.get(); }

private:
    RealmList();
    ~RealmList() = default;

    void UpdateRealmLoop(boost::system::error_code const& error);
    bool UpdateRealm(const uint32& realmID, std::string const& name,
        const std::string& externalAddrStr, const std::string& localAddrStr, const std::string& localSubMaskStr,
        uint16 port, RealmFlags flag, uint8 realmTimezone);

    std::unique_ptr<Realm> _realm{nullptr};
    uint32 _updateInterval{0};
    std::unique_ptr<boost::asio::steady_timer> _updateTimer;
    std::unique_ptr<Acore::Asio::Resolver> _resolver;
};

#define sRealmList RealmList::Instance()

#endif
