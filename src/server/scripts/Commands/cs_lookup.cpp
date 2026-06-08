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

#include "CharacterCache.h"
#include "Chat.h"
#include "CommandScript.h"
#include "GameEventMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

using namespace Acore::ChatCommands;

class lookup_commandscript : public CommandScript
{
public:
    lookup_commandscript() : CommandScript("lookup_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable lookupCommandTable =
        {
            { "area",     HandleLookupAreaCommand,         SEC_MODERATOR, Console::Yes  },
            { "creature", HandleLookupCreatureCommand,     SEC_MODERATOR, Console::Yes  },
            { "event",    HandleLookupEventCommand,        SEC_MODERATOR, Console::Yes  },
            { "faction",  HandleLookupFactionCommand,      SEC_MODERATOR, Console::Yes  },
            { "item",     HandleLookupItemCommand,         SEC_MODERATOR, Console::Yes  },
            { "item set", HandleLookupItemSetCommand,      SEC_MODERATOR, Console::Yes  },
            { "map",      HandleLookupMapCommand,          SEC_MODERATOR, Console::Yes  },
            { "object",   HandleLookupObjectCommand,       SEC_MODERATOR, Console::Yes  },
            { "gobject",  HandleLookupObjectCommand,       SEC_MODERATOR, Console::Yes  },
            { "quest",    HandleLookupQuestCommand,        SEC_MODERATOR, Console::Yes  },
            { "skill",    HandleLookupSkillCommand,        SEC_MODERATOR, Console::Yes  },
            { "taxinode", HandleLookupTaxiNodeCommand,     SEC_MODERATOR, Console::Yes  },
            { "teleport", HandleLookupTeleCommand,         SEC_MODERATOR, Console::Yes  },
            { "title",    HandleLookupTitleCommand,        SEC_MODERATOR, Console::Yes  },
            { "spell",    HandleLookupSpellCommand,        SEC_MODERATOR, Console::Yes  },
            { "spell id", HandleLookupSpellIdCommand,      SEC_MODERATOR, Console::Yes  },
        };

        static ChatCommandTable commandTable =
        {
            { "lookup", lookupCommandTable }
        };

        return commandTable;
    }

    static bool HandleLookupAreaCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        // Search in AreaTable.dbc
        for (auto areaEntry : sAreaTableStore)
        {
            int locale = handler->GetSessionDbcLocale();
            std::string name = areaEntry->AreaName;

            if (name.empty())
            {
                continue;
            }

            if (!Utf8FitTo(name, wNamePart))
            {
                locale = 0;
                for (; locale < TOTAL_LOCALES; ++locale)
                {
                    if (locale == handler->GetSessionDbcLocale())
                    {
                        continue;
                    }

                    name = areaEntry->AreaName;
                    if (name.empty())
                    {
                        continue;
                    }

                    if (Utf8FitTo(name, wNamePart))
                    {
                        break;
                    }
                }
            }

            if (locale < TOTAL_LOCALES)
            {
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                // send area in "id - [name]" format
                std::ostringstream ss;
                if (handler->GetSession())
                {
                    ss << areaEntry->ID << " - |cffffffff|Harea:" << areaEntry->ID << "|h[" << name << ' ' << localeNames[locale] << "]|h|r";
                }
                else
                {
                    ss << areaEntry->ID << " - " << name << ' ' << localeNames[locale];
                }

                handler->SendSysMessage(ss.str().c_str());

                if (!found)
                {
                    found = true;
                }
            }
        }

        if (!found)
        {
            handler->SendSysMessage(LANG_COMMAND_NOAREAFOUND);
        }

        return true;
    }

    static bool HandleLookupCreatureCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        std::wstring wNamePart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        for (auto const& [entry, creatureTemplate] : *sObjectMgr->GetCreatureTemplates())
        {
            uint32 id = creatureTemplate.Entry;
            std::string name = creatureTemplate.Name;
            if (name.empty())
                continue;

            if (Utf8FitTo(name, wNamePart))
            {
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_CREATURE_ENTRY_LIST_CHAT, id, id, name);
                else
                    handler->PSendSysMessage(LANG_CREATURE_ENTRY_LIST_CONSOLE, id, name);

                if (!found)
                    found = true;
            }
        }

        if (!found)
            handler->SendSysMessage(LANG_COMMAND_NOCREATUREFOUND);

        return true;
    }

    static bool HandleLookupEventCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
            return false;

        std::wstring wNamePart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namePart, wNamePart))
            return false;

        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
        GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();

        for (uint32 id = 0; id < events.size(); ++id)
        {
            GameEventData const& eventData = events[id];

            std::string descr = eventData.Description;
            if (descr.empty())
                continue;

            if (Utf8FitTo(descr, wNamePart))
            {
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                std::string active = activeEvents.find(id) != activeEvents.end() ? handler->GetNcoreString(LANG_ACTIVE) : "";

                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_EVENT_ENTRY_LIST_CHAT, id, id, eventData.Description, active);
                else
                    handler->PSendSysMessage(LANG_EVENT_ENTRY_LIST_CONSOLE, id, eventData.Description, active);

                if (!found)
                    found = true;
            }
        }

        if (!found)
            handler->SendSysMessage(LANG_NOEVENTFOUND);

        return true;
    }

    static bool HandleLookupFactionCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
            return false;

        // Can be nullptr at console call
        Player* target = handler->getSelectedPlayer();

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        const int locale = handler->GetSessionDbcLocale();

        for (const auto factionEntry : sFactionStore)
        {
            FactionState const* factionState = target ? target->GetReputationMgr().GetState(factionEntry) : nullptr;

            std::string name = factionEntry->Name;
            if (name.empty() || !Utf8FitTo(name, wNamePart))
                continue;

            if (maxResults && count++ == maxResults)
            {
                handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                return true;
            }

            // send faction in "id - [faction] rank reputation [visible] [at war] [own team] [unknown] [invisible] [inactive]" format
            // or              "id - [faction] [no reputation]" format
            std::ostringstream ss;
            if (handler->GetSession())
                ss << factionEntry->ID << " - |cffffffff|Hfaction:" << factionEntry->ID << "|h[" << name << ' ' << localeNames[locale] << "]|h|r";
            else
                ss << factionEntry->ID << " - " << name << ' ' << localeNames[locale];

            if (factionState) // and then target != nullptr also
            {
                const uint32 index = target->GetReputationMgr().GetReputationRankStrIndex(factionEntry);
                std::string rankName = handler->GetNcoreString(index);

                ss << ' ' << rankName << "|h|r (" << target->GetReputationMgr().GetReputation(factionEntry) << ')';

                if (factionState->Flags & FACTION_FLAG_VISIBLE)
                    ss << handler->GetNcoreString(LANG_FACTION_VISIBLE);

                if (factionState->Flags & FACTION_FLAG_AT_WAR)
                    ss << handler->GetNcoreString(LANG_FACTION_ATWAR);

                if (factionState->Flags & FACTION_FLAG_PEACE_FORCED)
                    ss << handler->GetNcoreString(LANG_FACTION_PEACE_FORCED);

                if (factionState->Flags & FACTION_FLAG_HIDDEN)
                    ss << handler->GetNcoreString(LANG_FACTION_HIDDEN);

                if (factionState->Flags & FACTION_FLAG_INVISIBLE_FORCED)
                    ss << handler->GetNcoreString(LANG_FACTION_INVISIBLE_FORCED);

                if (factionState->Flags & FACTION_FLAG_INACTIVE)
                    ss << handler->GetNcoreString(LANG_FACTION_INACTIVE);
            }
            else
                ss << handler->GetNcoreString(LANG_FACTION_NOREPUTATION);

            handler->SendSysMessage(ss.str().c_str());

            if (!found)
                found = true;
        }

        if (!found)
            handler->SendSysMessage(LANG_COMMAND_FACTION_NOTFOUND);

        return true;
    }

    static bool HandleLookupItemCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
            return false;

        std::wstring wNamePart;

        // Converting string that we try to find to lower case
        if (!Utf8toWStr(namePart, wNamePart))
            return false;

        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        // Search in `world_item_template`
        for (const auto& itemTemplate : *sObjectMgr->GetItemTemplateStore() | std::views::values)
        {
            std::string name = itemTemplate.Name1;
            if (name.empty())
                continue;

            if (Utf8FitTo(name, wNamePart))
            {
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                if (handler->GetSession())
                {
                    std::ostringstream color;
                    color << std::hex << ItemQualityColors[itemTemplate.Quality];
                    handler->PSendSysMessage(LANG_ITEM_LIST_CHAT, itemTemplate.ItemId, color.str(), itemTemplate.ItemId, name);
                }
                else
                    handler->PSendSysMessage(LANG_ITEM_LIST_CONSOLE, itemTemplate.ItemId, name);

                if (!found)
                    found = true;
            }
        }

        if (!found)
            handler->SendSysMessage(LANG_COMMAND_NOITEMFOUND);

        return true;
    }

    static bool HandleLookupItemSetCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        // Search in ItemSet.dbc
        for (const ItemSetEntry* set : sItemSetStore)
        {
            const auto localename = localeNames[handler->GetSessionDbcLocale()];
            std::string name = set->Name;
            if (name.empty() || !Utf8FitTo(name, wNamePart))
                continue;

            if (maxResults && count++ == maxResults)
            {
                handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                return true;
            }

            // Send item set in "id - [NamedLink locale]" format
            if (handler->GetSession())
                handler->PSendSysMessage(LANG_ITEMSET_LIST_CHAT, set->ID, set->ID, name, localename);
            else
                handler->PSendSysMessage(LANG_ITEMSET_LIST_CONSOLE, set->ID, name, localename);

            if (!found)
                found = true;
        }

        if (!found)
            handler->SendSysMessage(LANG_COMMAND_NOITEMSETFOUND);

        return true;
    }

    static bool HandleLookupObjectCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
            return false;

        std::wstring wNamePart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namePart, wNamePart))
            return false;

        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        for (const auto& gameObjectTemplate : *sObjectMgr->GetGameObjectTemplates() | std::views::values)
        {
            std::string name = gameObjectTemplate.name;
            if (name.empty())
                continue;

            if (Utf8FitTo(name, wNamePart))
            {
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_GO_ENTRY_LIST_CHAT, gameObjectTemplate.Entry, gameObjectTemplate.Entry, name);
                else
                    handler->PSendSysMessage(LANG_GO_ENTRY_LIST_CONSOLE, gameObjectTemplate.Entry, name);

                if (!found)
                    found = true;
            }
        }

        if (!found)
            handler->SendSysMessage(LANG_COMMAND_NOGAMEOBJECTFOUND);

        return true;
    }

    static bool HandleLookupQuestCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        // can be nullptr at console call
        Player* target = handler->getSelectedPlayer();

        std::wstring wNamePart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        for (const auto& qInfo : sObjectMgr->GetQuestTemplates() | std::views::values)
        {
            std::string title = qInfo->GetTitle();
            if (title.empty())
                continue;

            if (Utf8FitTo(title, wNamePart))
            {
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                std::string statusStr = "";

                if (target)
                {
                    switch (target->GetQuestStatus(qInfo->GetQuestId()))
                    {
                        case QUEST_STATUS_COMPLETE:
                            statusStr = handler->GetNcoreString(LANG_COMPLETE);
                            break;
                        case QUEST_STATUS_INCOMPLETE:
                            statusStr = handler->GetNcoreString(LANG_ACTIVE);
                            break;
                        case QUEST_STATUS_REWARDED:
                            statusStr = handler->GetNcoreString(LANG_REWARDED);
                            break;
                        case QUEST_STATUS_NONE:
                            break;
                        case QUEST_STATUS_FAILED:
                            break;
                        case MAX_QUEST_STATUS:
                            break;
                        default:
                            break;
                    }
                }

                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_QUEST_LIST_CHAT, qInfo->GetQuestId(), qInfo->GetQuestId(), qInfo->GetQuestLevel(), title, statusStr);
                else
                    handler->PSendSysMessage(LANG_QUEST_LIST_CONSOLE, qInfo->GetQuestId(), title, statusStr);

                if (!found)
                    found = true;
            }
        }

        if (!found)
            handler->SendSysMessage(LANG_COMMAND_NOQUESTFOUND);

        return true;
    }

    static bool HandleLookupSkillCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        // Can be nullptr in console call
        const Player* target = handler->getSelectedPlayer();

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        const auto localeName = localeNames[handler->GetSessionDbcLocale()];

        // Search in SkillLine.dbc
        for (const auto skillInfo : sSkillLineStore)
        {
            std::string name = skillInfo->Name;

            if (name.empty())
                continue;

            if (!name.empty() && Utf8FitTo(name, wNamePart))
            {
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                std::string valStr;
                std::string knownStr;

                if (target && target->HasSkill(skillInfo->ID))
                {
                    knownStr = handler->GetNcoreString(LANG_KNOWN);
                    uint32 curValue = target->GetPureSkillValue(skillInfo->ID);
                    uint32 maxValue = target->GetPureMaxSkillValue(skillInfo->ID);
                    uint32 permValue = target->GetSkillPermBonusValue(skillInfo->ID);
                    uint32 tempValue = target->GetSkillTempBonusValue(skillInfo->ID);

                    valStr = Acore::StringFormat(handler->GetNcoreString(LANG_SKILL_VALUES), curValue, maxValue, permValue, tempValue);
                }

                // Send skill in "id - [NamedLink locale]" format
                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_SKILL_LIST_CHAT, skillInfo->ID, skillInfo->ID, name, localeName, knownStr, valStr);
                else
                    handler->PSendSysMessage(LANG_SKILL_LIST_CONSOLE, skillInfo->ID, name, localeName, knownStr, valStr);

                if (!found)
                    found = true;
            }
        }

        if (!found)
        {
            handler->SendSysMessage(LANG_COMMAND_NOSKILLFOUND);
        }

        return true;
    }

    static bool HandleLookupSpellCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        // can be nullptr at console call
        Player* target = handler->getSelectedPlayer();

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        // Search in Spell.dbc
        for (uint32 id = 0; id < sSpellMgr->GetSpellInfoStoreSize(); id++)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(id);
            if (spellInfo)
            {
                const int locale = handler->GetSessionDbcLocale();
                std::string name = spellInfo->SpellName;
                if (name.empty() || !Utf8FitTo(name, wNamePart))
                    continue;
                if (maxResults && count++ == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                const bool known = target && target->HasSpell(id);
                const bool learn = (spellInfo->Effects[0].Effect == SPELL_EFFECT_LEARN_SPELL);

                SpellInfo const* learnSpellInfo = sSpellMgr->GetSpellInfo(spellInfo->Effects[0].TriggerSpell);

                const uint32 talentCost = GetTalentSpellCost(id);

                const bool talent = (talentCost > 0);
                const bool passive = spellInfo->IsPassive();
                const bool active = target && target->HasAura(id);

                // unit32 used to prevent interpreting uint8 as char at output
                // find rank of learned spell for learning spell, or talent rank
                const uint32 rank = talentCost ? talentCost : learn && learnSpellInfo ? learnSpellInfo->GetRank() : spellInfo->GetRank();

                // send spell in "id - [name, rank N] [talent] [passive] [learn] [known]" format
                std::ostringstream ss;
                if (handler->GetSession())
                    ss << id << " - |cffffffff|Hspell:" << id << "|h[" << name;
                else
                    ss << id << " - " << name;

                // Include rank in link name
                if (rank)
                    ss << handler->GetNcoreString(LANG_SPELL_RANK) << rank;

                if (handler->GetSession())
                    ss << ' ' << localeNames[locale] << "]|h|r";
                else
                    ss << ' ' << localeNames[locale];

                if (talent)
                    ss << handler->GetNcoreString(LANG_TALENT);

                if (passive)
                    ss << handler->GetNcoreString(LANG_PASSIVE);

                if (learn)
                    ss << handler->GetNcoreString(LANG_LEARN);

                if (known)
                    ss << handler->GetNcoreString(LANG_KNOWN);

                if (active)
                    ss << handler->GetNcoreString(LANG_ACTIVE);

                handler->SendSysMessage(ss.str().c_str());

                if (!found)
                    found = true;
            }
        }

        if (!found)
        {
            handler->SendSysMessage(LANG_COMMAND_NOSPELLFOUND);
        }

        return true;
    }

    static bool HandleLookupSpellIdCommand(ChatHandler* handler, SpellInfo const* spell)
    {
        // can be nullptr at console call
        Player* target = handler->getSelectedPlayer();

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = 1;

        if (!SpellMgr::IsSpellValid(spell))
        {
            handler->SendErrorMessage(LANG_COMMAND_SPELL_BROKEN, spell->ID);
            return false;
        }

        int locale = handler->GetSessionDbcLocale();
        std::string name = spell->SpellName;
        if (name.empty())
        {
            handler->SendSysMessage(LANG_COMMAND_NOSPELLFOUND);
            return true;
        }

        if (locale < TOTAL_LOCALES)
        {
            if (maxResults && count++ == maxResults)
            {
                handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                return true;
            }

            bool known = target && target->HasSpell(spell->ID);
            bool learn = (spell->Effects[0].Effect == SPELL_EFFECT_LEARN_SPELL);

            SpellInfo const* learnSpellInfo = sSpellMgr->GetSpellInfo(spell->Effects[0].TriggerSpell);

            uint32 talentCost = GetTalentSpellCost(spell->ID);

            bool talent = (talentCost > 0);
            bool passive = spell->IsPassive();
            bool active = target && target->HasAura(spell->ID);

            // unit32 used to prevent interpreting uint8 as char at output
            // find rank of learned spell for learning spell, or talent rank
            uint32 rank = talentCost ? talentCost : learn && learnSpellInfo ? learnSpellInfo->GetRank() : spell->GetRank();

            // send spell in "id - [name, rank N] [talent] [passive] [learn] [known]" format
            std::ostringstream ss;
            if (handler->GetSession())
            {
                ss << spell->ID << " - |cffffffff|Hspell:" << spell->ID << "|h[" << name;
            }
            else
            {
                ss << spell->ID << " - " << name;
            }

            // include rank in link name
            if (rank)
            {
                ss << handler->GetNcoreString(LANG_SPELL_RANK) << rank;
            }

            if (handler->GetSession())
            {
                ss << ' ' << localeNames[locale] << "]|h|r";
            }
            else
            {
                ss << ' ' << localeNames[locale];
            }

            if (talent)
            {
                ss << handler->GetNcoreString(LANG_TALENT);
            }

            if (passive)
            {
                ss << handler->GetNcoreString(LANG_PASSIVE);
            }

            if (learn)
            {
                ss << handler->GetNcoreString(LANG_LEARN);
            }

            if (known)
            {
                ss << handler->GetNcoreString(LANG_KNOWN);
            }

            if (active)
            {
                ss << handler->GetNcoreString(LANG_ACTIVE);
            }

            handler->SendSysMessage(ss.str().c_str());

            if (!found)
            {
                found = true;
            }
        }

        if (!found)
        {
            handler->SendSysMessage(LANG_COMMAND_NOSPELLFOUND);
        }

        return true;
    }

    static bool HandleLookupTaxiNodeCommand(ChatHandler* handler, const Tail namePart)
    {
        if (namePart.empty())
            return false;

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        bool found = false;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        const int locale = handler->GetSessionDbcLocale();

        // Search in TaxiNodes.dbc
        for (const auto nodeEntry : sTaxiNodesStore)
        {

            std::string name = nodeEntry->Name;

            if (name.empty() || !Utf8FitTo(name, wNamePart))
                continue;

            if (maxResults && count++ == maxResults)
            {
                handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                return true;
            }

            // Send taxi node in "id - [name] (Map:m X:x Y:y Z:z)" format
            if (handler->GetSession())
                handler->PSendSysMessage(LANG_TAXINODE_ENTRY_LIST_CHAT, nodeEntry->ID, nodeEntry->ID, name, localeNames[locale],
                                         nodeEntry->MapID, nodeEntry->X, nodeEntry->Y, nodeEntry->Z);
            else
                handler->PSendSysMessage(LANG_TAXINODE_ENTRY_LIST_CONSOLE, nodeEntry->ID, name, localeNames[locale],
                                         nodeEntry->MapID, nodeEntry->X, nodeEntry->Y, nodeEntry->Z);

            if (!found)
                found = true;
        }
        if (!found)
            handler->SendSysMessage(LANG_COMMAND_NOTAXINODEFOUND);

        return true;
    }

    // Find teleport in game_tele order by name
    static bool HandleLookupTeleCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        std::ostringstream reply;
        uint32 count = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);
        bool limitReached = false;

        for (auto const& [id, tele] : sObjectMgr->GetGameTeleMap())
        {
            if (tele.WNameLow.find(wNamePart) == std::wstring::npos)
            {
                continue;
            }

            if (maxResults && count++ == maxResults)
            {
                limitReached = true;
                break;
            }

            if (handler->GetSession())
            {
                reply << "  |cffffffff|Htele:" << id << "|h[" << tele.Name << "]|h|r\n";
            }
            else
            {
                reply << "  " << id << ' ' << tele.Name << "\n";
            }
        }

        if (reply.str().empty())
        {
            handler->SendSysMessage(LANG_COMMAND_TELE_NOLOCATION);
        }
        else
        {
            handler->PSendSysMessage(LANG_COMMAND_TELE_LOCATION, reply.str());
        }

        if (limitReached)
        {
            handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
        }

        return true;
    }

    static bool HandleLookupTitleCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        // can be nullptr in console call
        Player* target = handler->getSelectedPlayer();

        // title name have single string arg for player name
        char const* targetName = target ? target->GetName().c_str() : "NAME";

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        // converting string that we try to find to lower case
        wstrToLower(wNamePart);

        uint32 counter = 0;                                     // Counter for figure out that we found smth.
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        // Search in CharTitles.dbc
        for (auto titleInfo : sCharTitlesStore)
        {
            int locale = handler->GetSessionDbcLocale();
            std::string name = titleInfo->NameMale;
            if (name.empty())
            {
                continue;
            }

            if (!Utf8FitTo(name, wNamePart))
            {
                locale = 0;
                for (; locale < TOTAL_LOCALES; ++locale)
                {
                    if (locale == handler->GetSessionDbcLocale())
                    {
                        continue;
                    }

                    name = titleInfo->NameMale;
                    if (name.empty())
                        continue;

                    if (Utf8FitTo(name, wNamePart))
                        break;
                }
            }

            if (locale < TOTAL_LOCALES)
            {
                if (maxResults && counter == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                std::string knownStr = target && target->HasTitle(titleInfo) ? handler->GetNcoreString(LANG_KNOWN) : "";
                std::string activeStr = target && target->GetUInt32Value(PLAYER_CHOSEN_TITLE) == titleInfo->BitIndex ? handler->GetNcoreString(LANG_ACTIVE) : "";

                std::string titleNameStr = Acore::StringFormat(name, targetName);

                // send title in "id (idx:idx) - [namedlink locale]" format
                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_TITLE_LIST_CHAT, titleInfo->ID, titleInfo->BitIndex, titleInfo->ID, titleNameStr, localeNames[locale], knownStr, activeStr);
                else
                    handler->PSendSysMessage(LANG_TITLE_LIST_CONSOLE, titleInfo->ID, titleInfo->BitIndex, titleNameStr, localeNames[locale], knownStr, activeStr);

                ++counter;
            }
        }

        if (!counter)  // if counter == 0 then we found nth
        {
            handler->SendSysMessage(LANG_COMMAND_NOTITLEFOUND);
        }

        return true;
    }

    static bool HandleLookupMapCommand(ChatHandler* handler, Tail namePart)
    {
        if (namePart.empty())
        {
            return false;
        }

        std::wstring wNamePart;

        if (!Utf8toWStr(namePart, wNamePart))
        {
            return false;
        }

        wstrToLower(wNamePart);

        uint32 counter = 0;
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);
        uint8 locale = handler->GetSession() ? handler->GetSession()->GetSessionDbcLocale() : sWorld->GetDefaultDbcLocale();

        // search in Map.dbc
        for (auto mapInfo : sMapStore)
        {
            std::string name = mapInfo->Name;

            if (name.empty())
                continue;

            if (Utf8FitTo(name, wNamePart) && locale < TOTAL_LOCALES)
            {
                if (maxResults && counter == maxResults)
                {
                    handler->PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                std::ostringstream ss;
                ss << mapInfo->ID << " - [" << name << ']';

                if (mapInfo->IsContinent())
                {
                    ss << handler->GetNcoreString(LANG_CONTINENT);
                }

                switch (mapInfo->MapType)
                {
                    case MAP_INSTANCE:
                        ss << handler->GetNcoreString(LANG_INSTANCE);
                        break;
                    case MAP_RAID:
                        ss << handler->GetNcoreString(LANG_RAID);
                        break;
                    case MAP_BATTLEGROUND:
                        ss << handler->GetNcoreString(LANG_BATTLEGROUND);
                        break;
                    case MAP_ARENA:
                        ss << handler->GetNcoreString(LANG_ARENA);
                        break;
                }

                handler->SendSysMessage(ss.str().c_str());

                ++counter;
            }
        }

        if (!counter)
        {
            handler->SendSysMessage(LANG_COMMAND_NOMAPFOUND);
        }

        return true;
    }
};

void AddSC_lookup_commandscript()
{
    new lookup_commandscript();
}
