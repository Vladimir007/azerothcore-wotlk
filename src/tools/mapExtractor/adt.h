#ifndef ADT_H
#define ADT_H

#include "Define.h"
#include "loadlib.h"

enum : uint8
{
    LIQUID_TYPE_WATER = 0,
    LIQUID_TYPE_OCEAN = 1,
    LIQUID_TYPE_MAGMA = 2,
    LIQUID_TYPE_SLIME = 3,
};

enum class LiquidVertexFormatType : uint16
{
    HeightDepth        = 0,
    HeightTextureCoord = 1,
    Depth              = 2,
};

#define ADT_CELLS_PER_GRID 16
#define ADT_CELL_SIZE      8
#define ADT_GRID_SIZE      (ADT_CELLS_PER_GRID * ADT_CELL_SIZE)

/// Adt file height map chunk
struct MCVT
{
    uint32 fcc;
    uint32 size;
    float heightMap[(ADT_CELL_SIZE + 1) * (ADT_CELL_SIZE + 1) + ADT_CELL_SIZE * ADT_CELL_SIZE];
    bool prepareLoadedData();
};

/// Adt file liquid map chunk (old)
struct MCLQ
{
    uint32 fcc;
    uint32 size;
    float height1;
    float height2;
    struct LiquidData
    {
        uint32 light;
        float  height;
    } liquid[ADT_CELL_SIZE + 1][ADT_CELL_SIZE + 1];

    // 1<<0 - ocean
    // 1<<1 - lava/slime
    // 1<<2 - water
    // 1<<6 - all water
    // 1<<7 - dark water
    // == 0x0F - do not show liquid
    uint8 flags[ADT_CELL_SIZE][ADT_CELL_SIZE];
    uint8 data[84];
    bool prepareLoadedData();
};

/// Adt file cell chunk
struct MCNK
{
    uint32 fcc;
    uint32 size;
    uint32 flags;
    uint32 ix;
    uint32 iy;
    uint32 nLayers;
    uint32 nDoodadRefs;
    uint32 offsMCVT;  // Height map
    uint32 offsMCNR;  // Normal vectors for each vertex
    uint32 offsMCLY;  // Texture layer definitions
    uint32 offsMCRF;  // A list of indices into the parent file's MDDF chunk
    uint32 offsMCAL;  // Alpha maps for additional texture layers
    uint32 sizeMCAL;
    uint32 offsMCSH;  // Shadow map for static shadows on the terrain
    uint32 sizeMCSH;
    uint32 areaID;
    uint32 nMapObjRefs;
    uint32 holes;
    uint16 s[2];
    uint32 data1;
    uint32 data2;
    uint32 data3;
    uint32 predTex;
    uint32 nEffectDoodad;
    uint32 offsMCSE;
    uint32 nSndEmitters;
    uint32 offsMCLQ;  // Liquid level (old)
    uint32 sizeMCLQ;
    float  zpos;
    float  xpos;
    float  ypos;
    uint32 offsMCCV;  // offsColorValues in WotLK
    uint32 props;
    uint32 effectID;

    bool prepareLoadedData();
    MCVT* getMCVT()
    {
        if (offsMCVT)
            return reinterpret_cast<MCVT*>(reinterpret_cast<uint8*>(this) + offsMCVT);
        return nullptr;
    }
    MCLQ* getMCLQ()
    {
        if (offsMCLQ)
            return reinterpret_cast<MCLQ*>(reinterpret_cast<uint8*>(this) + offsMCLQ);
        return nullptr;
    }
};

/// Adt file grid chunk
struct MCIN
{
    uint32 fcc;
    uint32 size;
    struct Cell
    {
        uint32 offsMCNK;
        uint32 size;
        uint32 flags;
        uint32 asyncId;
    } cells[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

    bool prepareLoadedData();

    // Offset from beginning of the file (used this-84)
    MCNK* getMCNK(const int x, const int y)
    {
        if (cells[x][y].offsMCNK)
            return reinterpret_cast<MCNK*>(reinterpret_cast<uint8*>(this) + cells[x][y].offsMCNK - 84);
        return nullptr;
    }
};

struct LiquidInstance
{
    uint16 LiquidType;  // Index from LiquidType.dbc
    LiquidVertexFormatType LiquidVertexFormat;
    float MinHeightLevel;
    float MaxHeightLevel;
    uint8 OffsetX;
    uint8 OffsetY;
    uint8 Width;
    uint8 Height;
    uint32 OffsetExistsBitmap;
    uint32 OffsetVertexData;

    uint8 GetOffsetX() const { return OffsetX; }
    uint8 GetOffsetY() const { return OffsetY; }
    uint8 GetWidth() const { return Width; }
    uint8 GetHeight() const { return Height; }
};

struct LiquidAttributes
{
    uint64 Fishable;
    uint64 Deep;
};

/// Adt file liquid data chunk (new)
struct MH2O
{
    uint32 fcc;
    uint32 size;

    struct LiquidData
    {
        uint32 OffsetInstances;
        uint32 used;
        uint32 OffsetAttributes;
    } liquid[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

    bool prepareLoadedData();

    const LiquidInstance* GetLiquidInstance(const int32 x, const int32 y) const
    {
        if (liquid[x][y].used && liquid[x][y].OffsetInstances)
            return reinterpret_cast<const LiquidInstance*>(reinterpret_cast<const uint8*>(this) + 8 + liquid[x][y].OffsetInstances);
        return nullptr;
    }

    LiquidAttributes GetLiquidAttributes(const int32 x, const int32 y) const
    {
        if (!liquid[x][y].used)
            return { 0, 0 };
        if (!liquid[x][y].OffsetAttributes)
            return { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF };
        return *reinterpret_cast<const LiquidAttributes*>(reinterpret_cast<const uint8*>(this) + 8 + liquid[x][y].OffsetAttributes);
    }

    static uint16 GetLiquidType(const LiquidInstance* h)
    {
        if (h->LiquidVertexFormat == LiquidVertexFormatType::Depth)
            return 2;
        return h->LiquidType;
    }

    float GetLiquidHeight(const LiquidInstance* h, const int32 pos) const
    {
        if (!h->OffsetVertexData)
            return 0.0f;

        switch (h->LiquidVertexFormat)
        {
            case LiquidVertexFormatType::HeightDepth:
            case LiquidVertexFormatType::HeightTextureCoord:
                return reinterpret_cast<const float*>(reinterpret_cast<const uint8*>(this) + 8 + h->OffsetVertexData)[pos];
            case LiquidVertexFormatType::Depth:
                return 0.0f;
            default:
                break;
        }
        return 0.0f;
    }

    int8 GetLiquidDepth(const LiquidInstance* h, const int32 pos) const
    {
        if (!h->OffsetVertexData)
            return -1;

        switch (h->LiquidVertexFormat)
        {
            case LiquidVertexFormatType::HeightDepth:
                return (reinterpret_cast<const int8*>(this) + 8 + h->OffsetVertexData + (h->GetWidth() + 1) * (h->GetHeight() + 1) * 4)[pos];
            case LiquidVertexFormatType::HeightTextureCoord:
                return 0;
            case LiquidVertexFormatType::Depth:
                return reinterpret_cast<const int8*>(reinterpret_cast<const uint8*>(this) + 8 + h->OffsetVertexData)[pos];
            default:
                break;
        }
        return 0;
    }

    const uint16* GetLiquidTextureCoordMap(const LiquidInstance* h, const int32 pos) const
    {
        if (!h->OffsetVertexData)
            return nullptr;

        switch (h->LiquidVertexFormat)
        {
            case LiquidVertexFormatType::HeightDepth:
            case LiquidVertexFormatType::Depth:
                return nullptr;
            case LiquidVertexFormatType::HeightTextureCoord:
                return reinterpret_cast<const uint16*>(
                    reinterpret_cast<const uint8*>(this) + 8 + h->OffsetVertexData + 4 * ((h->GetWidth() + 1) * (h->GetHeight() + 1) + pos)
                );
            default:
                break;
        }
        return nullptr;
    }

    uint64 GetLiquidExistsBitmap(const LiquidInstance* h) const
    {
        if (h->OffsetExistsBitmap)
            return *reinterpret_cast<const uint64*>(reinterpret_cast<const uint8*>(this) + 8 + h->OffsetExistsBitmap);
        return 0xFFFFFFFFFFFFFFFFuLL;
    }
};

/// Adt file min/max height chunk
struct MFBO
{
    uint32 fcc;
    uint32 size;
    struct plane
    {
        int16 coords[9];
    };
    plane max;
    plane min;

    bool prepareLoadedData();
};

/// Adt file header chunk
struct MHDR
{
    uint32 fcc;
    uint32 size;

    uint32 flags;
    uint32 offsMCIN;           // MCIN
    uint32 offsTex;            // MTEX
    uint32 offsModels;         // MMDX
    uint32 offsModelsIds;      // MMID
    uint32 offsMapObjects;     // MWMO
    uint32 offsMapObjectsIds;  // MWID
    uint32 offsDoodsDef;       // MDDF
    uint32 offsObjectsDef;     // MODF
    uint32 offsMFBO;           // MFBO
    uint32 offsMH2O;           // MH2O
    uint32 data[5];

    bool prepareLoadedData();
    MCIN* getMCIN()
    {
        return reinterpret_cast<MCIN*>(reinterpret_cast<uint8*>(&flags) + offsMCIN);
    }
    MH2O* getMH2O()
    {
        if (offsMH2O)
            return reinterpret_cast<MH2O*>(reinterpret_cast<uint8*>(&flags) + offsMH2O);
        return nullptr;
    }
    MFBO* getMFBO()
    {
        if (flags & 1 && offsMFBO)
            return reinterpret_cast<MFBO*>(reinterpret_cast<uint8*>(&flags) + offsMFBO);
        return nullptr;
    }
};

class FileADT : public FileLoader
{
public:
    bool prepareLoadedData() override;
    FileADT();
    ~FileADT() override;
    void free() override;

    MHDR* grid;
};

#endif
