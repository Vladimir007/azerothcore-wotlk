/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "GameTime.h"
#include "Log.h"
#include "MapMgr.h"
#include "NPCHandler.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Pet.h"
#include "Player.h"
#include "QueryPackets.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::SendNameQueryOpcode(ObjectGuid guid)
{
    CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(guid);

    WorldPackets::Query::NameQueryResponse nameQueryResponse;
    nameQueryResponse.Guid = guid.WriteAsPacked();
    if (!playerData)
    {
        nameQueryResponse.NameUnknown = true;
        SendPacket(nameQueryResponse.Write());
        return;
    }

    Player* player = ObjectAccessor::FindConnectedPlayer(guid);

    nameQueryResponse.NameUnknown = false;
    nameQueryResponse.Name = playerData->Name;
    nameQueryResponse.Race = player ? player->getRace() : playerData->Race;
    nameQueryResponse.Sex = player ? player->getGender() : playerData->Sex;
    nameQueryResponse.Class = player ? player->getClass() : playerData->Class;

    if (DeclinedName const* names = (player ? player->GetDeclinedNames() : nullptr))
    {
        nameQueryResponse.Declined = true;
        nameQueryResponse.DeclinedNames = *names;
    }
    else
        nameQueryResponse.Declined = false;

    SendPacket(nameQueryResponse.Write());
}

void WorldSession::HandleNameQueryOpcode(WorldPackets::Query::NameQuery& packet)
{
    // This is disable by default to prevent lots of console spam
    // LOG_INFO("network.opcode", "HandleNameQueryOpcode {}", guid);

    SendNameQueryOpcode(packet.Guid);
}

void WorldSession::HandleTimeQueryOpcode(WorldPackets::Query::TimeQuery& /*packet*/)
{
    SendTimeQueryResponse();
}

void WorldSession::SendTimeQueryResponse()
{
    auto timeResponse = sWorld->GetNextDailyQuestsResetTime() - GameTime::GetGameTime();

    WorldPackets::Query::TimeQueryResponse timeQueryResponse;
    timeQueryResponse.ServerTime = GameTime::GetGameTime().count();
    timeQueryResponse.TimeResponse = timeResponse.count();
    SendPacket(timeQueryResponse.Write());
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleCreatureQueryOpcode(WorldPacket& recvData)
{
    uint32 entry;
    recvData >> entry;
    ObjectGuid guid;
    recvData >> guid;

    CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(entry);
    if (ci)
    {
        // guess size
        WorldPacket data(SMSG_CREATURE_QUERY_RESPONSE, 100);
        data << uint32(entry);                                       // creature entry
        data << ci->Name;
        data << uint8(0) << uint8(0) << uint8(0);                    // name2, name3, name4, always empty
        data << ci->SubName;
        data << ci->IconName;                                        // "Directions" for guard, string for Icons 2.3.0
        data << uint32(ci->TypeFlags);                              // flags
        data << uint32(ci->type);                                    // CreatureType.dbc
        data << uint32(ci->Family);                                  // CreatureFamily.dbc
        data << uint32(ci->Rank);                                    // Creature Rank (elite, boss, etc)
        data << uint32(ci->KillCredit[0]);                           // new in 3.1, kill credit
        data << uint32(ci->KillCredit[1]);                           // new in 3.1, kill credit
        if (ci->GetModelByIdx(0))
            data << uint32(ci->GetModelByIdx(0)->CreatureDisplayID); // Modelid1
        else
            data << uint32(0);                                       // Modelid1
        if (ci->GetModelByIdx(1))
            data << uint32(ci->GetModelByIdx(1)->CreatureDisplayID); // Modelid2
        else
            data << uint32(0);                                       // Modelid2
        if (ci->GetModelByIdx(2))
            data << uint32(ci->GetModelByIdx(2)->CreatureDisplayID); // Modelid3
        else
            data << uint32(0);                                       // Modelid3
        if (ci->GetModelByIdx(3))
            data << uint32(ci->GetModelByIdx(3)->CreatureDisplayID); // Modelid4
        else
            data << uint32(0);                                       // Modelid4
        data << float(ci->ModHealth);                                // dmg/hp modifier
        data << float(ci->ModMana);                                  // dmg/mana modifier
        data << uint8(ci->RacialLeader);

        CreatureQuestItemList const* items = sObjectMgr->GetCreatureQuestItemList(entry);
        if (items)
            for (std::size_t i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
                data << (i < items->size() ? uint32((*items)[i]) : uint32(0));
        else
            for (std::size_t i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
                data << uint32(0);

        data << uint32(ci->MovementID);                              // CreatureMovementInfo.dbc
        SendPacket(&data);
    }
    else
    {
        LOG_DEBUG("network", "WORLD: CMSG_CREATURE_QUERY - NO CREATURE INFO! ({})", guid.ToString());
        WorldPacket data(SMSG_CREATURE_QUERY_RESPONSE, 4);
        data << uint32(entry | 0x80000000);
        SendPacket(&data);
        LOG_DEBUG("network", "WORLD: Sent SMSG_CREATURE_QUERY_RESPONSE");
    }
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleGameObjectQueryOpcode(WorldPacket& recvData)
{
    uint32 entry;
    recvData >> entry;
    ObjectGuid guid;
    recvData >> guid;

    const GameObjectTemplate* info = sObjectMgr->GetGameObjectTemplate(entry);
    if (info)
    {
        LOG_DEBUG("network", "WORLD: CMSG_GAMEOBJECT_QUERY '{}' - Entry: {}. ", info->name, entry);
        WorldPacket data (SMSG_GAMEOBJECT_QUERY_RESPONSE, 150);
        data << uint32(entry);
        data << uint32(info->Type);
        data << uint32(info->DisplayID);
        data << info->name;
        data << uint8(0) << uint8(0) << uint8(0);           // name2, name3, name4
        data << info->IconName;                                   // 2.0.3, string. Icon name to use instead of default icon for go's (ex: "Attack" makes sword)
        data << info->CastBarCaption;                             // 2.0.3, string. Text will appear in Cast Bar when using GO (ex: "Collecting")
        data << info->AlertText;                                 // 2.0.3, string
        data.append(info->Raw.data, MAX_GAME_OBJECT_DATA);
        data << float(info->Size);                          // go size

        GameObjectQuestItemList const* items = sObjectMgr->GetGameObjectQuestItemList(entry);
        if (items)
            for (std::size_t i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; ++i)
                data << (i < items->size() ? uint32((*items)[i]) : uint32(0));
        else
            for (std::size_t i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; ++i)
                data << uint32(0);

        SendPacket(&data);
        LOG_DEBUG("network", "WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE");
    }
    else
    {
        LOG_DEBUG("network", "WORLD: CMSG_GAMEOBJECT_QUERY - Missing gameobject info for ({})", guid.ToString());
        WorldPacket data (SMSG_GAMEOBJECT_QUERY_RESPONSE, 4);
        data << uint32(entry | 0x80000000);
        SendPacket(&data);
        LOG_DEBUG("network", "WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE");
    }
}

void WorldSession::HandleCorpseQueryOpcode(WorldPacket& /*recvData*/)
{
    if (!_player->HasCorpse())
    {
        WorldPacket data(MSG_CORPSE_QUERY, 1);
        data << uint8(0);                                   // corpse not found
        SendPacket(&data);
        return;
    }

    WorldLocation corpseLocation = _player->GetCorpseLocation();
    uint32 corpseMapID = corpseLocation.GetMapId();
    uint32 mapID = corpseLocation.GetMapId();
    float x = corpseLocation.GetPositionX();
    float y = corpseLocation.GetPositionY();
    float z = corpseLocation.GetPositionZ();

    // if corpse at different map
    if (mapID != _player->GetMapId())
    {
        // search entrance map for proper show entrance
        if (MapEntry const* corpseMapEntry = sMapStore.LookupEntry(mapID))
        {
            if (corpseMapEntry->IsDungeon() && corpseMapEntry->EntranceMap >= 0)
            {
                // if corpse map have entrance
                if (Map const* entranceMap = sMapMgr->CreateBaseMap(corpseMapEntry->EntranceMap))
                {
                    mapID = corpseMapEntry->EntranceMap;
                    x = corpseMapEntry->EntranceX;
                    y = corpseMapEntry->EntranceY;
                    z = entranceMap->GetHeight(GetPlayer()->GetPhaseMask(), x, y, MAX_HEIGHT);
                }
            }
        }
    }

    WorldPacket data(MSG_CORPSE_QUERY, 1 + (6 * 4));
    data << uint8(1);                                       // corpse found
    data << int32(mapID);
    data << float(x);
    data << float(y);
    data << float(z);
    data << int32(corpseMapID);
    data << uint32(0);                                      // unknown
    SendPacket(&data);
}

void WorldSession::HandleNpcTextQueryOpcode(WorldPacket& recvData)
{
    uint32 textID;
    ObjectGuid guid;

    recvData >> textID;
    LOG_DEBUG("network", "WORLD: CMSG_NPC_TEXT_QUERY TextId: {}", textID);

    recvData >> guid;

    GossipText const* gossip = sObjectMgr->GetGossipText(textID);

    WorldPacket data(SMSG_NPC_TEXT_UPDATE, 100);          // guess size
    data << textID;

    if (!gossip)
    {
        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            data << float(0);
            data << "Greetings $N";
            data << "Greetings $N";
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
        }
    }
    else
    {
        std::string text0[MAX_GOSSIP_TEXT_OPTIONS], text1[MAX_GOSSIP_TEXT_OPTIONS];
        LocaleConstant locale = GetSessionDbLocaleIndex();

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            if (const BroadcastText* bct = sObjectMgr->GetBroadcastText(gossip->Options[i].BroadcastTextID))
            {
                text0[i] = bct->GetText(GENDER_MALE, true);
                text1[i] = bct->GetText(GENDER_FEMALE, true);
            }
            else
            {
                text0[i] = gossip->Options[i].Text0;
                text1[i] = gossip->Options[i].Text1;
            }

            data << gossip->Options[i].Probability;

            if (text0[i].empty())
                data << text1[i];
            else
                data << text0[i];

            if (text1[i].empty())
                data << text0[i];
            else
                data << text1[i];

            data << gossip->Options[i].Language;

            for (uint8 j = 0; j < MAX_GOSSIP_TEXT_EMOTES; ++j)
            {
                data << gossip->Options[i].Emotes[j]._Delay;
                data << gossip->Options[i].Emotes[j]._Emote;
            }
        }
    }

    SendPacket(&data);

    LOG_DEBUG("network", "WORLD: Sent SMSG_NPC_TEXT_UPDATE");
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandlePageTextQueryOpcode(WorldPacket& recvData)
{
    uint32 pageID;
    recvData >> pageID;
    recvData.read_skip<uint64>();                          // guid

    while (pageID)
    {
        PageText const* pageText = sObjectMgr->GetPageText(pageID);
        // guess size
        WorldPacket data(SMSG_PAGE_TEXT_QUERY_RESPONSE, 50);
        data << pageID;

        if (!pageText)
        {
            data << "Item page missing.";
            data << static_cast<uint32>(0);
            pageID = 0;
        }
        else
        {
            data << pageText->Text;
            data << pageText->NextPage;
            pageID = pageText->NextPage;
        }
        SendPacket(&data);

        LOG_DEBUG("network", "WORLD: Sent SMSG_PAGE_TEXT_QUERY_RESPONSE");
    }
}

void WorldSession::HandleCorpseMapPositionQuery(WorldPackets::Query::CorpseMapPositionQuery& /*packet*/)
{
    LOG_DEBUG("network", "WORLD: Recv CMSG_CORPSE_MAP_POSITION_QUERY");

    WorldPacket data(SMSG_CORPSE_MAP_POSITION_QUERY_RESPONSE, 4 + 4 + 4 + 4);
    data << float(0);
    data << float(0);
    data << float(0);
    data << float(0);
    SendPacket(&data);
}

void WorldSession::HandleQuestPOIQuery(WorldPacket& recvData)
{
    uint32 count;
    recvData >> count; // quest count, max=25

    if (count > MAX_QUEST_LOG_SIZE)
    {
        recvData.rFinish();
        return;
    }

    // Read quest ids and add the in a unordered_set so we don't send POIs for the same quest multiple times
    std::unordered_set<uint32> questIds;
    for (uint32 i = 0; i < count; ++i)
        questIds.insert(recvData.read<uint32>()); // quest id

    WorldPacket data(SMSG_QUEST_POI_QUERY_RESPONSE, 4 + (4 + 4)*questIds.size());
    data << uint32(questIds.size()); // count

    for (std::unordered_set<uint32>::const_iterator itr = questIds.begin(); itr != questIds.end(); ++itr)
    {
        uint32 questId = *itr;
        bool questOk = false;

        uint16 questSlot = _player->FindQuestSlot(questId);

        if (questSlot != MAX_QUEST_LOG_SIZE)
            questOk = _player->GetQuestSlotQuestId(questSlot) == questId;

        if (questOk)
        {
            QuestPOIVector const* POI = sObjectMgr->GetQuestPOIVector(questId);

            if (POI)
            {
                data << uint32(questId); // quest ID
                data << uint32(POI->size()); // POI count

                for (QuestPOIVector::const_iterator itr = POI->begin(); itr != POI->end(); ++itr)
                {
                    data << uint32(itr->ID);                // POI index
                    data << int32(itr->ObjectiveIndex);     // objective index
                    data << uint32(itr->MapID);             // mapid
                    data << uint32(itr->AreaID);            // areaid
                    data << uint32(itr->FloorID);           // floorid
                    data << uint32(itr->Priority);              // unknown
                    data << uint32(itr->Flags);              // unknown
                    data << uint32(itr->points.size());     // POI points count

                    for (std::vector<QuestPOIPoint>::const_iterator itr2 = itr->points.begin(); itr2 != itr->points.end(); ++itr2)
                    {
                        data << int32(itr2->x); // POI point x
                        data << int32(itr2->y); // POI point y
                    }
                }
            }
            else
            {
                data << uint32(questId); // quest ID
                data << uint32(0); // POI count
            }
        }
        else
        {
            data << uint32(questId); // quest ID
            data << uint32(0); // POI count
        }
    }

    SendPacket(&data);
}
