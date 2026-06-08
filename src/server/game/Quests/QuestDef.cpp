#include "QuestDef.h"
#include "Formulas.h"
#include "Opcodes.h"
#include "Player.h"
#include "World.h"

Quest::Quest(const Field* questRecord)
{
    EmoteOnIncomplete = 0;
    EmoteOnComplete = 0;
    _reqItemsCount = 0;
    _reqCreatureOrGOcount = 0;
    _rewItemsCount = 0;
    _rewChoiceItemsCount = 0;

    Id = questRecord[0].Get<uint32>();
    Method = questRecord[1].Get<uint8>();
    Level = questRecord[2].Get<int16>();
    MinLevel = questRecord[3].Get<uint8>();
    ZoneOrSort = questRecord[4].Get<int16>();
    Type = questRecord[5].Get<uint16>();
    SuggestedPlayers = questRecord[6].Get<uint8>();
    TimeAllowed = questRecord[7].Get<uint32>();
    AllowableRaces = questRecord[8].Get<uint32>();
    Title = questRecord[9].Get<std::string>();
    Objectives = questRecord[10].Get<std::string>();
    Details = questRecord[11].Get<std::string>();
    AreaDescription = questRecord[12].Get<std::string>();
    CompletedText = questRecord[13].Get<std::string>();
    StartItem = questRecord[14].Get<uint32>();
    Flags = questRecord[15].Get<uint32>();
    POIContinent = questRecord[16].Get<uint16>();
    POIx = questRecord[17].Get<float>();
    POIy = questRecord[18].Get<float>();
    POIPriority = questRecord[19].Get<uint32>();

    const auto itemDrop = questRecord[20].GetArray<uint32, QUEST_SOURCE_ITEM_IDS_COUNT>();
    const auto itemDropQuantity = questRecord[21].GetArray<uint16, QUEST_SOURCE_ITEM_IDS_COUNT>();
    for (int i = 0; i < QUEST_SOURCE_ITEM_IDS_COUNT; ++i)
    {
        ItemDrop[i] = itemDrop[i];
        ItemDropQuantity[i] = itemDropQuantity[i];
    }

    RequiredFactionId1 = questRecord[22].Get<uint16>();
    RequiredFactionId2 = questRecord[23].Get<uint16>();
    RequiredFactionValue1 = questRecord[24].Get<int32>();
    RequiredFactionValue2 = questRecord[25].Get<int32>();

    const auto requiredNpcOrGo = questRecord[26].GetArray<int32, QUEST_OBJECTIVES_COUNT>();
    const auto requiredNpcOrGoCount = questRecord[27].GetArray<uint16, QUEST_OBJECTIVES_COUNT>();
    const auto objectiveText = questRecord[28].GetArray<std::string, QUEST_OBJECTIVES_COUNT>();
    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
    {
        RequiredNpcOrGo[i] = requiredNpcOrGo[i];
        RequiredNpcOrGoCount[i] = requiredNpcOrGoCount[i];
        ObjectiveText[i] = objectiveText[i];

        if (RequiredNpcOrGo[i])
            ++_reqCreatureOrGOcount;
    }

    const auto requiredItems = questRecord[29].GetArray<uint32, QUEST_ITEM_OBJECTIVES_COUNT>();
    const auto requiredItemCounts = questRecord[30].GetArray<uint16, QUEST_ITEM_OBJECTIVES_COUNT>();
    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
    {
        RequiredItemId[i] = requiredItems[i];
        RequiredItemCount[i] = requiredItemCounts[i];

        if (RequiredItemId[i])
            ++_reqItemsCount;
    }

    RequiredPlayerKills = questRecord[31].Get<uint8>();

    const auto rewardItems = questRecord[32].GetArray<uint32, QUEST_REWARDS_COUNT>();
    const auto rewardAmounts = questRecord[33].GetArray<uint16, QUEST_REWARDS_COUNT>();
    for (int i = 0; i < QUEST_REWARDS_COUNT; ++i)
    {
        RewardItemId[i] = rewardItems[i];
        RewardItemIdCount[i] = rewardAmounts[i];

        if (RewardItemId[i])
            ++_rewItemsCount;
    }

    const auto rewardChoiceItems = questRecord[34].GetArray<uint32, QUEST_REWARD_CHOICES_COUNT>();
    const auto rewardChoiceAmounts = questRecord[35].GetArray<uint16, QUEST_REWARD_CHOICES_COUNT>();
    for (int i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
    {
        RewardChoiceItemId[i] = rewardChoiceItems[i];
        RewardChoiceItemCount[i] = rewardChoiceAmounts[i];

        if (RewardChoiceItemId[i])
            ++_rewChoiceItemsCount;
    }

    const auto rewardFactions = questRecord[36].GetArray<uint32, QUEST_REPUTATIONS_COUNT>();
    const auto rewardFactionValues = questRecord[37].GetArray<int32, QUEST_REPUTATIONS_COUNT>();
    const auto rewardFactionValueOverrides = questRecord[38].GetArray<int32, QUEST_REPUTATIONS_COUNT>();
    for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        RewardFactionId[i] = rewardFactions[i];
        RewardFactionValueId[i] = rewardFactionValues[i];
        RewardFactionValueIdOverride[i] = rewardFactionValueOverrides[i];
    }

    RewardMoney = questRecord[39].Get<int32>();
    RewardMoneyDifficulty = questRecord[40].Get<uint32>();
    RewardArenaPoints = questRecord[41].Get<uint16>();
    RewardHonor = questRecord[42].Get<uint32>();
    RewardKillHonor = questRecord[43].Get<float>();
    RewardNextQuest = questRecord[44].Get<uint32>();
    RewardSpell = questRecord[45].Get<int32>();
    RewardDisplaySpell = questRecord[46].Get<uint32>();
    RewardTalents = questRecord[47].Get<uint8>();
    RewardTitleId = questRecord[48].Get<uint8>();
    RewardXPDifficulty = questRecord[49].Get<uint8>();

    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
    {
        DetailsEmote[i] = 0;
        DetailsEmoteDelay[i] = 0;
        OfferRewardEmote[i] = 0;
        OfferRewardEmoteDelay[i] = 0;
    }

    _eventIdForQuest = 0;

    if (sWorld->getBoolConfig(CONFIG_QUEST_IGNORE_AUTO_ACCEPT))
        Flags &= ~QUEST_FLAGS_AUTO_ACCEPT;

    if (sWorld->getBoolConfig(CONFIG_QUEST_IGNORE_AUTO_COMPLETE))
        Flags &= ~QUEST_FLAGS_AUTOCOMPLETE;
}

void Quest::LoadQuestTemplateAddon(const Field* fields)
{
    MaxLevel = fields[1].Get<uint8>();
    RequiredClasses = fields[2].Get<uint32>();
    SourceSpellID = fields[3].Get<uint32>();
    PrevQuestId = fields[4].Get<int32>();
    NextQuestId = fields[5].Get<uint32>();
    ExclusiveGroup = fields[6].Get<int32>();
    BreadcrumbForQuestId = fields[7].Get<uint32>();
    RewardMailTemplateId = fields[8].Get<uint32>();
    RewardMailDelay = fields[9].Get<uint32>();
    RequiredSkillId = fields[10].Get<uint16>();
    RequiredSkillPoints = fields[11].Get<uint16>();
    RequiredMinRepFaction = fields[12].Get<uint16>();
    RequiredMaxRepFaction = fields[13].Get<uint16>();
    RequiredMinRepValue = fields[14].Get<int32>();
    RequiredMaxRepValue = fields[15].Get<int32>();
    StartItemCount = fields[16].Get<uint8>();
    RewardMailSenderEntry = fields[17].Get<uint32>();
    SpecialFlags = fields[18].Get<uint32>();

    if ((SpecialFlags & QUEST_SPECIAL_FLAGS_AUTO_ACCEPT) && !sWorld->getBoolConfig(CONFIG_QUEST_IGNORE_AUTO_ACCEPT))
        Flags |= QUEST_FLAGS_AUTO_ACCEPT;
}

uint32 Quest::XPValue(const uint8 playerLevel) const
{
    const int32 quest_level = Level == -1 ? playerLevel : Level;
    const QuestXPEntry* xpEntry = sQuestXPStore.LookupEntry(quest_level);
    if (!xpEntry)
    {
        return 0;
    }

    int32 diffFactor = 2 * (quest_level - playerLevel) + 20;
    if (diffFactor < 1)
    {
        diffFactor = 1;
    }
    else if (diffFactor > 10)
    {
        diffFactor = 10;
    }

    uint32 xp = diffFactor * xpEntry->Exp[RewardXPDifficulty] / 10;
    if (xp <= 100)
    {
        xp = 5 * ((xp + 2) / 5);
    }
    else if (xp <= 500)
    {
        xp = 10 * ((xp + 5) / 10);
    }
    else if (xp <= 1000)
    {
        xp = 25 * ((xp + 12) / 25);
    }
    else
    {
        xp = 50 * ((xp + 25) / 50);
    }

    return xp;
}

int32 Quest::GetRewOrReqMoney(uint8 playerLevel) const
{
    int32 rewardedMoney = RewardMoney;
    if (rewardedMoney < 0)
    {
        return rewardedMoney;
    }

    if (playerLevel && RewardMoneyDifficulty)
    {
        if (uint32 questRewardedMoney = sObjectMgr->GetQuestMoneyReward(playerLevel, RewardMoneyDifficulty))
        {
            rewardedMoney = questRewardedMoney;
        }
    }

    return static_cast<int32>(rewardedMoney * sWorld->getRate(RATE_REWARD_QUEST_MONEY));
}

uint32 Quest::GetRewMoneyMaxLevel() const
{
    uint32 rewMoney = 0;

    if (HasFlag(QUEST_FLAGS_NO_MONEY_FROM_XP))
        return rewMoney;

    rewMoney = (XPValue(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)) * (6 * COPPER));
    // https://wowpedia.fandom.com/wiki/Quest?oldid=1035002 Formula is XP gained * 6c
    return static_cast<int32>(rewMoney * sWorld->getRate(RATE_REWARD_BONUS_MONEY));
}

bool Quest::IsAutoAccept() const
{
    return HasFlag(QUEST_FLAGS_AUTO_ACCEPT);
}

bool Quest::IsAutoComplete() const
{
    return HasFlag(QUEST_FLAGS_AUTOCOMPLETE);
}

bool Quest::IsRaidQuest(Difficulty difficulty) const
{
    switch (Type)
    {
        case QUEST_TYPE_RAID:
            return true;
        case QUEST_TYPE_RAID_10:
            return !(difficulty & RAID_DIFFICULTY_MASK_25MAN);
        case QUEST_TYPE_RAID_25:
            return difficulty & RAID_DIFFICULTY_MASK_25MAN;
        default:
            break;
    }

    return false;
}

bool Quest::IsAllowedInRaid(Difficulty difficulty) const
{
    if (IsRaidQuest(difficulty))
        return true;

    return sWorld->getBoolConfig(CONFIG_QUEST_IGNORE_RAID);
}

uint32 Quest::CalculateHonorGain(uint8 level) const
{
    if (level > GT_MAX_LEVEL)
        level = GT_MAX_LEVEL;

    uint32 honor = 0;

    if (GetRewHonorAddition() > 0 || GetRewHonorMultiplier() > 0.0f)
    {
        TeamContributionPointsEntry const* tc = sTeamContributionPointsStore.LookupEntry(level);
        if (!tc)
            return 0;
        honor = static_cast<uint32>(tc->Value * GetRewHonorMultiplier() * 0.1000000014901161);
        honor += GetRewHonorAddition();
    }

    return honor;
}

void Quest::InitializeQueryData()
{
    queryData.Initialize(SMSG_QUEST_QUERY_RESPONSE, 1);

    queryData << uint32(GetQuestId());                    // quest id
    queryData << uint32(GetQuestMethod());                // Accepted values: 0, 1 or 2. 0 == IsAutoComplete() (skip objectives/details)
    queryData << uint32(GetQuestLevel());                 // may be -1, static data, in other cases must be used dynamic level: Player::GetQuestLevel (0 is not known, but assuming this is no longer valid for quest intended for client)
    queryData << uint32(GetMinLevel());                   // min level
    queryData << uint32(GetZoneOrSort());                 // zone or sort to display in quest log

    queryData << uint32(GetType());                       // quest type
    queryData << uint32(GetSuggestedPlayers());           // suggested players count

    queryData << uint32(GetRepObjectiveFaction());        // shown in quest log as part of quest objective
    queryData << uint32(GetRepObjectiveValue());          // shown in quest log as part of quest objective

    queryData << uint32(GetRepObjectiveFaction2());       // shown in quest log as part of quest objective OPPOSITE faction
    queryData << uint32(GetRepObjectiveValue2());         // shown in quest log as part of quest objective OPPOSITE faction

    queryData << uint32(GetNextQuestInChain());           // client will request this quest from NPC, if not 0
    queryData << uint32(GetXPId());                       // used for calculating rewarded experience

    if (HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
        queryData << uint32(0);                           // Hide money rewarded
    else
        queryData << int32(GetRewOrReqMoney());           // reward money (below max lvl)

    queryData << uint32(GetRewMoneyMaxLevel());           // used in XP calculation at client
    queryData << uint32(GetRewSpell());                   // reward spell, this spell will display (icon) (casted if RewSpellCast == 0)
    queryData << int32(GetRewSpellCast());                // casted spell

    // rewarded honor points
    queryData << uint32(GetRewHonorAddition());
    queryData << float(GetRewHonorMultiplier());
    queryData << uint32(GetSrcItemId());                  // source item id
    queryData << uint32(GetFlags() & 0xFFFF);             // quest flags
    queryData << uint32(GetCharTitleId());                // CharTitleId, new 2.4.0, player gets this title (id from CharTitles)
    queryData << uint32(GetPlayersSlain());               // players slain
    queryData << uint32(GetBonusTalents());               // bonus talents
    queryData << uint32(GetRewArenaPoints());             // bonus arena points
    queryData << uint32(0);                                      // review rep show mask

    if (HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
    {
        for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            queryData << uint32(0) << uint32(0);
        for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
            queryData << uint32(0) << uint32(0);
    }
    else
    {
        for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
        {
            queryData << uint32(RewardItemId[i]);
            queryData << uint32(RewardItemIdCount[i]);
        }
        for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            queryData << uint32(RewardChoiceItemId[i]);
            queryData << uint32(RewardChoiceItemCount[i]);
        }
    }

    for (uint32 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)        // reward factions ids
        queryData << uint32(RewardFactionId[i]);

    for (uint32 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)        // columnid+1 QuestFactionReward.dbc?
        queryData << int32(RewardFactionValueId[i]);

    for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)           // unk (0)
        queryData << int32(RewardFactionValueIdOverride[i]);

    queryData << GetPOIContinent();
    queryData << GetPOIx();
    queryData << GetPOIy();
    queryData << GetPointOpt();

    queryData << GetTitle();
    queryData << GetObjectives();
    queryData << GetDetails();
    queryData << GetAreaDescription();
    queryData << GetCompletedText();                                  // display in quest objectives window once all objectives are completed

    for (uint32 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
    {
        if (RequiredNpcOrGo[i] < 0)
            queryData << uint32((RequiredNpcOrGo[i] * (-1)) | 0x80000000);    // client expects gameobject template id in form (id|0x80000000)
        else
            queryData << uint32(RequiredNpcOrGo[i]);

        queryData << uint32(RequiredNpcOrGoCount[i]);
        queryData << uint32(ItemDrop[i]);
        queryData << uint32(0);                                  // req source count?
    }

    for (uint32 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
    {
        queryData << uint32(RequiredItemId[i]);
        queryData << uint32(RequiredItemCount[i]);
    }

    for (uint32 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        queryData << ObjectiveText[i];
}
