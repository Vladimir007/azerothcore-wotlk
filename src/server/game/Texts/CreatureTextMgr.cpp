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

#include "CreatureTextMgr.h"
#include "Cell.h"
#include "CellImpl.h"
#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "GridNotifiers.h"
#include "MiscPackets.h"
#include "ObjectMgr.h"

class CreatureTextBuilder
{
public:
    CreatureTextBuilder(WorldObject* obj, uint8 gender, ChatMsg msgtype, uint8 textGroup, uint32 id, uint32 language, WorldObject const* target)
        : _source(obj), _gender(gender), _msgType(msgtype), _textGroup(textGroup), _textId(id), _language(language), _target(target) { }

    std::size_t operator()(WorldPacket* data, LocaleConstant locale) const
    {
        std::string const& text = sCreatureTextMgr->GetLocalizedChatString(_source->GetEntry(), _gender, _textGroup, _textId, locale);

        return ChatHandler::BuildChatPacket(*data, _msgType, Language(_language), _source, _target, text);
    }

private:
    WorldObject* _source;
    uint8 _gender;
    ChatMsg _msgType;
    uint8 _textGroup;
    uint32 _textId;
    uint32 _language;
    WorldObject const* _target;
};

class PlayerTextBuilder
{
public:
    PlayerTextBuilder(WorldObject* obj, WorldObject* speaker, uint8 gender, ChatMsg msgtype, uint8 textGroup, uint32 id, uint32 language, WorldObject const* target)
        : _source(obj), _talker(speaker), _gender(gender), _msgType(msgtype), _textGroup(textGroup), _textId(id), _language(language), _target(target) { }

    std::size_t operator()(WorldPacket* data, LocaleConstant locale) const
    {
        std::string const& text = sCreatureTextMgr->GetLocalizedChatString(_source->GetEntry(), _gender, _textGroup, _textId, locale);

        return ChatHandler::BuildChatPacket(*data, _msgType, Language(_language), _talker, _target, text);
    }

private:
    WorldObject* _source;
    WorldObject* _talker;
    uint8 _gender;
    ChatMsg _msgType;
    uint8 _textGroup;
    uint32 _textId;
    uint32 _language;
    WorldObject const* _target;
};

CreatureTextMgr* CreatureTextMgr::instance()
{
    static CreatureTextMgr instance;
    return &instance;
}

void CreatureTextMgr::LoadCreatureTexts()
{
    uint32 oldMSTime = getMSTime();

    mTextMap.clear(); // for reload case
    //all currently used temp texts are NOT reset

    const auto table = "world_creature_text";
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_CREATURE_TEXT);
    QueryResult result = WorldDatabase.Query(stmt);

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature texts. DB table `{}` is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 textCount = 0;

    do
    {
        const Field* fields = result->Fetch();
        CreatureTextEntry temp;

        temp.Entry           = fields[0].Get<uint32>();
        temp.Group           = fields[1].Get<uint8>();
        temp.ID              = fields[2].Get<uint8>();
        temp.Text            = fields[3].Get<std::string>();
        temp.Type            = static_cast<ChatMsg>(fields[4].Get<uint8>());
        temp.Lang            = static_cast<Language>(fields[5].Get<uint8>());
        temp.Probability     = fields[6].Get<float>();
        temp.Emote           = static_cast<Emote>(fields[7].Get<uint32>());
        temp.Duration        = fields[8].Get<uint32>();
        temp.Sound           = fields[9].Get<uint32>();
        temp.BroadcastTextID = fields[10].Get<uint32>();
        temp.TextRange       = static_cast<CreatureTextRange>(fields[11].Get<uint8>());

        if (temp.Sound && !sSoundEntriesStore.LookupEntry(temp.Sound))
        {
            LOG_ERROR("sql.sql", "CreatureTextMgr: Entry {}, Group {} in table `{}` has Sound {} but sound does not exist.", temp.Entry, temp.Group, table, temp.Sound);
            temp.Sound = 0;
        }
        if (!GetLanguageDescByID(temp.Lang))
        {
            LOG_ERROR("sql.sql", "CreatureTextMgr: Entry {}, Group {} in table `{}` using Language {} but Language does not exist.", temp.Entry, temp.Group, table, static_cast<uint32>(temp.Lang));
            temp.Lang = LANG_UNIVERSAL;
        }
        if (temp.Type >= MAX_CHAT_MSG_TYPE)
        {
            LOG_ERROR("sql.sql", "CreatureTextMgr: Entry {}, Group {} in table `{}` has Type {} but this Chat Type does not exist.", temp.Entry, temp.Group, table, static_cast<uint32>(temp.Type));
            temp.Type = CHAT_MSG_SAY;
        }
        if (temp.Emote)
        {
            if (!sEmotesStore.LookupEntry(temp.Emote))
            {
                LOG_ERROR("sql.sql", "CreatureTextMgr: Entry {}, Group {} in table `{}` has Emote {} but emote does not exist.", temp.Entry, temp.Group, table, static_cast<uint32>(temp.Emote));
                temp.Emote = EMOTE_ONESHOT_NONE;
            }
        }
        if (temp.BroadcastTextID)
        {
            if (!sObjectMgr->GetBroadcastText(temp.BroadcastTextID))
            {
                LOG_ERROR("sql.sql", "CreatureTextMgr: Entry {}, Group {}, ID {} in table `{}` has non-existing or incompatible BroadcastTextId {}.", temp.Entry, temp.Group, temp.ID, table, temp.BroadcastTextID);
                temp.BroadcastTextID = 0;
            }
        }
        if (temp.TextRange > TEXT_RANGE_WORLD)
        {
            LOG_ERROR("sql.sql", "CreatureTextMgr: Entry {}, Group {}, ID {} in table `{}` has incorrect TextRange {}.", temp.Entry, temp.Group, temp.ID, table, temp.TextRange);
            temp.TextRange = TEXT_RANGE_NORMAL;
        }

        // Add the text into our entry's group
        mTextMap[temp.Entry][temp.Group].push_back(temp);

        ++textCount;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Texts For {} Creatures in {} ms", textCount, mTextMap.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 CreatureTextMgr::SendChat(Creature* source, uint8 textGroup, WorldObject const* target /*= nullptr*/, ChatMsg msgType /*= CHAT_MSG_ADDON*/, Language language /*= LANG_ADDON*/, CreatureTextRange range /*= TEXT_RANGE_NORMAL*/, uint32 sound /*= 0*/, TeamID teamId /*= TEAM_NEUTRAL*/, bool gmOnly /*= false*/, Player* srcPlr /*= nullptr*/)
{
    if (!source)
        return 0;

    CreatureTextMap::const_iterator sList = mTextMap.find(source->GetEntry());
    if (sList == mTextMap.end())
    {
        LOG_ERROR("sql.sql", "CreatureTextMgr: Could not find Text for Creature({}) Entry {} in 'creature_text' table. Ignoring.", source->GetName(), source->GetEntry());
        return 0;
    }

    CreatureTextHolder const& textHolder = sList->second;
    CreatureTextHolder::const_iterator itr = textHolder.find(textGroup);
    if (itr == textHolder.end())
    {
        LOG_ERROR("sql.sql", "CreatureTextMgr: Could not find TextGroup {} for Creature {} ({}). Ignoring.",
            uint32(textGroup), source->GetName(), source->GetGUID().ToString());
        return 0;
    }

    CreatureTextGroup const& textGroupContainer = itr->second;  //has all texts in the group
    CreatureTextRepeatIds repeatGroup = source->GetTextRepeatGroup(textGroup);//has all textIDs from the group that were already said
    CreatureTextGroup tempGroup;//will use this to talk after sorting repeatGroup

    for (CreatureTextGroup::const_iterator giter = textGroupContainer.begin(); giter != textGroupContainer.end(); ++giter)
        if (std::find(repeatGroup.begin(), repeatGroup.end(), giter->ID) == repeatGroup.end())
            tempGroup.push_back(*giter);

    if (tempGroup.empty())
    {
        source->ClearTextRepeatGroup(textGroup);
        tempGroup = textGroupContainer;
    }

    auto iter = Acore::Containers::SelectRandomWeightedContainerElement(tempGroup, [](CreatureTextEntry const& t) -> double
    {
        return t.Probability;
    });

    ChatMsg finalType = (msgType == CHAT_MSG_ADDON) ? iter->Type : msgType;
    Language finalLang = (language == LANG_ADDON) ? iter->Lang : language;
    uint32 finalSound = sound ? sound : iter->Sound;

    if (range == TEXT_RANGE_NORMAL)
        range = iter->TextRange;

    if (finalSound)
        SendSound(source, finalSound, finalType, target, range, teamId, gmOnly);

    Unit* finalSource = source;
    if (srcPlr)
        finalSource = srcPlr;

    if (iter->Emote)
        SendEmote(finalSource, iter->Emote);

    if (srcPlr)
    {
        PlayerTextBuilder builder(source, finalSource, finalSource->getGender(), finalType, iter->Group, iter->ID, finalLang, target);
        SendChatPacket(finalSource, builder, finalType, target, range, teamId, gmOnly);
    }
    else
    {
        CreatureTextBuilder builder(finalSource, finalSource->getGender(), finalType, iter->Group, iter->ID, finalLang, target);
        SendChatPacket(finalSource, builder, finalType, target, range, teamId, gmOnly);
    }

    source->SetTextRepeatId(textGroup, iter->ID);
    return iter->Duration;
}

float CreatureTextMgr::GetRangeForChatType(ChatMsg msgType) const
{
    float dist = sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY);
    switch (msgType)
    {
        case CHAT_MSG_MONSTER_YELL:
            dist = sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL);
            break;
        case CHAT_MSG_MONSTER_EMOTE:
        case CHAT_MSG_RAID_BOSS_EMOTE:
            dist = sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE);
            break;
        default:
            break;
    }

    return dist;
}

void CreatureTextMgr::SendSound(Creature* source, uint32 sound, ChatMsg msgType, WorldObject const* target, CreatureTextRange range, TeamID teamId, bool gmOnly)
{
    if (!sound || !source)
        return;

    SendNonChatPacket(source, WorldPackets::Misc::Playsound(sound).Write(), msgType, target, range, teamId, gmOnly);
}

void CreatureTextMgr::SendNonChatPacket(WorldObject* source, WorldPacket const* data, ChatMsg msgType, WorldObject const* target, CreatureTextRange range, TeamID teamId, bool gmOnly) const
{
    float dist = GetRangeForChatType(msgType);

    switch (msgType)
    {
        case CHAT_MSG_MONSTER_WHISPER:
        case CHAT_MSG_RAID_BOSS_WHISPER:
            {
                if (range == TEXT_RANGE_NORMAL) // ignores team and GM only
                {
                    if (!target || !target->IsPlayer())
                        return;

                    target->ToPlayer()->SendDirectMessage(data);
                    return;
                }
                break;
            }
        default:
            break;
    }

    switch (range)
    {
        case TEXT_RANGE_AREA:
            {
                uint32 areaId = source->GetAreaId();
                Map::PlayerList const& players = source->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    if (itr->GetSource()->GetAreaId() == areaId && (teamId == TEAM_NEUTRAL || itr->GetSource()->GetTeamId() == teamId) && (!gmOnly || itr->GetSource()->IsGameMaster()))
                        itr->GetSource()->SendDirectMessage(data);
                return;
            }
        case TEXT_RANGE_ZONE:
            {
                uint32 zoneId = source->GetZoneId();
                Map::PlayerList const& players = source->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    if (itr->GetSource()->GetZoneId() == zoneId && (teamId == TEAM_NEUTRAL || itr->GetSource()->GetTeamId() == teamId) && (!gmOnly || itr->GetSource()->IsGameMaster()))
                        itr->GetSource()->SendDirectMessage(data);
                return;
            }
        case TEXT_RANGE_MAP:
            {
                Map::PlayerList const& players = source->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    if ((teamId == TEAM_NEUTRAL || itr->GetSource()->GetTeamId() == teamId) && (!gmOnly || itr->GetSource()->IsGameMaster()))
                        itr->GetSource()->SendDirectMessage(data);
                return;
            }
        case TEXT_RANGE_WORLD:
            {
                WorldSessionMgr::SessionMap const& sessionMap = sWorldSessionMgr->GetAllSessions();
                for (WorldSessionMgr::SessionMap::const_iterator itr = sessionMap.begin(); itr != sessionMap.end(); ++itr)
                    if (Player* player = itr->second->GetPlayer())
                        if ((teamId == TEAM_NEUTRAL || player->GetTeamId() == teamId) && (!gmOnly || player->IsGameMaster()))
                            player->SendDirectMessage(data);
                return;
            }
        case TEXT_RANGE_NORMAL:
        default:
            break;
    }

    source->SendMessageToSetInRange(data, dist, true);
}

void CreatureTextMgr::SendEmote(Unit* source, uint32 emote)
{
    if (!source)
        return;

    source->HandleEmoteCommand(emote);
}

bool CreatureTextMgr::TextExist(uint32 sourceEntry, uint8 textGroup)
{
    if (!sourceEntry)
        return false;

    CreatureTextMap::const_iterator sList = mTextMap.find(sourceEntry);
    if (sList == mTextMap.end())
    {
        LOG_DEBUG("entities.unit", "CreatureTextMgr::TextExist: Could not find Text for Creature (entry {}) in 'creature_text' table.", sourceEntry);
        return false;
    }

    CreatureTextHolder const& textHolder = sList->second;
    CreatureTextHolder::const_iterator itr = textHolder.find(textGroup);
    if (itr == textHolder.end())
    {
        LOG_DEBUG("entities.unit", "CreatureTextMgr::TextExist: Could not find TextGroup {} for Creature (entry {}).", uint32(textGroup), sourceEntry);
        return false;
    }

    return true;
}

std::string CreatureTextMgr::GetLocalizedChatString(uint32 entry, uint8 gender, uint8 textGroup, uint32 id, LocaleConstant locale) const
{
    CreatureTextMap::const_iterator mapitr = mTextMap.find(entry);
    if (mapitr == mTextMap.end())
        return "";

    CreatureTextHolder::const_iterator holderItr = mapitr->second.find(textGroup);
    if (holderItr == mapitr->second.end())
        return "";

    CreatureTextGroup::const_iterator groupItr = holderItr->second.begin();
    for (; groupItr != holderItr->second.end(); ++groupItr)
        if (groupItr->ID == id)
            break;

    if (groupItr == holderItr->second.end())
        return "";

    if (locale > MAX_LOCALES)
        locale = DEFAULT_LOCALE;

    std::string baseText = "";

    if (const BroadcastText* bct = sObjectMgr->GetBroadcastText(groupItr->BroadcastTextID))
        baseText = bct->GetText(gender);
    else
        baseText = groupItr->Text;

    return baseText;
}
