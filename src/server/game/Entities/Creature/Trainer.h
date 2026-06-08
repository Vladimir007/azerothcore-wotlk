#ifndef TRAINER_H
#define TRAINER_H

#include <array>
#include <vector>
#include "Common.h"

class Creature;
class ObjectMgr;
class Player;

namespace Trainer
{
    enum class Type : uint32
    {
        Class = 0,
        Mount = 1,
        TradeSkill = 2,
        Pet = 3
    };

    enum class SpellState : uint8
    {
        Available = 0,
        Unavailable = 1,
        Known = 2
    };

    enum class FailReason : uint32
    {
        Unavailable = 0,
        NotEnoughMoney = 1,
        NotEnoughSkill = 2
    };

    struct Spell
    {
        uint32 SpellID = 0;
        uint32 MoneyCost = 0;
        uint32 ReqSkillLine = 0;
        uint32 ReqSkillRank = 0;
        std::array<uint32, 3> ReqAbility = {};
        uint8 ReqLevel = 0;

        [[nodiscard]] bool IsCastable() const;
    };

    class Trainer
    {
        friend ObjectMgr;

        uint32 _trainerID;
        Type _type;
        uint32 _requirement;
        std::vector<Spell> _spells;
        std::string _greeting;

    public:
        Trainer(uint32 trainerID, Type type, uint32 requirement, std::string greeting, std::vector<Spell> spells);

        [[nodiscard]] const Spell* GetSpell(uint32 spellID) const;
        [[nodiscard]] const std::vector<Spell>& GetSpells() const { return _spells; }
        void SendSpells(const Creature* npc, const Player* player) const;
        static bool CanTeachSpell(const Player* player, Spell const* trainerSpell);
        void TeachSpell(Creature* npc, Player* player, uint32 spellID);

        [[nodiscard]] Type GetTrainerType() const { return _type; }
        [[nodiscard]] uint32 GetTrainerRequirement() const { return _requirement; }
        bool IsTrainerValidForPlayer(const Player* player) const;

    private:
        static SpellState GetSpellState(const Player* player, const Spell* trainerSpell);
        static void SendTeachFailure(const Creature* npc, const Player* player, uint32 spellID, FailReason reason);
        static void SendTeachSucceeded(const Creature* npc, const Player* player, uint32 spellID);
    };
}

#endif
