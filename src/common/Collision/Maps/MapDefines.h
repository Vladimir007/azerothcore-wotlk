#ifndef MAP_DEFINES_H
#define MAP_DEFINES_H

#include "Define.h"
#include "DetourNavMesh.h"

#define MAX_HEIGHT            100000.0f  // Can be used for finding ground height at surface
#define INVALID_HEIGHT       -100000.0f  // For check, real value for unknown height is INVALID_HEIGHT_VALUE
#define INVALID_HEIGHT_VALUE -200000.0f  // Real assigned value in unknown height case
#define MAX_FALL_DISTANCE     250000.0f  // "Unlimited fall" to find vMap ground if it is available, just larger than MAX_HEIGHT - INVALID_HEIGHT
#define MIN_HEIGHT           -500.0f

#define MAP_LIQUID_TYPE_NO_WATER    0x00
#define MAP_LIQUID_TYPE_WATER       0x01
#define MAP_LIQUID_TYPE_OCEAN       0x02
#define MAP_LIQUID_TYPE_MAGMA       0x04
#define MAP_LIQUID_TYPE_SLIME       0x08
#define MAP_LIQUID_TYPE_DARK_WATER  0x10

#define MAP_ALL_LIQUIDS (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME)

#define MAX_NUMBER_OF_GRIDS 64
#define MAX_NUMBER_OF_CELLS 8
#define SIZE_OF_GRIDS       533.3333f

#define MAP_AREA_NO_AREA             0x1

#define MAP_HEIGHT_NO_HEIGHT         0x1
#define MAP_HEIGHT_AS_INT16          0x2
#define MAP_HEIGHT_AS_INT8           0x4
#define MAP_HEIGHT_HAS_FLIGHT_BOUNDS 0x8

#define MAP_LIQUID_NO_TYPE           0x1
#define MAP_LIQUID_NO_HEIGHT         0x2

#define MMAP_MAGIC 0x4d4d4150  // 'MMAP'
#define MMAP_VERSION 19

#define MAP_FILE_NAME_FORMAT  "{:03}{:02}{:02}.map"
#define MMAP_FILE_NAME_FORMAT  "{:03}.mmap"
#define MMAP_TILE_FILE_NAME_FORMAT "{:03}{:02}{:02}.mmtile"

union uMapMagic
{
    char asChar[4];
    uint32 asUInt;
};

constexpr uMapMagic MapMagic        = { {'M', 'A', 'P', 'S'} };
constexpr uint32    MapVersionMagic = 9;
constexpr uMapMagic MapAreaMagic    = { {'A', 'R', 'E', 'A'} };
constexpr uMapMagic MapHeightMagic  = { {'M', 'H', 'G', 'T'} };
constexpr uMapMagic MapLiquidMagic  = { {'M', 'L', 'I', 'Q'} };

// ******************************************
// Map file format defines
// ******************************************
struct MapFileHeader
{
    uint32 mapMagic;
    uint32 versionMagic;
    uint32 buildMagic;
    uint32 areaMapOffset;
    uint32 areaMapSize;
    uint32 heightMapOffset;
    uint32 heightMapSize;
    uint32 liquidMapOffset;
    uint32 liquidMapSize;
    uint32 holesOffset;
    uint32 holesSize;
};

struct MapAreaHeader
{
    uint32 fourcc;
    uint16 flags;
    uint16 gridArea;
};

struct MapHeightHeader
{
    uint32 fourcc;
    uint32 flags;
    float gridHeight;
    float gridMaxHeight;
};

struct MapLiquidHeader
{
    uint32 fourcc;
    uint8 flags;
    uint8 liquidFlags;
    uint16 liquidType;
    uint8 offsetX;
    uint8 offsetY;
    uint8 width;
    uint8 height;
    float liquidLevel;
};


struct MmapTileRecastConfig
{
    float walkableSlopeAngle;

    uint8 walkableRadius;            // 1
    uint8 walkableHeight;            // 1
    uint8 walkableClimb;             // 1
    uint8 padding0{0};               // 1 → align next to 4

    uint32 vertexPerMapEdge;
    uint32 vertexPerTileEdge;
    uint32 tilesPerMapEdge;
    float baseUnitDim;
    float cellSizeHorizontal;
    float cellSizeVertical;
    float maxSimplificationError;

    bool operator==(const MmapTileRecastConfig& b) const {
        return walkableSlopeAngle == b.walkableSlopeAngle &&
               walkableRadius == b.walkableRadius &&
               walkableHeight == b.walkableHeight &&
               walkableClimb == b.walkableClimb &&
               vertexPerMapEdge == b.vertexPerMapEdge &&
               vertexPerTileEdge == b.vertexPerTileEdge &&
               tilesPerMapEdge == b.tilesPerMapEdge &&
               baseUnitDim == b.baseUnitDim &&
               cellSizeHorizontal == b.cellSizeHorizontal &&
               cellSizeVertical == b.cellSizeVertical &&
               maxSimplificationError == b.maxSimplificationError;
    }
};
static_assert(sizeof(MmapTileRecastConfig) == 36, "Unexpected size of MmapTileRecastConfig");

struct MmapTileHeader
{
    uint32 mmapMagic{MMAP_MAGIC};
    uint32 dtVersion;
    uint32 mmapVersion{MMAP_VERSION};
    uint32 size{0};

    MmapTileRecastConfig recastConfig{};

    MmapTileHeader() : dtVersion(DT_NAVMESH_VERSION) { }
};

// All padding fields must be handled and initialized to ensure mMapsGenerator will produce binary-identical *.mmtile files
static_assert(sizeof(MmapTileHeader) == 52, "MmapTileHeader size is not correct, adjust the padding field size");
static_assert(sizeof(MmapTileHeader) == (sizeof(MmapTileHeader::mmapMagic) +
              sizeof(MmapTileHeader::dtVersion) +
              sizeof(MmapTileHeader::mmapVersion) +
              sizeof(MmapTileHeader::size) +
              sizeof(MmapTileRecastConfig)), "MmapTileHeader has uninitialized padding fields");

enum NavTerrain
{
    // We only have 8 bits
    NAV_EMPTY   = 0x00,
    NAV_GROUND  = 0x01,
    NAV_MAGMA   = 0x02,
    NAV_SLIME   = 0x04,
    NAV_WATER   = 0x08,
    NAV_UNUSED1 = 0x10,
    NAV_UNUSED2 = 0x20,
    NAV_UNUSED3 = 0x40,
    NAV_UNUSED4 = 0x80
};

#endif
