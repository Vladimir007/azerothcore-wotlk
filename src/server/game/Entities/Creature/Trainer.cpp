#include "Trainer.h"
#include "Creature.h"
#include "NPCPackets.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

namespace Trainer
{
    bool Spell::IsCastable() const
    {
        return sSpellMgr->AssertSpellInfo(SpellID)->HasEffect(SPELL_EFFECT_LEARN_SPELL);
    }

    Trainer::Trainer(const uint32 trainerID, const Type type, const uint32 requirement, std::string greeting, std::vector<Spell> spells):
        _trainerID(trainerID), _type(type), _requirement(requirement), _spells(std::move(spells))
    {
        _greeting = std::move(greeting);
    }

    void Trainer::SendSpells(const Creature* npc, const Player* player) const
    {
        const float reputationDiscount = player->GetReputationPriceDiscount(npc);

        WorldPackets::NPC::TrainerList trainerList;
        trainerList.TrainerGUID = npc->GetGUID();
        trainerList.TrainerType = AsUnderlyingType(_type);
        trainerList.Greeting = _greeting;
        trainerList.Spells.reserve(_spells.size());
        for (const Spell& trainerSpell : _spells)
        {
            if (!player->IsSpellFitByClassAndRace(trainerSpell.SpellID))
                continue;

            const SpellInfo* trainerSpellInfo = sSpellMgr->AssertSpellInfo(trainerSpell.SpellID);

            bool primaryProfessionFirstRank = false;
            for (const SpellEffectInfo& spellEffectInfo : trainerSpellInfo->GetEffects())
            {
                if (!spellEffectInfo.IsEffect(SPELL_EFFECT_LEARN_SPELL))
                    continue;

                const SpellInfo* learnedSpellInfo = sSpellMgr->GetSpellInfo(spellEffectInfo.TriggerSpell);
                if (learnedSpellInfo && learnedSpellInfo->IsPrimaryProfessionFirstRank())
                    primaryProfessionFirstRank = true;
            }

            trainerList.Spells.emplace_back();
            WorldPackets::NPC::TrainerListSpell& trainerListSpell = trainerList.Spells.back();
            trainerListSpell.SpellID = trainerSpell.SpellID;
            trainerListSpell.Usable = AsUnderlyingType(GetSpellState(player, &trainerSpell));
            trainerListSpell.MoneyCost = static_cast<int32>(trainerSpell.MoneyCost * reputationDiscount);
            trainerListSpell.PointCost[0] = 0; // spells don't cost talent points
            trainerListSpell.PointCost[1] = (primaryProfessionFirstRank ? 1 : 0);
            trainerListSpell.ReqLevel = trainerSpell.ReqLevel;
            trainerListSpell.ReqSkillLine = trainerSpell.ReqSkillLine;
            trainerListSpell.ReqSkillRank = trainerSpell.ReqSkillRank;
            std::ranges::copy(trainerSpell.ReqAbility, trainerListSpell.ReqAbility.begin());
        }

        player->SendDirectMessage(trainerList.Write());
    }

    void Trainer::TeachSpell(Creature* npc, Player* player, const uint32 spellID)
    {
        if (!IsTrainerValidForPlayer(player))
            return;

        const Spell* trainerSpell = GetSpell(spellID);
        if (!trainerSpell)
        {
            SendTeachFailure(npc, player, spellID, FailReason::Unavailable);
            return;
        }

        if (!CanTeachSpell(player, trainerSpell))
        {
            SendTeachFailure(npc, player, spellID, FailReason::NotEnoughSkill);
            return;
        }

        const float reputationDiscount = player->GetReputationPriceDiscount(npc);
        const int32 moneyCost = static_cast<int32>(trainerSpell->MoneyCost * reputationDiscount);
        if (!player->HasEnoughMoney(moneyCost))
        {
            SendTeachFailure(npc, player, spellID, FailReason::NotEnoughMoney);
            return;
        }

        player->ModifyMoney(-moneyCost);

        npc->SendPlaySpellVisual(179); // 53 SpellCastDirected
        npc->SendPlaySpellImpact(player->GetGUID(), 362); // 113 EmoteSalute

        // Learn explicitly or cast explicitly
        if (trainerSpell->IsCastable())
            player->CastSpell(player, trainerSpell->SpellID, true);
        else
            player->learnSpell(trainerSpell->SpellID, false);

        SendTeachSucceeded(npc, player, spellID);
    }

    const Spell* Trainer::GetSpell(uint32 spellID) const
    {
        const auto itr = std::ranges::find_if(_spells, [spellID](const Spell& trainerSpell) { return trainerSpell.SpellID == spellID; });
        if (itr == _spells.end())
            return nullptr;
        return &*itr;
    }

    bool Trainer::CanTeachSpell(const Player* player, const Spell* trainerSpell)
    {
        if (const SpellState state = GetSpellState(player, trainerSpell); state != SpellState::Available)
            return false;

        const SpellInfo* trainerSpellInfo = sSpellMgr->AssertSpellInfo(trainerSpell->SpellID);
        for (const SpellEffectInfo& spellEffectInfo : trainerSpellInfo->GetEffects())
        {
            if (!spellEffectInfo.IsEffect(SPELL_EFFECT_LEARN_SPELL))
                continue;

            const SpellInfo* learnedSpellInfo = sSpellMgr->GetSpellInfo(spellEffectInfo.TriggerSpell);
            if (learnedSpellInfo && learnedSpellInfo->IsPrimaryProfessionFirstRank() && !player->GetFreePrimaryProfessionPoints())
                return false;
        }

        return true;
    }

    SpellState Trainer::GetSpellState(const Player* player, const Spell* trainerSpell)
    {
        if (player->HasSpell(trainerSpell->SpellID))
            return SpellState::Known;

        // Check race/class requirement
        if (!player->IsSpellFitByClassAndRace(trainerSpell->SpellID))
            return SpellState::Unavailable;

        // Check skill requirement
        if (trainerSpell->ReqSkillLine && player->GetBaseSkillValue(trainerSpell->ReqSkillLine) < trainerSpell->ReqSkillRank)
            return SpellState::Unavailable;

        for (const int32 reqAbility : trainerSpell->ReqAbility)
            if (reqAbility && !player->HasSpell(reqAbility))
                return SpellState::Unavailable;

        // Check level requirement
        if (player->GetLevel() < trainerSpell->ReqLevel)
            return SpellState::Unavailable;

        // Check ranks
        bool hasLearnSpellEffect = false;
        bool knowsAllLearnedSpells = true;
        for (const SpellEffectInfo& spellEffectInfo : sSpellMgr->AssertSpellInfo(trainerSpell->SpellID)->GetEffects())
        {
            if (!spellEffectInfo.IsEffect(SPELL_EFFECT_LEARN_SPELL))
                continue;

            hasLearnSpellEffect = true;
            if (!player->HasSpell(spellEffectInfo.TriggerSpell))
                knowsAllLearnedSpells = false;

            if (const uint32 previousRankSpellId = sSpellMgr->GetPrevSpellInChain(spellEffectInfo.TriggerSpell))
                if (!player->HasSpell(previousRankSpellId))
                    return SpellState::Unavailable;
        }

        if (!hasLearnSpellEffect)
        {
            if (const uint32 previousRankSpellId = sSpellMgr->GetPrevSpellInChain(trainerSpell->SpellID))
                if (!player->HasSpell(previousRankSpellId))
                    return SpellState::Unavailable;
        }
        else if (knowsAllLearnedSpells)
            return SpellState::Known;

        // Check additional spell requirement
        for (const auto& requirePair : sSpellMgr->GetSpellsRequiredForSpellBounds(trainerSpell->SpellID))
            if (!player->HasSpell(requirePair.second))
                return SpellState::Unavailable;

        return SpellState::Available;
    }

    bool Trainer::IsTrainerValidForPlayer(const Player* player) const
    {
        if (!GetTrainerRequirement())
            return true;

        switch (GetTrainerType())
        {
            case Type::Class:
            case Type::Pet:
                // Check class for class trainers
                return player->getClass() == GetTrainerRequirement();
            case Type::Mount:
                // Check race for mount trainers
                return player->getRace() == GetTrainerRequirement();
            case Type::TradeSkill:
                // Check spell for profession trainers
                return player->HasSpell(GetTrainerRequirement());
            default:
                break;
        }

        return true;
    }

    void Trainer::SendTeachFailure(const Creature* npc, const Player* player, const uint32 spellID, const FailReason reason)
    {
        WorldPackets::NPC::TrainerBuyFailed trainerBuyFailed;
        trainerBuyFailed.TrainerGUID = npc->GetGUID();
        trainerBuyFailed.SpellID = spellID;
        trainerBuyFailed.TrainerFailedReason = AsUnderlyingType(reason);
        player->SendDirectMessage(trainerBuyFailed.Write());
    }

    void Trainer::SendTeachSucceeded(Creature const* npc, Player const* player, const uint32 spellID)
    {
        WorldPackets::NPC::TrainerBuySucceeded trainerBuySucceeded;
        trainerBuySucceeded.TrainerGUID = npc->GetGUID();
        trainerBuySucceeded.SpellID = spellID;
        player->SendDirectMessage(trainerBuySucceeded.Write());
    }
}
