#ifndef WMO_H
#define WMO_H

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <G3D/AABox.h>
#include <G3D/Quat.h>
#include <G3D/Vector3.h>

#include "MPQ.h"

enum MopyFlags
{
    WHO_MATERIAL_UNK01            = 0x01,
    WMO_MATERIAL_NO_CAM_COLLIDE   = 0x02,
    WMO_MATERIAL_DETAIL           = 0x04,
    WMO_MATERIAL_COLLISION        = 0x08,
    WMO_MATERIAL_HINT             = 0x10,
    WMO_MATERIAL_RENDER           = 0x20,
    WMO_MATERIAL_WALL_SURFACE     = 0x40, // Guessed
    WMO_MATERIAL_COLLIDE_HIT      = 0x80,
};

class WMOInstance;
class WMOMgr;
class MPQFile;
namespace ADT { struct MODF; }

namespace WMO
{
    struct MODS
    {
        char Name[20];
        uint32 StartIndex;     // index of first doodad instance in this set
        uint32 Count;          // number of doodad instances in this set
        char _pad[4];
    };

    struct MODD
    {
        uint32 NameIndex : 24;
        G3D::Vector3 Position;
        G3D::Quat Rotation;
        float Scale;
        uint32 Color;
    };
}

static G3D::Vector3 fixCoords(const G3D::Vector3& v) { return G3D::Vector3(v.z, v.x, v.y); }

struct WMODoodadData
{
    std::vector<WMO::MODS> Sets;
    std::unique_ptr<char[]> Paths;
    std::vector<WMO::MODD> Spawns;
    std::unordered_set<uint16> References;
};

class WMORoot
{
    std::string filename;
public:
    explicit WMORoot(std::string const& filename);
    bool open();
    bool ConvertToVMAPRootWmo(FILE* pOutFile);

    unsigned int color;
    uint32 nTextures, nGroups, nPortals, nLights, nDoodadNames, nDoodadDefs, nDoodadSets, RootWMOid, flags;
    float bbCorn1[3];
    float bbCorn2[3];

    std::vector<char> GroupNames;
    WMODoodadData DoodadData;
    std::unordered_set<uint32> ValidDoodadNames;
};

#pragma pack(push, 1)

struct WMOLiquidHeader
{
    int xverts, yverts, xtiles, ytiles;
    float pos_x;
    float pos_y;
    float pos_z;
    short material;
};

struct WMOLiquidVert
{
    uint16 unk1;
    uint16 unk2;
    float height;
};

#pragma pack(pop)

class WMOGroup
{
    std::string filename;
public:
    explicit WMOGroup(std::string const& filename);
    ~WMOGroup();

    bool open(const WMORoot* rootWMO);
    int ConvertToVMAPGroupWmo(FILE* output);
    uint32 GetLiquidTypeId(uint32 liquidTypeId);
    bool ShouldSkip(WMORoot const* root) const;

    char* MOPY;
    uint16* MOVI;
    uint16* MoviEx;
    float* MOVT;
    uint16* MOBA;
    int* MobaEx;
    WMOLiquidHeader* hlq;
    WMOLiquidVert* LiquEx;
    char* LiquBytes;
    int groupName, descGroupName;
    int mogpFlags;
    float bbCorn1[3];
    float bbCorn2[3];
    uint16 moprIdx;
    uint16 moprNItems;
    uint16 nBatchA;
    uint16 nBatchB;
    uint32 nBatchC, fogIdx, groupLiquid, groupWMOid;

    int mopy_size, moba_size;
    int LiquEx_size;
    unsigned int nVertices; // Number when loaded
    int nTriangles; // Number when loaded
    uint32 liquidFlags;

    std::vector<uint16> DoodadReferences;
};

namespace MapObject
{
    void Extract(ADT::MODF const& mapObjDef, const std::string& WmoInstName, uint32 mapID, uint32 tileX, uint32 tileY, FILE* pDirFile);
}

#endif
