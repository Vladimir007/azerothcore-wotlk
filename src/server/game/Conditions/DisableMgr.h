#ifndef DISABLE_MGR_H
#define DISABLE_MGR_H

#include "Define.h"
#include "Map.h"

class Unit;

enum DisableType
{
    DISABLE_TYPE_SPELL                  = 0,
    DISABLE_TYPE_QUEST                  = 1,
    DISABLE_TYPE_MAP                    = 2,
    DISABLE_TYPE_BATTLEGROUND           = 3,
    DISABLE_TYPE_ACHIEVEMENT_CRITERIA   = 4,
    DISABLE_TYPE_OUTDOOR_PVP            = 5,
    DISABLE_TYPE_VMAP                   = 6,
    DISABLE_TYPE_GO_LOS                 = 7,
    DISABLE_TYPE_LFG_MAP                = 8,
    DISABLE_TYPE_GAME_EVENT             = 9,
    DISABLE_TYPE_LOOT                   = 10,
    MAX_DISABLE_TYPES
};

enum SpellDisableTypes
{
    SPELL_DISABLE_PLAYER            = 0x1,
    SPELL_DISABLE_CREATURE          = 0x2,
    SPELL_DISABLE_PET               = 0x4,
    SPELL_DISABLE_DEPRECATED_SPELL  = 0x8,
    SPELL_DISABLE_MAP               = 0x10,
    SPELL_DISABLE_AREA              = 0x20,
    SPELL_DISABLE_LOS               = 0x40,
    MAX_SPELL_DISABLE_TYPE          = (SPELL_DISABLE_PLAYER | SPELL_DISABLE_CREATURE | SPELL_DISABLE_PET |
                                       SPELL_DISABLE_DEPRECATED_SPELL | SPELL_DISABLE_MAP | SPELL_DISABLE_AREA |
                                       SPELL_DISABLE_LOS)
};

struct DisableData
{
    uint8 flags;
    std::set<uint32> params[2];  // params0, params1
};

class DisableMgr
{
    DisableMgr();
    ~DisableMgr();

public:
    static DisableMgr* instance();

    static void LoadDisables();
    static bool HandleDisableType(DisableType type, uint32 entry, uint8 flags, const std::vector<uint32>& params0, const std::vector<uint32>& params1, DisableData& data);
    static bool IsDisabledFor(DisableType type, uint32 entry, Unit const* unit, uint8 flags = 0);
    static void CheckQuestDisables();
    static bool IsVMAPDisabledFor(uint32 entry, uint8 flags);
    static bool IsPathfindingEnabled(Map const* map);

    // single disables here with optional data
    typedef std::unordered_map<uint32, DisableData> DisableTypeMap;
    // global disable map by source
    typedef std::array<DisableTypeMap, MAX_DISABLE_TYPES> DisableMap;

private:
    static DisableMap m_DisableMap;
};

#define sDisableMgr DisableMgr::instance()

#endif
