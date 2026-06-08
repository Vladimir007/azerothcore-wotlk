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

#ifndef GAMEOBJECTDATA_H
#define GAMEOBJECTDATA_H

#include "SharedDefines.h"
#include "SpawnData.h"
#include <array>
#include <vector>
#include <string>

#define MAX_GAMEOBJECT_QUEST_ITEMS 6
#define MAX_GO_STATE               3

 // from `gameobject_template`
struct GameObjectTemplate
{
    uint32 Entry;
    uint32 Type;
    uint32 DisplayID;
    std::string name;
    std::string IconName;
    std::string CastBarCaption;
    std::string AlertText;
    float   Size;
    union                                                   // different GO types have different data field
    {
        //0 GAME_OBJECT_TYPE_DOOR
        struct
        {
            uint32 startOpen;                               //0 used client side to determine GO_ACTIVATED means open/closed
            uint32 lockId;                                  //1 -> Lock.dbc
            uint32 autoCloseTime;                           //2 secs till autoclose = autoCloseTime / 0x10000
            uint32 noDamageImmune;                          //3 break opening whenever you recieve damage?
            uint32 openTextID;                              //4 can be used to replace castBarCaption?
            uint32 closeTextID;                             //5
            uint32 ignoredByPathing;                        //6
        } Door;
        //1 GAME_OBJECT_TYPE_BUTTON
        struct
        {
            uint32 startOpen;                               //0
            uint32 lockId;                                  //1 -> Lock.dbc
            uint32 autoCloseTime;                           //2 secs till autoclose = autoCloseTime / 0x10000
            uint32 linkedTrap;                              //3
            uint32 noDamageImmune;                          //4 isBattlegroundObject
            uint32 large;                                   //5
            uint32 openTextID;                              //6 can be used to replace castBarCaption?
            uint32 closeTextID;                             //7
            uint32 losOK;                                   //8
        } Button;
        //2 GAME_OBJECT_TYPE_QUESTGIVER
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 questList;                               //1
            uint32 pageMaterial;                            //2
            uint32 gossipID;                                //3
            uint32 customAnim;                              //4
            uint32 noDamageImmune;                          //5
            uint32 openTextID;                              //6 can be used to replace castBarCaption?
            uint32 losOK;                                   //7
            uint32 allowMounted;                            //8
            uint32 large;                                   //9
        } QuestGiver;
        //3 GAME_OBJECT_TYPE_CHEST
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 lootId;                                  //1
            uint32 chestRestockTime;                        //2
            uint32 consumable;                              //3
            uint32 minSuccessOpens;                         //4 Deprecated, pre 3.0 was used for mining nodes but since WotLK all mining nodes are usable once and grant all loot with a single use
            uint32 maxSuccessOpens;                         //5 Deprecated, pre 3.0 was used for mining nodes but since WotLK all mining nodes are usable once and grant all loot with a single use
            uint32 eventId;                                 //6 lootedEvent
            uint32 linkedTrapId;                            //7
            uint32 questId;                                 //8 not used currently but store quest required for GO activation for player
            uint32 level;                                   //9
            uint32 losOK;                                   //10
            uint32 leaveLoot;                               //11
            uint32 notInCombat;                             //12
            uint32 logLoot;                                 //13
            uint32 openTextID;                              //14 can be used to replace castBarCaption?
            uint32 groupLootRules;                          //15
            uint32 floatingTooltip;                         //16
        } Chest;
        //4 GAME_OBJECT_TYPE_BINDER - empty
        //5 GAME_OBJECT_TYPE_GENERIC
        struct
        {
            uint32 floatingTooltip;                         //0
            uint32 highlight;                               //1
            uint32 serverOnly;                              //2
            uint32 large;                                   //3
            uint32 floatOnWater;                            //4
            int32 questID;                                  //5
        } Generic;
        //6 GAME_OBJECT_TYPE_TRAP
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 level;                                   //1
            uint32 diameter;                                //2 diameter for trap activation
            uint32 spellId;                                 //3
            uint32 type;                                    //4 0 trap with no despawn after cast. 1 trap despawns after cast. 2 bomb casts on spawn.
            uint32 cooldown;                                //5 time in secs
            int32 autoCloseTime;                            //6
            uint32 startDelay;                              //7
            uint32 serverOnly;                              //8
            uint32 stealthed;                               //9
            uint32 large;                                   //10
            uint32 invisible;                               //11
            uint32 openTextID;                              //12 can be used to replace castBarCaption?
            uint32 closeTextID;                             //13
            uint32 ignoreTotems;                            //14
        } Trap;
        //7 GAME_OBJECT_TYPE_CHAIR
        struct
        {
            uint32 slots;                                   //0
            uint32 height;                                  //1
            uint32 onlyCreatorUse;                          //2
            uint32 triggeredEvent;                          //3
        } Chair;
        //8 GAME_OBJECT_TYPE_SPELL_FOCUS
        struct
        {
            uint32 focusId;                                 //0
            uint32 dist;                                    //1
            uint32 linkedTrapId;                            //2
            uint32 serverOnly;                              //3
            uint32 questID;                                 //4
            uint32 large;                                   //5
            uint32 floatingTooltip;                         //6
        } SpellFocus;
        //9 GAME_OBJECT_TYPE_TEXT
        struct
        {
            uint32 pageID;                                  //0
            uint32 language;                                //1
            uint32 pageMaterial;                            //2
            uint32 allowMounted;                            //3
        } Text;
        //10 GAME_OBJECT_TYPE_GOOBER
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            int32 questId;                                  //1
            uint32 eventId;                                 //2
            uint32 autoCloseTime;                           //3
            uint32 customAnim;                              //4
            uint32 consumable;                              //5
            uint32 cooldown;                                //6
            uint32 pageId;                                  //7
            uint32 language;                                //8
            uint32 pageMaterial;                            //9
            uint32 spellId;                                 //10
            uint32 noDamageImmune;                          //11
            uint32 linkedTrapId;                            //12
            uint32 large;                                   //13
            uint32 openTextID;                              //14 can be used to replace castBarCaption?
            uint32 closeTextID;                             //15
            uint32 losOK;                                   //16 isBattlegroundObject
            uint32 allowMounted;                            //17
            uint32 floatingTooltip;                         //18
            uint32 gossipID;                                //19
            uint32 WorldStateSetsState;                     //20
        } Goober;
        //11 GAME_OBJECT_TYPE_TRANSPORT
        struct
        {
            uint32 pauseAtTime;                             //0
            uint32 startOpen;                               //1
            uint32 autoCloseTime;                           //2 secs till autoclose = autoCloseTime / 0x10000
            uint32 pause1EventID;                           //3
            uint32 pause2EventID;                           //4
        } Transport;
        //12 GAME_OBJECT_TYPE_AREADAMAGE
        struct
        {
            uint32 lockId;                                  //0
            uint32 radius;                                  //1
            uint32 damageMin;                               //2
            uint32 damageMax;                               //3
            uint32 damageSchool;                            //4
            uint32 autoCloseTime;                           //5 secs till autoclose = autoCloseTime / 0x10000
            uint32 openTextID;                              //6
            uint32 closeTextID;                             //7
        } AreaDamage;
        //13 GAME_OBJECT_TYPE_CAMERA
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 cinematicId;                             //1
            uint32 eventID;                                 //2
            uint32 openTextID;                              //3 can be used to replace castBarCaption?
        } Camera;
        //14 GAME_OBJECT_TYPE_MAPOBJECT - empty
        //15 GAME_OBJECT_TYPE_MO_TRANSPORT
        struct
        {
            uint32 taxiPathId;                              //0
            uint32 moveSpeed;                               //1
            uint32 accelRate;                               //2
            uint32 startEventID;                            //3
            uint32 stopEventID;                             //4
            uint32 transportPhysics;                        //5
            uint32 mapID;                                   //6
            uint32 worldState1;                             //7
            uint32 canBeStopped;                            //8
        } MOTransport;
        //16 GAME_OBJECT_TYPE_DUELFLAG - empty
        //17 GAME_OBJECT_TYPE_FISHINGNODE - empty
        //18 GAME_OBJECT_TYPE_SUMMONING_RITUAL
        struct
        {
            uint32 reqParticipants;                         //0
            uint32 spellId;                                 //1
            uint32 animSpell;                               //2
            uint32 ritualPersistent;                        //3
            uint32 casterTargetSpell;                       //4
            uint32 casterTargetSpellTargets;                //5
            uint32 castersGrouped;                          //6
            uint32 ritualNoTargetCheck;                     //7
        } SummoningRitual;
        //19 GAME_OBJECT_TYPE_MAILBOX - empty
        //20 GAME_OBJECT_TYPE_DONOTUSE - empty
        //21 GAME_OBJECT_TYPE_GUARDPOST
        struct
        {
            uint32 creatureID;                              //0
            uint32 charges;                                 //1
        } GuardPost;
        //22 GAME_OBJECT_TYPE_SPELLCASTER
        struct
        {
            uint32 spellId;                                 //0
            uint32 charges;                                 //1
            uint32 partyOnly;                               //2
            uint32 allowMounted;                            //3
            uint32 large;                                   //4
        } SpellCaster;
        //23 GAME_OBJECT_TYPE_MEETINGSTONE
        struct
        {
            uint32 minLevel;                                //0
            uint32 maxLevel;                                //1
            uint32 areaID;                                  //2
        } MeetingStone;
        //24 GAME_OBJECT_TYPE_FLAGSTAND
        struct
        {
            uint32 lockId;                                  //0
            uint32 pickupSpell;                             //1
            uint32 radius;                                  //2
            uint32 returnAura;                              //3
            uint32 returnSpell;                             //4
            uint32 noDamageImmune;                          //5
            uint32 openTextID;                              //6
            uint32 losOK;                                   //7
        } Flagstand;
        //25 GAME_OBJECT_TYPE_FISHINGHOLE
        struct
        {
            uint32 radius;                                  //0 how close bobber must land for sending loot
            uint32 lootId;                                  //1
            uint32 minSuccessOpens;                         //2
            uint32 maxSuccessOpens;                         //3
            uint32 lockId;                                  //4 -> Lock.dbc; possibly 1628 for all?
        } FishingHole;
        //26 GAME_OBJECT_TYPE_FLAGDROP
        struct
        {
            uint32 lockId;                                  //0
            uint32 eventID;                                 //1
            uint32 pickupSpell;                             //2
            uint32 noDamageImmune;                          //3
            uint32 openTextID;                              //4
        } FlagDrop;
        //27 GAME_OBJECT_TYPE_MINI_GAME
        struct
        {
            uint32 gameType;                                //0
        } MiniGame;
        //29 GAME_OBJECT_TYPE_CAPTURE_POINT
        struct
        {
            uint32 radius;                                  //0
            uint32 spell;                                   //1
            uint32 worldState1;                             //2
            uint32 worldstate2;                             //3
            uint32 winEventID1;                             //4
            uint32 winEventID2;                             //5
            uint32 contestedEventID1;                       //6
            uint32 contestedEventID2;                       //7
            uint32 progressEventID1;                        //8
            uint32 progressEventID2;                        //9
            uint32 neutralEventID1;                         //10
            uint32 neutralEventID2;                         //11
            uint32 neutralPercent;                          //12
            uint32 worldstate3;                             //13
            uint32 minSuperiority;                          //14
            uint32 maxSuperiority;                          //15
            uint32 minTime;                                 //16
            uint32 maxTime;                                 //17
            uint32 large;                                   //18
            uint32 highlight;                               //19
            uint32 startingValue;                           //20
            uint32 unidirectional;                          //21
        } CapturePoint;
        //30 GAME_OBJECT_TYPE_AURA_GENERATOR
        struct
        {
            uint32 startOpen;                               //0
            uint32 radius;                                  //1
            uint32 auraID1;                                 //2
            uint32 conditionID1;                            //3
            uint32 auraID2;                                 //4
            uint32 conditionID2;                            //5
            uint32 serverOnly;                              //6
        } AuraGenerator;
        //31 GAME_OBJECT_TYPE_DUNGEON_DIFFICULTY
        struct
        {
            uint32 mapID;                                   //0
            uint32 difficulty;                              //1
        } DungeonDifficulty;
        //32 GAME_OBJECT_TYPE_BARBER_CHAIR
        struct
        {
            uint32 chairHeight;                             //0
            uint32 heightOffset;                            //1
        } BarberChair;
        //33 GAME_OBJECT_TYPE_DESTRUCTIBLE_BUILDING
        struct
        {
            uint32 intactNumHits;                           //0
            uint32 creditProxyCreature;                     //1
            uint32 state1Name;                              //2
            uint32 intactEvent;                             //3
            uint32 damagedDisplayId;                        //4
            uint32 damagedNumHits;                          //5
            uint32 empty3;                                  //6
            uint32 empty4;                                  //7
            uint32 empty5;                                  //8
            uint32 damagedEvent;                            //9
            uint32 destroyedDisplayId;                      //10
            uint32 empty7;                                  //11
            uint32 empty8;                                  //12
            uint32 empty9;                                  //13
            uint32 destroyedEvent;                          //14
            uint32 empty10;                                 //15
            uint32 debuildingTimeSecs;                      //16
            uint32 empty11;                                 //17
            uint32 destructibleData;                        //18
            uint32 rebuildingEvent;                         //19
            uint32 empty12;                                 //20
            uint32 empty13;                                 //21
            uint32 damageEvent;                             //22
            uint32 empty14;                                 //23
        } Building;
        //34 GAME_OBJECT_TYPE_GUILDBANK - empty
        //35 GAME_OBJECT_TYPE_TRAPDOOR
        struct
        {
            uint32 whenToPause;                             // 0
            uint32 startOpen;                               // 1
            uint32 autoClose;                               // 2
        } TrapDoor;

        // not use for specific field access (only for output with loop by all filed), also this determinate max union size
        struct
        {
            uint32 data[MAX_GAME_OBJECT_DATA];
        } Raw;
    };

    std::string AIName;
    uint32 ScriptID;
    bool IsForQuests; // pussywizard

    // helpers
    [[nodiscard]] bool IsDespawnAtAction() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_CHEST:
            return Chest.consumable;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.consumable;
        default:
            return false;
        }
    }

    [[nodiscard]] bool IsUsableMounted() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            return QuestGiver.allowMounted;
        case GAME_OBJECT_TYPE_TEXT:
            return Text.allowMounted;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.allowMounted;
        case GAME_OBJECT_TYPE_SPELLCASTER:
            return SpellCaster.allowMounted;
        default:
            return false;
        }
    }

    [[nodiscard]] uint32 GetLockId() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_DOOR:
            return Door.lockId;
        case GAME_OBJECT_TYPE_BUTTON:
            return Button.lockId;
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            return QuestGiver.lockId;
        case GAME_OBJECT_TYPE_CHEST:
            return Chest.lockId;
        case GAME_OBJECT_TYPE_TRAP:
            return Trap.lockId;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.lockId;
        case GAME_OBJECT_TYPE_AREA_DAMAGE:
            return AreaDamage.lockId;
        case GAME_OBJECT_TYPE_CAMERA:
            return Camera.lockId;
        case GAME_OBJECT_TYPE_FLAGSTAND:
            return Flagstand.lockId;
        case GAME_OBJECT_TYPE_FISHING_HOLE:
            return FishingHole.lockId;
        case GAME_OBJECT_TYPE_FLAG_DROP:
            return FlagDrop.lockId;
        default:
            return 0;
        }
    }

    [[nodiscard]] bool GetDespawnPossibility() const                      // despawn at targeting of cast?
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_DOOR:
            return Door.noDamageImmune;
        case GAME_OBJECT_TYPE_BUTTON:
            return Button.noDamageImmune;
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            return QuestGiver.noDamageImmune;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.noDamageImmune;
        case GAME_OBJECT_TYPE_FLAGSTAND:
            return Flagstand.noDamageImmune;
        case GAME_OBJECT_TYPE_FLAG_DROP:
            return FlagDrop.noDamageImmune;
        default:
            return true;
        }
    }

    [[nodiscard]] uint32 GetCharges() const                               // despawn at uses amount
    {
        switch (Type)
        {
            //case GAME_OBJECT_TYPE_TRAP:        return trap.charges;
        case GAME_OBJECT_TYPE_GUARDPOST:
            return GuardPost.charges;
        case GAME_OBJECT_TYPE_SPELLCASTER:
            return SpellCaster.charges;
        default:
            return 0;
        }
    }

    [[nodiscard]] uint32 GetLinkedGameObjectEntry() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_BUTTON:
            return Button.linkedTrap;
        case GAME_OBJECT_TYPE_CHEST:
            return Chest.linkedTrapId;
        case GAME_OBJECT_TYPE_SPELL_FOCUS:
            return SpellFocus.linkedTrapId;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.linkedTrapId;
        default:
            return 0;
        }
    }

    [[nodiscard]] uint32 GetAutoCloseTime() const
    {
        uint32 autoCloseTime = 0;
        switch (Type)
        {
        case GAME_OBJECT_TYPE_DOOR:
            autoCloseTime = Door.autoCloseTime;
            break;
        case GAME_OBJECT_TYPE_BUTTON:
            autoCloseTime = Button.autoCloseTime;
            break;
        case GAME_OBJECT_TYPE_TRAP:
            autoCloseTime = Trap.autoCloseTime;
            break;
        case GAME_OBJECT_TYPE_GOOBER:
            autoCloseTime = Goober.autoCloseTime;
            break;
        case GAME_OBJECT_TYPE_TRANSPORT:
            autoCloseTime = Transport.autoCloseTime;
            break;
        case GAME_OBJECT_TYPE_AREA_DAMAGE:
            autoCloseTime = AreaDamage.autoCloseTime;
            break;
        default:
            break;
        }
        return autoCloseTime /* xinef: changed to milliseconds/ IN_MILLISECONDS*/;              // prior to 3.0.3, conversion was / 0x10000;
    }

    [[nodiscard]] uint32 GetLootId() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_CHEST:
            return Chest.lootId;
        case GAME_OBJECT_TYPE_FISHING_HOLE:
            return FishingHole.lootId;
        default:
            return 0;
        }
    }

    [[nodiscard]] uint32 GetGossipMenuId() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            return QuestGiver.gossipID;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.gossipID;
        default:
            return 0;
        }
    }

    [[nodiscard]] uint32 GetEventScriptId() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.eventId;
        case GAME_OBJECT_TYPE_CHEST:
            return Chest.eventId;
        case GAME_OBJECT_TYPE_CAMERA:
            return Camera.eventID;
        default:
            return 0;
        }
    }

    [[nodiscard]] uint32 GetCooldown() const                              // Cooldown preventing goober and traps to cast spell
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_TRAP:
            return Trap.cooldown;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.cooldown;
        default:
            return 0;
        }
    }

    [[nodiscard]] bool IsLargeGameObject() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_BUTTON:
            return Button.large != 0;
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            return QuestGiver.large != 0;
        case GAME_OBJECT_TYPE_GENERIC:
            return Generic.large != 0;
        case GAME_OBJECT_TYPE_TRAP:
            return Trap.large != 0;
        case GAME_OBJECT_TYPE_SPELL_FOCUS:
            return SpellFocus.large != 0;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.large != 0;
        case GAME_OBJECT_TYPE_SPELLCASTER:
            return SpellCaster.large != 0;
        case GAME_OBJECT_TYPE_CAPTURE_POINT:
            return CapturePoint.large != 0;
        default:
            return false;
        }
    }

    [[nodiscard]] bool IsInfiniteGameObject() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_DOOR:
            return true;
        case GAME_OBJECT_TYPE_FLAGSTAND:
            return true;
        case GAME_OBJECT_TYPE_FLAG_DROP:
            return true;
        case GAME_OBJECT_TYPE_DUNGEON_DIFFICULTY:
            return true;
        case GAME_OBJECT_TYPE_TRAPDOOR:
            return true;
        case GAME_OBJECT_TYPE_DESTRUCTIBLE_BUILDING:
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] bool IsGameObjectForQuests() const
    {
        return IsForQuests;
    }

    [[nodiscard]] bool IsIgnoringLOSChecks() const
    {
        switch (Type)
        {
        case GAME_OBJECT_TYPE_BUTTON:
            return Button.losOK == 0;
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            return QuestGiver.losOK == 0;
        case GAME_OBJECT_TYPE_CHEST:
            return Chest.losOK == 0;
        case GAME_OBJECT_TYPE_GOOBER:
            return Goober.losOK == 0;
        case GAME_OBJECT_TYPE_FLAGSTAND:
            return Flagstand.losOK == 0;
        default:
            return false;
        }
    }
};

// From `gameobject_template_addon`
struct GameObjectTemplateAddon
{
    uint32  entry;
    uint32  faction;
    uint32  flags;
    uint32  minGold;
    uint32  maxGold;
    std::array<uint32, 4> artKits = {};
};

struct AC_GAME_API QuaternionData
{
    float x;
    float y;
    float z;
    float w;

    QuaternionData() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) { }
    QuaternionData(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) { }

    [[nodiscard]] bool IsUnit() const;
    void ToEulerAnglesZYX(float& Z, float& Y, float& X) const;
    [[nodiscard]] static QuaternionData FromEulerAnglesZYX(float Z, float Y, float X);
};

// `gameobject_addon` table
struct GameObjectAddon
{
    QuaternionData parentRotation;
    InvisibilityType invisibilityType;
    uint32 invisibilityValue;
};

// client side GO show states
enum GOState
{
    GO_STATE_ACTIVE = 0,                        // show in world as used and not reset (closed door open)
    GO_STATE_READY = 1,                        // show in world as ready (closed door close)
    GO_STATE_ACTIVE_ALTERNATIVE = 2                         // show in world as used in alt way and not reset (closed door open by cannon fire)
};

// from `gameobject`
struct GameObjectData : public SpawnData
{
    GameObjectData() : SpawnData(SPAWN_TYPE_GAMEOBJECT) {}
    uint32 id{0};                                                // entry in gameobject_template
    G3D::Quat rotation;
    int32 spawnTimeSecs{0};
    uint32 animProgress{0};
    GOState goState{GO_STATE_ACTIVE};
    uint8 artKit{0};
};

#endif // GameObjectData_h__
