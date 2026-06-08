#include "Opcodes.h"
#include "Player.h"
#include "WorldSession.h"

void WorldSession::HandleGrantLevel(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: CMSG_GRANT_LEVEL");

    ObjectGuid guid;
    recvData >> guid.ReadAsPacked();

    WorldPacket data(SMSG_REFER_A_FRIEND_FAILURE, 24);
    data << static_cast<uint32>(ERR_REFER_A_FRIEND_NOT_REFERRED_BY);
    SendPacket(&data);
}

void WorldSession::HandleAcceptGrantLevel(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: CMSG_ACCEPT_LEVEL_GRANT");

    ObjectGuid guid;
    recvData >> guid.ReadAsPacked();
}
