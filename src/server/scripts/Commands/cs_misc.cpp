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

#include "AccountMgr.h"
#include "ArenaTeamMgr.h"
#include "BattlegroundMgr.h"
#include "CellImpl.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Common.h"
#include "GameGraveyard.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridTerrainLoader.h"
#include "Group.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "LFG.h"
#include "LFGMgr.h"
#include "Language.h"
#include "MapMgr.h"
#include "MiscPackets.h"
#include "MovementGenerator.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "Realm.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "TargetedMovementGenerator.h"
#include "Tokenize.h"
#include "Transport.h"
#include "WeatherMgr.h"
#include "WorldSessionMgr.h"

/// @todo: this import is not necessary for compilation and marked as unused by the IDE
//  however, for some reasons removing it would cause a damn linking issue
//  there is probably some underlying problem with imports which should properly addressed
//  see: https://github.com/azerothcore/azerothcore-wotlk/issues/9766
#include "GridNotifiersImpl.h"

constexpr auto SPELL_STUCK = 7355;
constexpr auto SPELL_FREEZE = 9454;

std::string GetCreatureName(const Creature* creature)
{
    std::string name;
    if (const auto creatureTemplate = sObjectMgr->GetCreatureTemplate(creature->GetEntry()))
        name = creatureTemplate->Name;
    if (name.empty())
        name = "Unknown creature";
    return name;
}

using namespace Acore::ChatCommands;

class misc_commandscript : public CommandScript
{
public:
    misc_commandscript() : CommandScript("misc_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable auraCommandTable =
        {
            { "stack",             HandleAuraStacksCommand,        SEC_GAME_MASTER,         Console::No  },
            { "",                  HandleAuraCommand,              SEC_GAME_MASTER,         Console::No  }
        };

        static ChatCommandTable commandTable =
        {
            { "commentator",       HandleCommentatorCommand,       SEC_MODERATOR,          Console::No  },
            { "dev",               HandleDevCommand,               SEC_ADMINISTRATOR,      Console::No  },
            { "gps",               HandleGPSCommand,               SEC_MODERATOR,          Console::No  },
            { "aura",              auraCommandTable                                                     },
            { "unaura",            HandleUnAuraCommand,            SEC_GAME_MASTER,         Console::No  },
            { "appear",            HandleAppearCommand,            SEC_MODERATOR,          Console::No  },
            { "summon",            HandleSummonCommand,            SEC_GAME_MASTER,         Console::No  },
            { "groupsummon",       HandleGroupSummonCommand,       SEC_GAME_MASTER,         Console::No  },
            { "commands",          HandleCommandsCommand,          SEC_PLAYER,             Console::Yes },
            { "die",               HandleDieCommand,               SEC_GAME_MASTER,         Console::No  },
            { "revive",            HandleReviveCommand,            SEC_GAME_MASTER,         Console::Yes },
            { "dismount",          HandleDismountCommand,          SEC_PLAYER,             Console::No  },
            { "guid",              HandleGUIDCommand,              SEC_GAME_MASTER,         Console::No  },
            { "help",              HandleHelpCommand,              SEC_PLAYER,             Console::Yes },
            { "cooldown",          HandleCooldownCommand,          SEC_GAME_MASTER,         Console::No  },
            { "distance",          HandleGetDistanceCommand,       SEC_ADMINISTRATOR,      Console::No  },
            { "recall",            HandleRecallCommand,            SEC_GAME_MASTER,         Console::No  },
            { "save",              HandleSaveCommand,              SEC_PLAYER,             Console::No  },
            { "saveall",           HandleSaveAllCommand,           SEC_GAME_MASTER,         Console::Yes },
            { "unstuck",           HandleUnstuckCommand,           SEC_GAME_MASTER,         Console::Yes },
            { "linkgrave",         HandleLinkGraveCommand,         SEC_ADMINISTRATOR,      Console::No  },
            { "neargrave",         HandleNearGraveCommand,         SEC_GAME_MASTER,         Console::No  },
            { "showarea",          HandleShowAreaCommand,          SEC_GAME_MASTER,         Console::No  },
            { "hidearea",          HandleHideAreaCommand,          SEC_ADMINISTRATOR,      Console::No  },
            { "additem",           HandleAddItemCommand,           SEC_GAME_MASTER,         Console::Yes },
            { "additem set",       HandleAddItemSetCommand,        SEC_GAME_MASTER,         Console::No  },
            { "wchange",           HandleChangeWeather,            SEC_ADMINISTRATOR,      Console::No  },
            { "maxskill",          HandleMaxSkillCommand,          SEC_GAME_MASTER,         Console::No  },
            { "setskill",          HandleSetSkillCommand,          SEC_GAME_MASTER,         Console::No  },
            { "respawn",           HandleRespawnCommand,           SEC_GAME_MASTER,         Console::No  },
            { "respawn all",       HandleRespawnAllCommand,        SEC_GAME_MASTER,         Console::No  },
            { "movegens",          HandleMovegensCommand,          SEC_ADMINISTRATOR,      Console::No  },
            { "cometome",          HandleComeToMeCommand,          SEC_ADMINISTRATOR,      Console::No  },
            { "damage",            HandleDamageCommand,            SEC_GAME_MASTER,         Console::No  },
            { "combatstop",        HandleCombatStopCommand,        SEC_GAME_MASTER,         Console::Yes },
            { "flusharenapoints",  HandleFlushArenaPointsCommand,  SEC_ADMINISTRATOR,      Console::Yes },
            { "freeze",            HandleFreezeCommand,            SEC_GAME_MASTER,         Console::No  },
            { "unfreeze",          HandleUnFreezeCommand,          SEC_GAME_MASTER,         Console::No  },
            { "possess",           HandlePossessCommand,           SEC_GAME_MASTER,         Console::No  },
            { "unpossess",         HandleUnPossessCommand,         SEC_GAME_MASTER,         Console::No  },
            { "bindsight",         HandleBindSightCommand,         SEC_ADMINISTRATOR,      Console::No  },
            { "unbindsight",       HandleUnbindSightCommand,       SEC_ADMINISTRATOR,      Console::No  },
            { "playall",           HandlePlayAllCommand,           SEC_GAME_MASTER,         Console::No  },
            { "skirmish",          HandleSkirmishCommand,          SEC_ADMINISTRATOR,      Console::No  },
            { "mailbox",           HandleMailBoxCommand,           SEC_MODERATOR,          Console::No  },
            { "string",            HandleStringCommand,            SEC_GAME_MASTER,         Console::No  },
            { "opendoor",          HandleOpenDoorCommand,          SEC_GAME_MASTER,         Console::No  },
            { "bm",                HandleBMCommand,                SEC_GAME_MASTER,         Console::No  },
            { "packetlog",         HandlePacketLog,                SEC_GAME_MASTER,         Console::Yes }
        };

        return commandTable;
    }

    static bool HandleSkirmishCommand(ChatHandler* handler, std::vector<std::string_view> args)
    {
        auto tokens = args;

        if (args.empty() || !tokens.size())
        {
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto tokensItr = tokens.begin();

        std::vector<BattlegroundTypeId> allowedArenas;
        std::string_view arenasStr = *(tokensItr++);

        auto arenaTokens = Acore::Tokenize(arenasStr, ',', false);
        for (auto const& arenaName : arenaTokens)
        {
            if (arenaName == "all")
            {
                if (arenaTokens.size() > 1)
                {
                    handler->SendErrorMessage("Invalid [arena] specified.");
                    return false;
                }

                allowedArenas.emplace_back(BATTLEGROUND_NA);
                allowedArenas.emplace_back(BATTLEGROUND_BE);
                allowedArenas.emplace_back(BATTLEGROUND_RL);
                allowedArenas.emplace_back(BATTLEGROUND_DS);
                allowedArenas.emplace_back(BATTLEGROUND_RV);
            }
            else if (arenaName == "NA")
            {
                allowedArenas.emplace_back(BATTLEGROUND_NA);
            }
            else if (arenaName == "BE")
            {
                allowedArenas.emplace_back(BATTLEGROUND_BE);
            }
            else if (arenaName == "RL")
            {
                allowedArenas.emplace_back(BATTLEGROUND_RL);
            }
            else if (arenaName == "DS")
            {
                allowedArenas.emplace_back(BATTLEGROUND_DS);
            }
            else if (arenaName == "RV")
            {
                allowedArenas.emplace_back(BATTLEGROUND_RV);
            }
            else
            {
                handler->SendErrorMessage("Invalid [arena] specified.");
                return false;
            }
        }

        ASSERT(!allowedArenas.empty());
        BattlegroundTypeId randomizedArenaBgTypeId = Acore::Containers::SelectRandomContainerElement(allowedArenas);

        uint8 count = 0;
        if (tokensItr != tokens.end())
        {
            std::string_view mode = *(tokensItr++);

            if (mode == "1v1")
            {
                count = 2;
            }
            else if (mode == "2v2")
            {
                count = 4;
            }
            else if (mode == "3v3")
            {
                count = 6;
            }
            else if (mode == "5v5")
            {
                count = 10;
            }
        }

        if (!count)
        {
            handler->SendErrorMessage("Invalid bracket. Can be 1v1, 2v2, 3v3, 5v5");
            return false;
        }

        if (tokens.size() != uint16(count + 2))
        {
            handler->SendErrorMessage("Invalid number of nicknames for this bracket.");
            return false;
        }

        uint8 hcnt = count / 2;
        uint8 error = 0;
        std::string last_name;
        Player* plr = nullptr;
        std::array<Player*, 10> players = {};
        uint8 cnt = 0;

        for (; tokensItr != tokens.end(); ++tokensItr)
        {
            last_name = std::string(*tokensItr);
            plr = ObjectAccessor::FindPlayerByName(last_name, false);

            if (!plr)
            {
                error = 1;
                break;
            }

            if (!plr->IsInWorld() || !plr->FindMap() || plr->IsBeingTeleported())
            {
                error = 2;
                break;
            }

            if (plr->GetMap()->GetEntry()->InstanceAble())
            {
                error = 3;
                break;
            }

            if (plr->IsUsingLfg())
            {
                error = 4;
                break;
            }

            if (plr->InBattlegroundQueue())
            {
                error = 5;
                break;
            }

            if (plr->IsInFlight())
            {
                error = 10;
                break;
            }

            if (!plr->IsAlive())
            {
                error = 11;
                break;
            }

            const Group* g = plr->GetGroup();

            if (hcnt > 1)
            {
                if (!g)
                {
                    error = 6;
                    break;
                }

                if (g->isRaidGroup() || g->isBGGroup() || g->isBFGroup() || g->isLFGGroup())
                {
                    error = 7;
                    break;
                }

                if (g->GetMembersCount() != hcnt)
                {
                    error = 8;
                    break;
                }

                uint8 sti = (cnt < hcnt ? 0 : hcnt);
                if (sti != cnt && players[sti]->GetGroup() != plr->GetGroup())
                {
                    error = 9;
                    last_name += " and " + players[sti]->GetName();
                    break;
                }
            }
            else // 1v1
            {
                if (g)
                {
                    error = 12;
                    break;
                }
            }

            players[cnt++] = plr;
        }

        for (uint8 i = 0; i < cnt && !error; ++i)
        {
            for (uint8 j = i + 1; j < cnt; ++j)
            {
                if (players[i]->GetGUID() == players[j]->GetGUID())
                {
                    last_name = players[i]->GetName();
                    error = 13;
                    break;
                }
            }
        }

        switch (error)
        {
            case 1:
                handler->PSendSysMessage("Player {} not found.", last_name);
                break;
            case 2:
                handler->PSendSysMessage("Player {} is being teleported.", last_name);
                break;
            case 3:
                handler->PSendSysMessage("Player {} is in instance/battleground/arena.", last_name);
                break;
            case 4:
                handler->PSendSysMessage("Player {} is in LFG system.", last_name);
                break;
            case 5:
                handler->PSendSysMessage("Player {} is queued for battleground/arena.", last_name);
                break;
            case 6:
                handler->PSendSysMessage("Player {} is not in group.", last_name);
                break;
            case 7:
                handler->PSendSysMessage("Player {} is not in normal group.", last_name);
                break;
            case 8:
                handler->PSendSysMessage("Group of player {} has invalid member count.", last_name);
                break;
            case 9:
                handler->PSendSysMessage("Players {} are not in the same group.", last_name);
                break;
            case 10:
                handler->PSendSysMessage("Player {} is in flight.", last_name);
                break;
            case 11:
                handler->PSendSysMessage("Player {} is dead.", last_name);
                break;
            case 12:
                handler->PSendSysMessage("Player {} is in a group.", last_name);
                break;
            case 13:
                handler->PSendSysMessage("Player {} occurs more than once.", last_name);
                break;
        }

        if (error)
        {
            handler->SetSentErrorMessage(true);
            return false;
        }

        Battleground* bgt = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
        if (!bgt)
        {
            handler->SendErrorMessage("Couldn't create arena map!");
            return false;
        }

        Battleground* bg = sBattlegroundMgr->CreateNewBattleground(randomizedArenaBgTypeId, GetBattlegroundBracketById(bgt->GetMapId(), bgt->GetBracketId()), ArenaType(hcnt >= 2 ? hcnt : 2), false);
        if (!bg)
        {
            handler->SendErrorMessage("Couldn't create arena map!");
            return false;
        }

        bg->StartBattleground();

        BattlegroundTypeId bgTypeId = bg->GetBgTypeID();

        TeamID teamId1 = Player::TeamIdForRace(players[0]->getRace());
        TeamID teamId2 = (teamId1 == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE);

        for (uint8 i = 0; i < cnt; ++i)
        {
            Player* player = players[i];

            TeamID teamId = (i < hcnt ? teamId1 : teamId2);
            player->SetEntryPoint();

            uint32 queueSlot = 0;
            WorldPacket data;
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, queueSlot, STATUS_IN_PROGRESS, 0, bg->GetStartTime(), bg->GetArenaType(), teamId);
            player->SendDirectMessage(&data);

            // Remove from LFG queues
            sLFGMgr->LeaveAllLfgQueues(player->GetGUID(), false);

            player->SetBattlegroundId(bg->GetInstanceID(), bgTypeId, queueSlot, true, false, teamId);
            sBattlegroundMgr->SendToBattleground(player, bg->GetInstanceID(), bgTypeId);
        }

        handler->PSendSysMessage("Success! Players are now being teleported to the arena.");
        return true;
    }

    static bool HandleCommentatorCommand(ChatHandler* handler, Optional<bool> enableArg)
    {
        WorldSession* session = handler->GetSession();

        if (!session)
        {
            handler->SendErrorMessage(LANG_USE_BOL);
            return false;
        }

        auto SetCommentatorMod = [&](bool enable)
        {
            handler->SendNotification(enable ? "Commentator mode on" : "Commentator mode off");
            session->GetPlayer()->SetCommentator(enable);
        };

        if (!enableArg)
        {
            if (session->IsGameMaster() && session->GetPlayer()->IsCommentator())
            {
                SetCommentatorMod(true);
            }
            else
            {
                SetCommentatorMod(false);
            }

            return true;
        }

        if (*enableArg)
        {
            SetCommentatorMod(true);
            return true;
        }
        else
        {
            SetCommentatorMod(false);
            return true;
        }
    }

    static bool HandleDevCommand(ChatHandler* handler, Optional<bool> enableArg)
    {
        WorldSession* session = handler->GetSession();

        if (!session)
        {
            handler->SendErrorMessage(LANG_USE_BOL);
            return false;
        }

        auto SetDevMod = [&](bool enable)
        {
            handler->SendNotification(enable ? LANG_DEV_ON : LANG_DEV_OFF);
            session->GetPlayer()->SetDeveloper(enable);
            sScriptMgr->OnHandleDevCommand(handler->GetSession()->GetPlayer(), enable);
        };

        if (!enableArg)
        {
            if (session->IsGameMaster() && session->GetPlayer()->IsDeveloper())
            {
                SetDevMod(true);
            }
            else
            {
                SetDevMod(false);
            }

            return true;
        }

        if (*enableArg)
        {
            SetDevMod(true);
            return true;
        }
        else
        {
            SetDevMod(false);
            return true;
        }
    }

    static bool HandleGPSCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        WorldObject* object = handler->getSelectedUnit();

        if (!object && !target)
        {
            return false;
        }

        if (!object && target && target->IsConnected())
        {
            object = target->GetConnectedPlayer();
        }

        if (!object)
        {
            return false;
        }

        CellCoord const cellCoord = Acore::ComputeCellCoord(object->GetPositionX(), object->GetPositionY());
        Cell cell(cellCoord);

        uint32 zoneId, areaId;
        object->GetZoneAndAreaId(zoneId, areaId);

        MapEntry const* mapEntry = sMapStore.LookupEntry(object->GetMapId());
        AreaTableEntry const* zoneEntry = sAreaTableStore.LookupEntry(zoneId);
        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(areaId);

        float zoneX = object->GetPositionX();
        float zoneY = object->GetPositionY();

        Map2ZoneCoordinates(zoneX, zoneY, zoneId);

        float groundZ = object->GetMapHeight(object->GetPositionX(), object->GetPositionY(), MAX_HEIGHT);
        float floorZ = object->GetMapHeight(object->GetPositionX(), object->GetPositionY(), object->GetPositionZ());

        uint32 haveMap = GridTerrainLoader::ExistMap(object->GetMapId(), cell.GridX(), cell.GridY()) ? 1 : 0;
        uint32 haveVMap = GridTerrainLoader::ExistVMap(object->GetMapId(), cell.GridX(), cell.GridY()) ? 1 : 0;
        uint32 haveMMAP = handler->GetSession()->GetPlayer()->GetMap()->GetMapCollisionData().GetMMapData().GetNavMesh() ? 1 : 0;

        if (haveVMap)
        {
            if (object->IsOutdoors())
            {
                handler->PSendSysMessage("You are outdoors");
            }
            else
            {
                handler->PSendSysMessage("You are indoors");
            }
        }
        else
        {
            handler->PSendSysMessage("no VMAP available for area info");
        }

        handler->PSendSysMessage(LANG_MAP_POSITION,
                                 object->GetMapId(), (mapEntry ? mapEntry->Name : "<unknown>"),
                                 zoneId, (zoneEntry ? zoneEntry->AreaName : "<unknown>"),
                                 areaId, (areaEntry ? areaEntry->AreaName : "<unknown>"),
                                 object->GetPhaseMask(),
                                 object->GetPositionX(), object->GetPositionY(), object->GetPositionZ(), object->GetOrientation(),
                                 cell.GridX(), cell.GridY(), cell.CellX(), cell.CellY(), object->GetInstanceId(),
                                 zoneX, zoneY, groundZ, floorZ, haveMap, haveVMap, haveMMAP);

        LiquidData const& liquidData = object->GetLiquidData();

        if (liquidData.Status)
        {
            handler->PSendSysMessage(LANG_LIQUID_STATUS, liquidData.Level, liquidData.DepthLevel, liquidData.Entry, liquidData.Flags, liquidData.Status);
        }

        if (object->GetTransport())
        {
            handler->PSendSysMessage("Transport offset: {:0.2f}, {:0.2f}, {:0.2f}, {:0.2f}", object->m_movementInfo.transport.pos.GetPositionX(), object->m_movementInfo.transport.pos.GetPositionY(), object->m_movementInfo.transport.pos.GetPositionZ(), object->m_movementInfo.transport.pos.GetOrientation());
        }

        return true;
    }

    static bool HandleAuraCommand(ChatHandler* handler, SpellInfo const* spell)
    {
        if (!spell)
        {
            handler->SendErrorMessage(LANG_COMMAND_NOSPELLFOUND);
            return false;
        }

        if (!SpellMgr::IsSpellValid(spell))
        {
            handler->SendErrorMessage(LANG_COMMAND_SPELL_BROKEN, spell->ID);
            return false;
        }

        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        Aura::TryRefreshStackOrCreate(spell, MAX_EFFECT_MASK, target, target);

        return true;
    }

    static bool HandleAuraStacksCommand(ChatHandler* handler, SpellInfo const* spell, int16 stacks)
    {
        if (!spell)
        {
            handler->SendErrorMessage(LANG_COMMAND_NOSPELLFOUND);
            return false;
        }

        if (!SpellMgr::IsSpellValid(spell))
        {
            handler->SendErrorMessage(LANG_COMMAND_SPELL_BROKEN, spell->ID);
            return false;
        }

        if (!stacks)
        {
            handler->SendErrorMessage(LANG_COMMAND_AURASTACK_NO_STACK);
            return false;
        }

        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        Aura* aur = target->GetAura(spell->ID);
        if (!aur)
        {
            handler->SendErrorMessage(LANG_COMMAND_AURASTACK_NO_AURA, spell->ID);
            return false;
        }

        if (!spell->StackAmount)
        {
            handler->SendErrorMessage(LANG_COMMAND_AURASTACK_CANT_STACK, spell->ID);
            return false;
        }

        aur->ModStackAmount(stacks);

        return true;
    }

    static bool HandleUnAuraCommand(ChatHandler* handler, Variant<SpellInfo const*, std::string_view> spells)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        if (spells.holds_alternative<std::string_view>() && spells.get<std::string_view>() == "all")
        {
            target->RemoveAllAuras();
            return true;
        }

        if (!spells.holds_alternative<SpellInfo const*>())
        {
            handler->SendErrorMessage(LANG_COMMAND_NOSPELLFOUND);
            return false;
        }

        auto spell = spells.get<SpellInfo const*>();

        if (!SpellMgr::IsSpellValid(spell))
        {
            handler->SendErrorMessage(LANG_COMMAND_SPELL_BROKEN, spell->ID);
            return false;
        }

        target->RemoveAurasDueToSpell(spell->ID);

        return true;
    }
    // Teleport to Player
    static bool HandleAppearCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            target = PlayerIdentifier::FromTarget(handler);
        }

        if (!target)
        {
            return false;
        }

        Player* _player = handler->GetSession()->GetPlayer();
        if (target->GetGUID() == _player->GetGUID())
        {
            handler->SendErrorMessage(LANG_CANT_TELEPORT_SELF);
            return false;
        }

        std::string nameLink = handler->playerLink(target->GetName());

        if (target->IsConnected())
        {
            auto targetPlayer = target->GetConnectedPlayer();

            // check online security
            if (handler->HasLowerSecurity(targetPlayer))
            {
                return false;
            }

            Map* map = targetPlayer->GetMap();
            if (map->IsBattlegroundOrArena())
            {
                // only allow if gm mode is on
                if (!_player->IsGameMaster())
                {
                    handler->SendErrorMessage(LANG_CANNOT_GO_TO_BG_GM, nameLink);
                    return false;
                }

                if (!_player->GetMap()->IsBattlegroundOrArena())
                {
                    _player->SetEntryPoint();
                }

                _player->SetBattlegroundId(targetPlayer->GetBattlegroundId(), targetPlayer->GetBattlegroundTypeId(), PLAYER_MAX_BATTLEGROUND_QUEUES, false, false, TEAM_NEUTRAL);
            }
            else if (map->IsDungeon())
            {
                // we have to go to instance, and can go to player only if:
                //   1) we are in his group (either as leader or as member)
                //   2) we are not bound to any group and have GM mode on
                if (_player->GetGroup())
                {
                    // we are in group, we can go only if we are in the player group
                    if (_player->GetGroup() != targetPlayer->GetGroup())
                    {
                        handler->SendErrorMessage(LANG_CANNOT_GO_TO_INST_PARTY, nameLink);
                        return false;
                    }
                }
                else
                {
                    // we are not in group, let's verify our GM mode
                    if (!_player->IsGameMaster())
                    {
                        handler->SendErrorMessage(LANG_CANNOT_GO_TO_INST_GM, nameLink);
                        return false;
                    }
                }

                // if the GM is bound to another instance, he will not be bound to another one
                InstancePlayerBind* bind = sInstanceSaveMgr->PlayerGetBoundInstance(_player->GetGUID(), targetPlayer->GetMapId(), targetPlayer->GetDifficulty(map->IsRaid()));
                if (!bind)
                {
                    if (InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(target->GetConnectedPlayer()->GetInstanceId()))
                    {
                        sInstanceSaveMgr->PlayerBindToInstance(_player->GetGUID(), save, !save->CanReset(), _player);
                    }
                }

                if (map->IsRaid())
                {
                    _player->SetRaidDifficulty(targetPlayer->GetRaidDifficulty());
                }
                else
                {
                    _player->SetDungeonDifficulty(targetPlayer->GetDungeonDifficulty());
                }
            }

            handler->PSendSysMessage(LANG_APPEARING_AT, nameLink);

            // stop flight if need
            if (_player->IsInFlight())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->CleanupAfterTaxiFlight();
            }
            else // save only in non-flight case
                _player->SaveRecallPosition();

            if (Transport* transport = targetPlayer->GetTransport())
            {
                if (Transport* oldTransport = _player->GetTransport())
                    oldTransport->RemovePassenger(_player, true);

                float x;
                float y;
                float z;
                float o;
                targetPlayer->m_movementInfo.transport.pos.GetPosition(x, y, z, o);

                _player->SetTransport(transport);
                _player->m_movementInfo.transport.guid = transport->GetGUID();
                _player->m_movementInfo.transport.pos.Relocate(x, y, z, o);
                _player->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);

                float worldX = x;
                float worldY = y;
                float worldZ = z;
                float worldO = o;
                transport->CalculatePassengerPosition(worldX, worldY, worldZ, &worldO);

                transport->AddPassenger(_player, false);

                if (_player->TeleportTo(transport->GetMapId(), worldX, worldY, worldZ + 0.25f, worldO, TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_GM_MODE, targetPlayer))
                    _player->SetPhaseMask(targetPlayer->GetPhaseMask() | 1, false);
            }
            else
            {
                if (_player->TeleportTo(targetPlayer->GetMapId(), targetPlayer->GetPositionX(), targetPlayer->GetPositionY(), targetPlayer->GetPositionZ() + 0.25f, _player->GetOrientation(), TELE_TO_GM_MODE, targetPlayer))
                    _player->SetPhaseMask(targetPlayer->GetPhaseMask() | 1, false);
            }
        }
        else
        {
            // check offline security
            if (handler->HasLowerSecurity(nullptr, target->GetGUID()))
            {
                return false;
            }

            handler->PSendSysMessage(LANG_APPEARING_AT, nameLink);

            // to point where player stay (if loaded)
            float x, y, z, o;
            uint32 map;
            bool in_flight;

            if (!Player::LoadPositionFromDB(map, x, y, z, o, in_flight, target->GetGUID().GetCounter()))
            {
                return false;
            }

            // stop flight if need
            if (_player->IsInFlight())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
            {
                _player->SaveRecallPosition();
            }

            _player->TeleportTo(map, x, y, z, _player->GetOrientation());
        }

        return true;
    }

    // Summon Player
    static bool HandleSummonCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            target = PlayerIdentifier::FromTarget(handler);
        }

        if (!target)
        {
            return false;
        }

        Player* _player = handler->GetSession()->GetPlayer();
        if (target->GetGUID() == _player->GetGUID())
        {
            handler->SendErrorMessage(LANG_CANT_TELEPORT_SELF);
            return false;
        }

        std::string nameLink = handler->playerLink(target->GetName());

        if (target->IsConnected())
        {
            auto targetPlayer = target->GetConnectedPlayer();

            // check online security
            if (handler->HasLowerSecurity(targetPlayer))
            {
                return false;
            }

            if (targetPlayer->IsBeingTeleported())
            {
                handler->SendErrorMessage(LANG_IS_TELEPORTED, nameLink);
                return false;
            }

            Map* map = handler->GetSession()->GetPlayer()->GetMap();

            if (map->IsBattlegroundOrArena())
            {
                handler->SendErrorMessage("Can't summon to a battleground!");
                return false;
            }
            if (map->IsDungeon())
            {
                // Allow GM to summon players or only other GM accounts inside instances.
                if (!sWorld->getBoolConfig(CONFIG_INSTANCE_GMSUMMON_PLAYER))
                {
                    // pussywizard: prevent unbinding normal player's perm bind by just summoning him >_>
                    if (!targetPlayer->GetSession()->IsGameMaster())
                    {
                        handler->SendErrorMessage("Only GMs can be summoned to an instance!");
                        return false;
                    }
                }

                Map* destMap = targetPlayer->GetMap();

                if (destMap->Instanceable() && destMap->GetInstanceId() != map->GetInstanceId())
                {
                    sInstanceSaveMgr->PlayerUnbindInstance(target->GetGUID(), map->GetInstanceId(), targetPlayer->GetDungeonDifficulty(), true, targetPlayer);
                }

                // we are in an instance, and can only summon players in our group with us as leader
                if (!handler->GetSession()->GetPlayer()->GetGroup() || !targetPlayer->GetGroup() ||
                    (targetPlayer->GetGroup()->GetLeaderGUID() != handler->GetSession()->GetPlayer()->GetGUID()) ||
                    (handler->GetSession()->GetPlayer()->GetGroup()->GetLeaderGUID() != handler->GetSession()->GetPlayer()->GetGUID()))
                // the last check is a bit excessive, but let it be, just in case
                {
                    handler->SendErrorMessage(LANG_CANNOT_SUMMON_TO_INST, nameLink);
                    return false;
                }
            }

            handler->PSendSysMessage(LANG_SUMMONING, nameLink, "");
            if (handler->needReportToTarget(targetPlayer))
            {
                ChatHandler(targetPlayer->GetSession()).PSendSysMessage(LANG_SUMMONED_BY, handler->playerLink(_player->GetName()));
            }

            // stop flight if need
            if (targetPlayer->IsInFlight())
            {
                targetPlayer->GetMotionMaster()->MovementExpired();
                targetPlayer->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
            {
                targetPlayer->SaveRecallPosition();
            }

            // before GM
            float x, y, z;
            handler->GetSession()->GetPlayer()->GetClosePoint(x, y, z, targetPlayer->GetObjectSize());
            targetPlayer->TeleportTo(handler->GetSession()->GetPlayer()->GetMapId(), x, y, z, targetPlayer->GetOrientation(), 0, handler->GetSession()->GetPlayer());
        }
        else
        {
            // check offline security
            if (handler->HasLowerSecurity(nullptr, target->GetGUID()))
            {
                return false;
            }

            handler->PSendSysMessage(LANG_SUMMONING, nameLink, handler->GetNcoreString(LANG_OFFLINE));

            // in point where GM stay
            Player::SavePositionInDB(handler->GetSession()->GetPlayer()->GetMapId(),
                                     handler->GetSession()->GetPlayer()->GetPositionX(),
                                     handler->GetSession()->GetPlayer()->GetPositionY(),
                                     handler->GetSession()->GetPlayer()->GetPositionZ(),
                                     handler->GetSession()->GetPlayer()->GetOrientation(),
                                     handler->GetSession()->GetPlayer()->GetZoneId(),
                                     target->GetGUID());
        }

        return true;
    }

    // Summon group of player
    static bool HandleGroupSummonCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target || !target->IsConnected())
        {
            return false;
        }

        // check online security
        if (handler->HasLowerSecurity(target->GetConnectedPlayer()))
        {
            return false;
        }

        auto targetPlayer = target->GetConnectedPlayer();

        Group* group = targetPlayer->GetGroup();

        std::string nameLink = handler->playerLink(target->GetName());

        if (!group)
        {
            handler->SendErrorMessage(LANG_NOT_IN_GROUP, nameLink);
            return false;
        }

        Map* gmMap = handler->GetSession()->GetPlayer()->GetMap();
        bool toInstance = gmMap->Instanceable();

        // we are in instance, and can summon only player in our group with us as lead
        if (toInstance && (
                    !handler->GetSession()->GetPlayer()->GetGroup() || (group->GetLeaderGUID() != handler->GetSession()->GetPlayer()->GetGUID()) ||
                    (handler->GetSession()->GetPlayer()->GetGroup()->GetLeaderGUID() != handler->GetSession()->GetPlayer()->GetGUID())))
            // the last check is a bit excessive, but let it be, just in case
        {
            handler->SendErrorMessage(LANG_CANNOT_SUMMON_TO_INST);
            return false;
        }

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* player = itr->GetSource();

            if (!player || player == handler->GetSession()->GetPlayer() || !player->GetSession())
            {
                continue;
            }

            // check online security
            if (handler->HasLowerSecurity(player))
            {
                return false;
            }

            std::string plNameLink = handler->GetNameLink(player);

            if (player->IsBeingTeleported())
            {
                handler->SendErrorMessage(LANG_IS_TELEPORTED, plNameLink);
                return false;
            }

            if (toInstance)
            {
                Map* playerMap = player->GetMap();

                if (playerMap->Instanceable() && playerMap->GetInstanceId() != gmMap->GetInstanceId())
                {
                    // cannot summon from instance to instance
                    handler->SendErrorMessage(LANG_CANNOT_SUMMON_TO_INST, plNameLink);
                    return false;
                }
            }

            handler->PSendSysMessage(LANG_SUMMONING, plNameLink, "");
            if (handler->needReportToTarget(player))
            {
                ChatHandler(player->GetSession()).PSendSysMessage(LANG_SUMMONED_BY, handler->GetNameLink());
            }

            // stop flight if need
            if (player->IsInFlight())
            {
                player->GetMotionMaster()->MovementExpired();
                player->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
            {
                player->SaveRecallPosition();
            }

            // before GM
            float x, y, z;
            handler->GetSession()->GetPlayer()->GetClosePoint(x, y, z, player->GetObjectSize());
            player->TeleportTo(handler->GetSession()->GetPlayer()->GetMapId(), x, y, z, player->GetOrientation(), 0, handler->GetSession()->GetPlayer());
        }

        return true;
    }

    static bool HandleCommandsCommand(ChatHandler* handler)
    {
        SendCommandHelpFor(*handler, "");
        return true;
    }

    static bool HandleDieCommand(ChatHandler* handler)
    {
        Unit* target = handler->getSelectedUnit();

        if (!target || !handler->GetSession()->GetPlayer()->GetTarget())
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        if (target->IsPlayer())
        {
            if (handler->HasLowerSecurity(target->ToPlayer()))
            {
                return false;
            }
        }

        if (target->IsAlive())
        {
            if (sWorld->getBoolConfig(CONFIG_DIE_COMMAND_MODE))
            {
                Unit::Kill(handler->GetSession()->GetPlayer(), target);
            }
            else
            {
                Unit::DealDamage(handler->GetSession()->GetPlayer(), target, target->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false, true);
            }
        }

        return true;
    }

    static bool HandleReviveCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target)
            return false;

        if (target->IsConnected())
        {
            auto targetPlayer = target->GetConnectedPlayer();
            targetPlayer->RemoveAurasDueToSpell(27827); // Spirit of Redemption
            targetPlayer->ResurrectPlayer(targetPlayer->GetSession()->IsGameMaster() ? 1.0f : 0.5f);
            targetPlayer->SpawnCorpseBones();
            targetPlayer->SaveToDB(false, false);
        }
        else
        {
            CharacterDatabaseTransaction trans(nullptr);
            Player::OfflineResurrect(target->GetGUID(), trans);
        }

        return true;
    }

    static bool HandleDismountCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        // If player is not mounted, so go out :)
        if (!player->IsMounted())
        {
            handler->SendErrorMessage(LANG_CHAR_NON_MOUNTED);
            return false;
        }

        if (player->IsInFlight())
        {
            handler->SendErrorMessage(LANG_YOU_IN_FLIGHT);
            return false;
        }

        player->Dismount();
        player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        player->SetSpeed(MOVE_RUN, 1, true);
        player->SetSpeed(MOVE_FLIGHT, 1, true);
        return true;
    }

    static bool HandleGUIDCommand(ChatHandler* handler)
    {
        ObjectGuid guid = handler->GetSession()->GetPlayer()->GetTarget();

        if (!guid)
        {
            handler->SendErrorMessage(LANG_NO_SELECTION);
            return false;
        }

        handler->PSendSysMessage(LANG_OBJECT_GUID, guid.ToString());
        return true;
    }

    static bool HandleHelpCommand(ChatHandler* handler, Tail cmd)
    {
        Acore::ChatCommands::SendCommandHelpFor(*handler, cmd);

        if (cmd.empty())
        {
            Acore::ChatCommands::SendCommandHelpFor(*handler, "help");
        }

        return true;
    }

    static bool HandleCooldownCommand(ChatHandler* handler, Optional<SpellInfo const*> spell)
    {
        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        std::string nameLink = handler->GetNameLink(target);

        if (!spell)
        {
            target->RemoveAllSpellCooldown();
            handler->PSendSysMessage(LANG_REMOVEALL_COOLDOWN, nameLink);
        }
        else
        {
            if (!SpellMgr::IsSpellValid(*spell))
            {
                handler->SendErrorMessage(LANG_COMMAND_SPELL_BROKEN, spell.value()->ID);
                return false;
            }

            target->RemoveSpellCooldown(spell.value()->ID, true);
            handler->PSendSysMessage(LANG_REMOVE_COOLDOWN, spell.value()->ID, target == handler->GetSession()->GetPlayer() ? handler->GetNcoreString(LANG_YOU) : nameLink);
        }
        return true;
    }

    static bool HandleGetDistanceCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        WorldObject* object = handler->getSelectedUnit();

        if (!object && !target)
        {
            return false;
        }

        if (!object && target && target->IsConnected())
        {
            object = target->GetConnectedPlayer();
        }

        if (!object)
        {
            return false;
        }

        handler->PSendSysMessage(LANG_DISTANCE, handler->GetSession()->GetPlayer()->GetDistance(object), handler->GetSession()->GetPlayer()->GetDistance2d(object), handler->GetSession()->GetPlayer()->GetExactDist(object), handler->GetSession()->GetPlayer()->GetExactDist2d(object));
        return true;
    }
    // Teleport player to last position
    static bool HandleRecallCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target || !target->IsConnected())
        {
            return false;
        }

        auto targetPlayer = target->GetConnectedPlayer();

        // check online security
        if (handler->HasLowerSecurity(targetPlayer))
        {
            return false;
        }

        if (targetPlayer->IsBeingTeleported())
        {
            handler->SendErrorMessage(LANG_IS_TELEPORTED, handler->playerLink(target->GetName()));
            return false;
        }

        // stop flight if need
        if (targetPlayer->IsInFlight())
        {
            targetPlayer->GetMotionMaster()->MovementExpired();
            targetPlayer->CleanupAfterTaxiFlight();
        }

        targetPlayer->TeleportTo(targetPlayer->m_recallMap, targetPlayer->m_recallX, targetPlayer->m_recallY, targetPlayer->m_recallZ, targetPlayer->m_recallO);
        return true;
    }

    static bool HandleSaveCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        // save GM account without delay and output message
        if (handler->GetSession()->IsGameMaster())
        {
            if (Player* target = handler->getSelectedPlayer())
            {
                target->SaveToDB(false, false);
            }
            else
            {
                player->SaveToDB(false, false);
            }

            handler->SendSysMessage(LANG_PLAYER_SAVED);
            return true;
        }

        // save if the player has last been saved over 20 seconds ago
        uint32 saveInterval = sWorld->getIntConfig(CONFIG_INTERVAL_SAVE);
        if (saveInterval == 0 || (saveInterval > 20 * IN_MILLISECONDS && player->GetSaveTimer() <= saveInterval - 20 * IN_MILLISECONDS))
        {
            player->SaveToDB(false, false);
        }

        return true;
    }

    // Save all players in the world
    static bool HandleSaveAllCommand(ChatHandler* handler)
    {
        ObjectAccessor::SaveAllPlayers();
        handler->SendSysMessage(LANG_PLAYERS_SAVED);
        return true;
    }

    static bool HandleUnstuckCommand(ChatHandler* handler, Optional<PlayerIdentifier> target, Optional<std::string_view> location)
    {
        // No args required for players
        if (handler->GetSession() && !handler->GetSession()->IsGameMaster())
        {
            if (Player* player = handler->GetSession()->GetPlayer())
            {
                player->CastSpell(player, SPELL_STUCK, false);
            }

            return true;
        }

        if (!target)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target || !target->IsConnected())
        {
            if (handler->HasLowerSecurity(nullptr, target->GetGUID()))
                return false;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
            stmt->SetData(0, target->GetGUID().GetCounter());

            QueryResult result = CharacterDatabase.Query(stmt);

            if (result)
            {
                Field* fieldsDB = result->Fetch();
                WorldLocation loc(fieldsDB[0].Get<uint16>(), fieldsDB[2].Get<float>(), fieldsDB[3].Get<float>(), fieldsDB[4].Get<float>(), 0.0f);
                uint32 zoneId = fieldsDB[1].Get<uint16>();

                Player::SavePositionInDB(loc, zoneId, target->GetGUID(), nullptr);

                handler->PSendSysMessage(LANG_SUMMONING, target->GetName(), handler->GetNcoreString(LANG_OFFLINE));
            }

            return true;
        }

        Player* player = target->GetConnectedPlayer();

        if (player->IsInFlight() || player->IsInCombat())
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(7355);
            if (!spellInfo)
                return false;

            if (player)
                Spell::SendCastResult(player, spellInfo, 0, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW);

            return false;
        }

        if (location->empty() || *location == "inn")
        {
            player->TeleportTo(player->m_homebindMapId, player->m_homebindX, player->m_homebindY, player->m_homebindZ, player->GetOrientation());
            return true;
        }

        if (*location == "graveyard")
        {
            player->RepopAtGraveyard();
            return true;
        }

        if (*location == "startzone")
        {
            player->TeleportTo(player->GetStartPosition());
            return true;
        }

        //Not a supported argument
        return false;
    }

    static bool HandleLinkGraveCommand(ChatHandler* handler, uint32 graveyardId, Optional<std::string_view> team)
    {
        TeamID teamId;

        if (!team)
        {
            teamId = TEAM_NEUTRAL;
        }
        else if (StringEqualI(team->substr(0, 6), "horde"))
        {
            teamId = TEAM_HORDE;
        }
        else if (StringEqualI(team->substr(0, 9), "alliance"))
        {
            teamId = TEAM_ALLIANCE;
        }
        else
        {
            return false;
        }

        GraveyardStruct const* graveyard = sGraveyard->GetGraveyard(graveyardId);

        if (!graveyard)
        {
            handler->SendErrorMessage(LANG_COMMAND_GRAVEYARDNOEXIST, graveyardId);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        uint32 zoneId = player->GetZoneId();

        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
        if (!areaEntry || areaEntry->Zone != 0)
        {
            handler->SendErrorMessage(LANG_COMMAND_GRAVEYARDWRONGZONE, graveyardId, zoneId);
            return false;
        }

        if (sGraveyard->AddGraveyardLink(graveyardId, zoneId, teamId))
        {
            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDLINKED, graveyardId, zoneId);
        }
        else
        {
            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDALRLINKED, graveyardId, zoneId);
        }

        return true;
    }

    static bool HandleNearGraveCommand(ChatHandler* handler, Optional<std::string_view> team)
    {
        TeamID teamId;

        if (!team)
        {
            teamId = TEAM_NEUTRAL;
        }
        else if (StringEqualI(team->substr(0, 6), "horde"))
        {
            teamId = TEAM_HORDE;
        }
        else if (StringEqualI(team->substr(0, 9), "alliance"))
        {
            teamId = TEAM_ALLIANCE;
        }
        else
        {
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        uint32 zone_id = player->GetZoneId();

        GraveyardStruct const* graveyard = sGraveyard->GetClosestGraveyard(player, teamId);

        if (graveyard)
        {
            uint32 graveyardId = graveyard->ID;

            GraveyardData const* data = sGraveyard->FindGraveyardData(graveyardId, zone_id);
            if (!data)
            {
                handler->SendErrorMessage(LANG_COMMAND_GRAVEYARDERROR, graveyardId);
                return false;
            }

            std::string team_name = handler->GetNcoreString(LANG_COMMAND_GRAVEYARD_NOTEAM);

            if (data->teamId == TEAM_NEUTRAL)
            {
                team_name = handler->GetNcoreString(LANG_COMMAND_GRAVEYARD_ANY);
            }
            else if (data->teamId == TEAM_HORDE)
            {
                team_name = handler->GetNcoreString(LANG_COMMAND_GRAVEYARD_HORDE);
            }
            else if (data->teamId == TEAM_ALLIANCE)
            {
                team_name = handler->GetNcoreString(LANG_COMMAND_GRAVEYARD_ALLIANCE);
            }

            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDNEAREST, graveyardId, team_name, zone_id);
        }
        else
        {
            std::string team_name;

            if (teamId == TEAM_NEUTRAL)
            {
                team_name = handler->GetNcoreString(LANG_COMMAND_GRAVEYARD_ANY);
            }
            else if (teamId == TEAM_HORDE)
            {
                team_name = handler->GetNcoreString(LANG_COMMAND_GRAVEYARD_HORDE);
            }
            else if (teamId == TEAM_ALLIANCE)
            {
                team_name = handler->GetNcoreString(LANG_COMMAND_GRAVEYARD_ALLIANCE);
            }

            handler->PSendSysMessage(LANG_COMMAND_ZONENOGRAFACTION, zone_id, team_name);
        }

        return true;
    }

    static bool HandleShowAreaCommand(ChatHandler* handler, uint32 areaID)
    {
        Player* playerTarget = handler->getSelectedPlayer();
        if (!playerTarget)
        {
            handler->SendErrorMessage(LANG_NO_CHAR_SELECTED);
            return false;
        }

        AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaID);
        if (!area)
        {
            handler->SendErrorMessage(LANG_BAD_VALUE);
            return false;
        }

        int32 offset = area->ExploreFlag / 32;
        if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
        {
            handler->SendErrorMessage(LANG_BAD_VALUE);
            return false;
        }

        uint32 val = uint32((1 << (area->ExploreFlag % 32)));
        uint32 currFields = playerTarget->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);
        playerTarget->SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, uint32((currFields | val)));

        handler->SendSysMessage(LANG_EXPLORE_AREA);
        return true;
    }

    static bool HandleHideAreaCommand(ChatHandler* handler, uint32 areaID)
    {
        Player* playerTarget = handler->getSelectedPlayer();
        if (!playerTarget)
        {
            handler->SendErrorMessage(LANG_NO_CHAR_SELECTED);
            return false;
        }

        AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaID);
        if (!area)
        {
            handler->SendErrorMessage(LANG_BAD_VALUE);
            return false;
        }

        int32 offset = area->ExploreFlag / 32;
        if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
        {
            handler->SendErrorMessage(LANG_BAD_VALUE);
            return false;
        }

        uint32 val = uint32((1 << (area->ExploreFlag % 32)));
        uint32 currFields = playerTarget->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);
        playerTarget->SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, uint32((currFields ^ val)));

        handler->SendSysMessage(LANG_UNEXPLORE_AREA);
        return true;
    }

    static bool HandleAddItemCommand(ChatHandler* handler, Optional<PlayerIdentifier> player, ItemTemplate const* itemTemplate, Optional<int32> _count)
    {
        if (!sObjectMgr->GetItemTemplate(itemTemplate->ItemId))
        {
            handler->SendErrorMessage(LANG_COMMAND_ITEMIDINVALID, itemTemplate->ItemId);
            return false;
        }

        uint32 itemId = itemTemplate->ItemId;
        int32 count = 1;

        if (_count)
            count = *_count;

        if (!count)
            count = 1;

        if (!player)
            player = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!player)
            return false;

        Player* playerTarget = player->GetConnectedPlayer();

        if (!playerTarget)
            return false;

        // Subtract
        if (count < 0)
        {
            // Only have scam check on player accounts
            if (!playerTarget->GetSession()->IsGameMaster())
            {
                if (!playerTarget->HasItemCount(itemId, 0))
                {
                    // output that player don't have any items to destroy
                    handler->SendErrorMessage(LANG_REMOVEITEM_FAILURE, handler->GetNameLink(playerTarget), itemId);
                    return false;
                }

                if (!playerTarget->HasItemCount(itemId, -count))
                {
                    // output that player don't have as many items that you want to destroy
                    handler->SendErrorMessage(LANG_REMOVEITEM_ERROR, handler->GetNameLink(playerTarget), itemId);
                    return false;
                }
            }

            // output successful amount of destroyed items
            playerTarget->DestroyItemCount(itemId, -count, true, false);
            handler->PSendSysMessage(LANG_REMOVEITEM, itemId, -count, handler->GetNameLink(playerTarget));
            return true;
        }

        // Adding items
        uint32 noSpaceForCount = 0;

        // check space and find places
        ItemPosCountVec dest;
        InventoryResult msg = playerTarget->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);

        if (msg != EQUIP_ERR_OK) // convert to possible store amount
            count -= noSpaceForCount;

        if (!count || dest.empty()) // can't add any
        {
            handler->SendErrorMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
            return false;
        }

        Item* item = playerTarget->StoreNewItem(dest, itemId, true);

        Player* p = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        // remove binding (let GM give it to another player later)
        if (p && p == playerTarget)
        {
            for (auto const& itemPos : dest)
            {
                if (Item* item1 = p->GetItemByPos(itemPos.pos))
                {
                    item1->SetBinding(false);
                }
            }
        }

        if (p && count && item)
        {
            p->SendNewItem(item, count, false, true);

            if (p != playerTarget)
            {
                playerTarget->SendNewItem(item, count, true, false);
            }
        }

        if (noSpaceForCount)
            handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);

        return true;
    }

    static bool HandleAddItemSetCommand(ChatHandler* handler, Variant<Hyperlink<itemset>, uint32> itemSetId)
    {
        // prevent generation all items with itemset field value '0'
        if (!*itemSetId)
        {
            handler->SendErrorMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, uint32(itemSetId));
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        Player* playerTarget = handler->getSelectedPlayer();

        if (!playerTarget)
        {
            playerTarget = player;
        }

        bool found = false;

        for (auto const& [itemid, itemTemplate] : *sObjectMgr->GetItemTemplateStore())
        {
            if (itemTemplate.ItemSet == uint32(itemSetId))
            {
                found = true;
                ItemPosCountVec dest;
                InventoryResult msg = playerTarget->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemTemplate.ItemId, 1);

                if (msg == EQUIP_ERR_OK)
                {
                    Item* item = playerTarget->StoreNewItem(dest, itemTemplate.ItemId, true);

                    // remove binding (let GM give it to another player later)
                    if (player == playerTarget)
                    {
                        item->SetBinding(false);
                    }

                    player->SendNewItem(item, 1, false, true);

                    if (player != playerTarget)
                    {
                        playerTarget->SendNewItem(item, 1, true, false);
                    }
                }
                else
                {
                    player->SendEquipError(msg, nullptr, nullptr, itemTemplate.ItemId);
                    handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemTemplate.ItemId, 1);
                }
            }
        }

        if (!found)
        {
            handler->SendErrorMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, uint32(itemSetId));
            return false;
        }

        return true;
    }

    static bool HandleChangeWeather(ChatHandler* handler, uint32 type, float grade)
    {
        // Weather is OFF
        if (!sWorld->getBoolConfig(CONFIG_WEATHER))
        {
            handler->SendErrorMessage(LANG_WEATHER_DISABLED);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        uint32 zoneid = player->GetZoneId();

        Weather* weather = player->GetMap()->GetOrGenerateZoneDefaultWeather(zoneid);
        if (!weather)
        {
            handler->SendErrorMessage(LANG_NO_WEATHER);
            return false;
        }

        weather->SetWeather(WeatherType(type), grade);

        return true;
    }

    static bool HandleMaxSkillCommand(ChatHandler* handler)
    {
        Player* SelectedPlayer = handler->getSelectedPlayer();
        if (!SelectedPlayer)
        {
            handler->SendErrorMessage(LANG_NO_CHAR_SELECTED);
            return false;
        }

        // each skills that have max skill value dependent from level seted to current level max skill value
        SelectedPlayer->UpdateSkillsToMaxSkillsForLevel();
        return true;
    }

    static bool HandleSetSkillCommand(ChatHandler* handler, Variant<Hyperlink<skill>, uint32> skillId, int32 level, Optional<uint16> maxPureSkill)
    {
        uint32 skillID = uint32(skillId);

        if (skillID <= 0)
        {
            handler->SendErrorMessage(LANG_INVALID_SKILL_ID, skillID);
            return false;
        }

        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendErrorMessage(LANG_NO_CHAR_SELECTED);
            return false;
        }

        SkillLineEntry const* skillLine = sSkillLineStore.LookupEntry(skillID);
        if (!skillLine)
        {
            handler->SendErrorMessage(LANG_INVALID_SKILL_ID, uint32(skillID));
            return false;
        }

        bool targetHasSkill = target->GetSkillValue(skillID);

        // If our target does not yet have the skill they are trying to add to them, the chosen level also becomes
        // the max level of the new profession.
        uint16 max = maxPureSkill ? *maxPureSkill : targetHasSkill ? target->GetPureMaxSkillValue(skillID) : uint16(level);

        if (level <= 0 || level > max || max <= 0)
        {
            return false;
        }

        // If the player has the skill, we get the current skill step. If they don't have the skill, we
        // add the skill to the player's book with step 1 (which is the first rank, in most cases something
        // like 'Apprentice <skill>'.
        target->SetSkill(skillID, targetHasSkill ? target->GetSkillStep(skillID) : 1, level, max);
        handler->PSendSysMessage(LANG_SET_SKILL, skillID, skillLine->Name, handler->GetNameLink(target), level, max);
        return true;
    }

    static bool HandleRespawnCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        Unit* target = handler->getSelectedUnit();
        if (player->GetTarget() && target)
        {
            if (!target->IsCreature() || target->IsPet())
            {
                handler->SendErrorMessage(LANG_SELECT_CREATURE);
                return false;
            }

            if (target->isDead())
            {
                target->ToCreature()->Respawn(true);
            }
            return true;
        }

        handler->SendErrorMessage(LANG_SELECT_CREATURE);
        return false;
    }

    static bool HandleRespawnAllCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        Acore::RespawnDo u_do;
        Acore::WorldObjectWorker<Acore::RespawnDo> worker(player, u_do);
        Cell::VisitObjects(player, worker, player->GetGridActivationRange());

        return true;
    }

    static bool HandleMovegensCommand(ChatHandler* handler)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        handler->PSendSysMessage(LANG_MOVEGENS_LIST, (unit->IsPlayer() ? "Player" : "Creature"), unit->GetGUID().ToString());

        MotionMaster* motionMaster = unit->GetMotionMaster();
        float x, y, z;
        motionMaster->GetDestination(x, y, z);

        for (uint8 i = 0; i < MAX_MOTION_SLOT; ++i)
        {
            MovementGenerator* movementGenerator = motionMaster->GetMotionSlot(i);
            if (!movementGenerator)
            {
                handler->SendSysMessage("Empty");
                continue;
            }

            switch (movementGenerator->GetMovementGeneratorType())
            {
                case IDLE_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_IDLE);
                    break;
                case RANDOM_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_RANDOM);
                    break;
                case WAYPOINT_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_WAYPOINT);
                    break;
                case ANIMAL_RANDOM_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_ANIMAL_RANDOM);
                    break;
                case CONFUSED_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_CONFUSED);
                    break;
                case CHASE_MOTION_TYPE:
                {
                    Unit* target = nullptr;
                    if (unit->IsPlayer())
                    {
                        target = static_cast<ChaseMovementGenerator<Player> const*>(movementGenerator)->GetTarget();
                    }
                    else
                    {
                        target = static_cast<ChaseMovementGenerator<Creature> const*>(movementGenerator)->GetTarget();
                    }

                    if (!target)
                    {
                        handler->SendSysMessage(LANG_MOVEGENS_CHASE_NULL);
                    }
                    else if (target->IsPlayer())
                    {
                        handler->PSendSysMessage(LANG_MOVEGENS_CHASE_PLAYER, target->GetName(), target->GetGUID().ToString());
                    }
                    else
                    {
                        handler->PSendSysMessage(LANG_MOVEGENS_CHASE_CREATURE, target->GetName(), target->GetGUID().ToString());
                    }
                    break;
                }
                case FOLLOW_MOTION_TYPE:
                {
                    Unit* target = nullptr;
                    if (unit->IsPlayer())
                    {
                        target = static_cast<FollowMovementGenerator<Player> const*>(movementGenerator)->GetTarget();
                    }
                    else
                    {
                        target = static_cast<FollowMovementGenerator<Creature> const*>(movementGenerator)->GetTarget();
                    }

                    if (!target)
                    {
                        handler->SendSysMessage(LANG_MOVEGENS_FOLLOW_NULL);
                    }
                    else if (target->IsPlayer())
                    {
                        handler->PSendSysMessage(LANG_MOVEGENS_FOLLOW_PLAYER, target->GetName(), target->GetGUID().ToString());
                    }
                    else
                    {
                        handler->PSendSysMessage(LANG_MOVEGENS_FOLLOW_CREATURE, target->GetName(), target->GetGUID().ToString());
                    }
                    break;
                }
                case HOME_MOTION_TYPE:
                {
                    if (unit->IsCreature())
                    {
                        handler->PSendSysMessage(LANG_MOVEGENS_HOME_CREATURE, x, y, z);
                    }
                    else
                    {
                        handler->SendSysMessage(LANG_MOVEGENS_HOME_PLAYER);
                    }
                    break;
                }
                case FLIGHT_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_FLIGHT);
                    break;
                case POINT_MOTION_TYPE:
                {
                    handler->PSendSysMessage(LANG_MOVEGENS_POINT, x, y, z);
                    break;
                }
                case FLEEING_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_FEAR);
                    break;
                case DISTRACT_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_DISTRACT);
                    break;
                case EFFECT_MOTION_TYPE:
                    handler->SendSysMessage(LANG_MOVEGENS_EFFECT);
                    break;
                default:
                    handler->PSendSysMessage(LANG_MOVEGENS_UNKNOWN, movementGenerator->GetMovementGeneratorType());
                    break;
            }
        }
        return true;
    }
    /*
    ComeToMe command REQUIRED for 3rd party scripting library to have access to PointMovementGenerator
    Without this function 3rd party scripting library will get linking errors (unresolved external)
    when attempting to use the PointMovementGenerator
    */
    static bool HandleComeToMeCommand(ChatHandler* handler)
    {
        Creature* caster = handler->getSelectedCreature();
        if (!caster)
        {
            handler->SendErrorMessage(LANG_SELECT_CREATURE);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        caster->GetMotionMaster()->MovePoint(0, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());

        return true;
    }

    static bool HandleDamageCommand(ChatHandler* handler, uint32 damage, Optional<std::string> percent)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target || !handler->GetSession()->GetPlayer()->GetTarget())
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        if (target->IsPlayer())
            if (handler->HasLowerSecurity(target->ToPlayer()))
                return false;

        if (!target->IsAlive() || !damage)
            return true;

        if (percent)
            if (StringStartsWith("pct", *percent))
                if (damage <= 100)
                    damage = target->CountPctFromMaxHealth(damage);

        Unit::DealDamage(handler->GetSession()->GetPlayer(), target, damage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false, true);

        if (target != handler->GetSession()->GetPlayer())
            handler->GetSession()->GetPlayer()->SendAttackStateUpdate(HITINFO_AFFECTS_VICTIM, target, 1, SPELL_SCHOOL_MASK_NORMAL, damage, 0, 0, VICTIMSTATE_HIT, 0);

        return true;
    }

    static bool HandleCombatStopCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target || !target->IsConnected())
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        Player* playerTarget = target->GetConnectedPlayer();

        // check online security
        if (handler->HasLowerSecurity(playerTarget))
        {
            return false;
        }

        playerTarget->CombatStop();
        playerTarget->GetThreatMgr().RemoveMeFromThreatLists();
        return true;
    }

    static bool HandleFlushArenaPointsCommand(ChatHandler* /*handler*/)
    {
        sArenaTeamMgr->DistributeArenaPoints();
        return true;
    }

    static bool HandleFreezeCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        Creature* creatureTarget = handler->getSelectedCreature();

        if (!target && !creatureTarget)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target && !creatureTarget)
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        Player* playerTarget = target->GetConnectedPlayer();
        if (playerTarget && !creatureTarget)
        {
            handler->PSendSysMessage(LANG_COMMAND_FREEZE, target->GetName());

            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_FREEZE))
            {
                Aura::TryRefreshStackOrCreate(spellInfo, MAX_EFFECT_MASK, playerTarget, playerTarget);
            }

            return true;
        }
        if (creatureTarget && creatureTarget->IsAlive())
        {
            handler->PSendSysMessage(LANG_COMMAND_FREEZE, GetCreatureName(creatureTarget));

            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_FREEZE))
                Aura::TryRefreshStackOrCreate(spellInfo, MAX_EFFECT_MASK, creatureTarget, creatureTarget);

            return true;
        }

        handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
        return false;
    }

    static bool HandleUnFreezeCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        Creature* creatureTarget = handler->getSelectedCreature();

        if (!target && !creatureTarget)
        {
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target && !creatureTarget)
        {
            handler->SendErrorMessage(LANG_SELECT_CHAR_OR_CREATURE);
            return false;
        }

        Player* playerTarget = target->GetConnectedPlayer();

        if (!creatureTarget && playerTarget && playerTarget->HasAura(SPELL_FREEZE))
        {
            handler->PSendSysMessage(LANG_COMMAND_UNFREEZE, target->GetName());
            playerTarget->RemoveAurasDueToSpell(SPELL_FREEZE);
            return true;
        }
        else if (creatureTarget && creatureTarget->HasAura(SPELL_FREEZE))
        {
            handler->PSendSysMessage(LANG_COMMAND_UNFREEZE, GetCreatureName(creatureTarget));
            creatureTarget->RemoveAurasDueToSpell(SPELL_FREEZE);
            return true;
        }
        else if (!creatureTarget && target && !target->IsConnected())
        {
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_AURA_FROZEN);
            stmt->SetData(0, target->GetGUID().GetCounter());
            CharacterDatabase.Execute(stmt);
            handler->PSendSysMessage(LANG_COMMAND_UNFREEZE, target->GetName());
            return true;
        }

        handler->SendSysMessage(LANG_COMMAND_FREEZE_WRONG);
        return true;
    }

    static bool HandlePlayAllCommand(ChatHandler* handler, uint32 soundId)
    {
        if (!sSoundEntriesStore.LookupEntry(soundId))
        {
            handler->SendErrorMessage(LANG_SOUND_NOT_EXIST, soundId);
            return false;
        }

        sWorldSessionMgr->SendGlobalMessage(WorldPackets::Misc::Playsound(soundId).Write());

        handler->PSendSysMessage(LANG_COMMAND_PLAYED_TO_ALL, soundId);
        return true;
    }

    static bool HandlePossessCommand(ChatHandler* handler)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            return false;
        }

        handler->GetSession()->GetPlayer()->CastSpell(unit, MAP_OUTLAND, true);
        return true;
    }

    static bool HandleUnPossessCommand(ChatHandler* handler)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            unit = handler->GetSession()->GetPlayer();
        }

        unit->RemoveCharmAuras();
        return true;
    }

    static bool HandleBindSightCommand(ChatHandler* handler)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            return false;
        }

        handler->GetSession()->GetPlayer()->CastSpell(unit, 6277, true);
        return true;
    }

    static bool HandleUnbindSightCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (player->isPossessing())
        {
            return false;
        }

        player->StopCastingBindSight();
        return true;
    }

    static bool HandleMailBoxCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        handler->GetSession()->SendShowMailBox(player->GetGUID());
        return true;
    }

    static bool HandleStringCommand(ChatHandler* handler, uint32 id, Optional<uint8> locale)
    {
        if (!id)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            return false;
        }

        std::string str = sObjectMgr->GetNcoreString(id);
        handler->SendSysMessage(str);
        return true;
    }

    static bool HandleOpenDoorCommand(ChatHandler* handler, Optional<float> range)
    {
        if (GameObject* go = handler->GetPlayer()->FindNearestGameObjectOfType(GAME_OBJECT_TYPE_DOOR, range ? *range : 5.0f))
        {
            go->SetGoState(GO_STATE_ACTIVE);
            handler->PSendSysMessage(LANG_CMD_DOOR_OPENED, go->GetName(), go->GetEntry());
            return true;
        }

        handler->SendErrorMessage(LANG_CMD_NO_DOOR_FOUND, range ? *range : 5.0f);
        return false;
    }

    static bool HandleBMCommand(ChatHandler* handler, Optional<bool> enableArg)
    {
        WorldSession* session = handler->GetSession();

        if (!session)
            return false;

        auto SetBMMod = [&](bool enable)
        {
            char const* enabled = "ON";
            char const* disabled = "OFF";
            handler->SendNotification(LANG_COMMAND_BEASTMASTER_MODE, enable ? enabled : disabled);

            session->GetPlayer()->SetBeastMaster(enable);
        };

        if (!enableArg)
        {
            if (session->IsGameMaster() && session->GetPlayer()->IsDeveloper())
                SetBMMod(true);
            else
                SetBMMod(false);

            return true;
        }

        SetBMMod(*enableArg);
        return true;
    }

    static bool HandlePacketLog(ChatHandler* handler, Optional<PlayerIdentifier> target, Optional<bool> enableArg)
    {
        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target || !target->IsConnected())
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        Player* playerTarget = target->GetConnectedPlayer();
        WorldSession* session = playerTarget->GetSession();

        if (!session)
            return false;

        if (enableArg)
        {
            if (*enableArg)
            {
                session->SetPacketLogging(true);
                handler->PSendSysMessage("Packet logging enabled for {}.", playerTarget->GetName());
                return true;
            }
            else
            {
                session->SetPacketLogging(false);
                handler->PSendSysMessage("Packet logging disabled for {}.", playerTarget->GetName());
                return true;
            }
        }

        handler->SendErrorMessage(LANG_USE_BOL);
        return false;
    }

};

void AddSC_misc_commandscript()
{
    new misc_commandscript();
}
