#include "wmo.h"
#include <cassert>
#include <cstdio>
#include <map>

#include "adtfile.h"
#include "MapDefines.h"
#include "vMapExport.h"

uMapMagic MOHDMagic = { { 'D', 'H', 'O', 'M' } };
uMapMagic MODSMagic = { { 'S', 'D', 'O', 'M' } };
uMapMagic MODNMagic = { { 'N', 'D', 'O', 'M' } };
uMapMagic MODDMagic = { { 'D', 'D', 'O', 'M' } };
uMapMagic MOGNMagic = { { 'N', 'G', 'O', 'M' } };

uMapMagic MOGPMagic = { { 'P', 'G', 'O', 'M' } };
uMapMagic MOPYMagic = { { 'Y', 'P', 'O', 'M' } };
uMapMagic MOVIMagic = { { 'I', 'V', 'O', 'M' } };
uMapMagic MOVTMagic = { { 'T', 'V', 'O', 'M' } };
uMapMagic MOBAMagic = { { 'A', 'B', 'O', 'M' } };
uMapMagic MODRMagic = { { 'R', 'D', 'O', 'M' } };
uMapMagic MLIQMagic = { { 'Q', 'I', 'L', 'M' } };

WMORoot::WMORoot(std::string const& filename) :
    filename(filename), color(0), nTextures(0), nGroups(0), nPortals(0), nLights(0),
    nDoodadNames(0), nDoodadDefs(0), nDoodadSets(0), RootWMOid(0), flags(0)
{
    memset(bbCorn1, 0, sizeof(bbCorn1));
    memset(bbCorn2, 0, sizeof(bbCorn2));
}

bool WMORoot::open()
{
    MPQFile f(filename.c_str());
    if (f.isEof())
    {
        std::cerr << "Can't open WMO root file." << std::endl;
        return false;
    }

    uint32 token;
    uint32 size;

    while (!f.isEof())
    {
        f.read(&token, 4);
        f.read(&size, 4);

        const std::size_t nextPos = f.getPos() + size;

        if (token == MOHDMagic.asUInt) // Header
        {
            f.read(&nTextures, 4);
            f.read(&nGroups, 4);
            f.read(&nPortals, 4);
            f.read(&nLights, 4);
            f.read(&nDoodadNames, 4);
            f.read(&nDoodadDefs, 4);
            f.read(&nDoodadSets, 4);
            f.read(&color, 4);
            f.read(&RootWMOid, 4);
            f.read(bbCorn1, 12);
            f.read(bbCorn2, 12);
            f.read(&flags, 4);
        }
        else if (token == MODSMagic.asUInt)
        {
            DoodadData.Sets.resize(size / sizeof(WMO::MODS));
            f.read(DoodadData.Sets.data(), size);
        }
        else if (token == MODNMagic.asUInt)
        {
            const char* ptr = f.getPointer();
            const char* end = ptr + size;
            DoodadData.Paths = std::make_unique<char[]>(size);
            memcpy(DoodadData.Paths.get(), ptr, size);
            while (ptr < end)
            {
                std::string path(ptr);
                uint32 doodadNameIndex = ptr - f.getPointer();
                if (std::string name; ExtractSingleModel(path, name))
                    ValidDoodadNames.insert(doodadNameIndex);
                ptr += path.length() + 1;
            }
        }
        else if (token == MODDMagic.asUInt)
        {
            DoodadData.Spawns.resize(size / sizeof(WMO::MODD));
            f.read(DoodadData.Spawns.data(), size);
        }
        else if (token == MOGNMagic.asUInt)
        {
            GroupNames.resize(size);
            f.read(GroupNames.data(), size);
        }
        f.seek(static_cast<int>(nextPos));
    }
    f.close ();
    return true;
}

bool WMORoot::ConvertToVMAPRootWmo(FILE* pOutFile)
{
    fwrite(VMAP::RAW_VMAP_MAGIC, 1, 8, pOutFile);
    constexpr unsigned int nVectors = 0;
    fwrite(&nVectors, sizeof(nVectors), 1, pOutFile); // Will be filled later
    fwrite(&nGroups, 4, 1, pOutFile);
    fwrite(&RootWMOid, 4, 1, pOutFile);
    return true;
}

WMOGroup::WMOGroup(std::string const& filename) :
    filename(std::move(filename)), MOPY(nullptr), MOVI(nullptr), MoviEx(nullptr), MOVT(nullptr), MOBA(nullptr), MobaEx(nullptr),
    hlq(nullptr), LiquEx(nullptr), LiquBytes(nullptr), groupName(0), descGroupName(0), mogpFlags(0),
    moprIdx(0), moprNItems(0), nBatchA(0), nBatchB(0), nBatchC(0), fogIdx(0),
    groupLiquid(0), groupWMOid(0), mopy_size(0), moba_size(0), LiquEx_size(0),
    nVertices(0), nTriangles(0), liquidFlags(0)
{
    memset(bbCorn1, 0, sizeof(bbCorn1));
    memset(bbCorn2, 0, sizeof(bbCorn2));
}

bool WMOGroup::open(const WMORoot* rootWMO)
{
    MPQFile f(filename.c_str());
    if (f.isEof ())
    {
        std::cerr << "Can't open WMO group file." << std::endl;
        return false;
    }

    uint32 token;
    uint32 size;
    while (!f.isEof())
    {
        f.read(&token, 4);
        f.read(&size, 4);

        if (token == MOGPMagic.asUInt) // Fix sizeof = Data size.
            size = 68;

        const std::size_t nextPos = f.getPos() + size;
        LiquEx_size = 0;
        liquidFlags = 0;

        if (token == MOGPMagic.asUInt) // Header
        {
            f.read(&groupName, 4);
            f.read(&descGroupName, 4);
            f.read(&mogpFlags, 4);
            f.read(bbCorn1, 12);
            f.read(bbCorn2, 12);
            f.read(&moprIdx, 2);
            f.read(&moprNItems, 2);
            f.read(&nBatchA, 2);
            f.read(&nBatchB, 2);
            f.read(&nBatchC, 4);
            f.read(&fogIdx, 4);
            f.read(&groupLiquid, 4);
            f.read(&groupWMOid, 4);

            // According to WoW.Dev Wiki:
            if (rootWMO->flags & 4)
                groupLiquid = GetLiquidTypeId(groupLiquid);
            else if (groupLiquid == 15)
                groupLiquid = 0;
            else
                groupLiquid = GetLiquidTypeId(groupLiquid + 1);

            if (groupLiquid)
                liquidFlags |= 2;
        }
        else if (token == MOPYMagic.asUInt)
        {
            MOPY = new char[size];
            mopy_size = size;
            nTriangles = static_cast<int>(size) / 2;
            f.read(MOPY, size);
        }
        else if (token == MOVIMagic.asUInt)
        {
            MOVI = new uint16[size / 2];
            f.read(MOVI, size);
        }
        else if (token == MOVTMagic.asUInt)
        {
            MOVT = new float[size / 4];
            f.read(MOVT, size);
            nVertices = static_cast<int>(size) / 12;
        }
        else if (token == MOBAMagic.asUInt)
        {
            MOBA = new uint16[size / 2];
            moba_size = size / 2;
            f.read(MOBA, size);
        }
        else if (token == MODRMagic.asUInt)
        {
            DoodadReferences.resize(size / sizeof(uint16));
            f.read(DoodadReferences.data(), size);
        }
        else if (token == MLIQMagic.asUInt)
        {
            liquidFlags |= 1;
            hlq = new WMOLiquidHeader();
            f.read(hlq, sizeof(WMOLiquidHeader));
            LiquEx_size = sizeof(WMOLiquidVert) * hlq->xverts * hlq->yverts;
            LiquEx = new WMOLiquidVert[hlq->xverts * hlq->yverts];
            f.read(LiquEx, LiquEx_size);
            const int nLiquBytes = hlq->xtiles * hlq->ytiles;
            LiquBytes = new char[nLiquBytes];
            f.read(LiquBytes, nLiquBytes);

            // Determine legacy liquid type
            if (!groupLiquid)
            {
                for (int i = 0; i < hlq->xtiles * hlq->ytiles; ++i)
                {
                    if ((LiquBytes[i] & 0xF) != 15)
                    {
                        groupLiquid = GetLiquidTypeId((LiquBytes[i] & 0xF) + 1);
                        break;
                    }
                }
            }
        }
        f.seek(static_cast<int>(nextPos));
    }
    f.close();
    return true;
}

int WMOGroup::ConvertToVMAPGroupWmo(FILE* output)
{
    fwrite(&mogpFlags, sizeof(uint32), 1, output);
    fwrite(&groupWMOid, sizeof(uint32), 1, output);
    // Group bound
    fwrite(bbCorn1, sizeof(float), 3, output);
    fwrite(bbCorn2, sizeof(float), 3, output);
    fwrite(&liquidFlags, sizeof(uint32), 1, output);

    constexpr char GRP[] = "GRP ";
    fwrite(GRP, 1, 4, output);

    int k = 0;
    const int mobaBatch = moba_size / 12;
    MobaEx = new int[mobaBatch * 4];
    for (int i = 8; i < moba_size; i += 12)
        MobaEx[k++] = MOBA[i];
    const int mobaSizeGrp = mobaBatch * 4 + 4;
    fwrite(&mobaSizeGrp, 4, 1, output);
    fwrite(&mobaBatch, 4, 1, output);
    fwrite(MobaEx, 4, k, output);
    delete [] MobaEx;

    const uint32 nIndexes = nTriangles * 3;

    if (fwrite("INDX", 4, 1, output) != 1)
        Abort("Error while writing file nBranches ID");

    int wsize = sizeof(uint32) + sizeof(unsigned short) * nIndexes;
    if (fwrite(&wsize, sizeof(int), 1, output) != 1)
        Abort("Error while writing file wsize");

    if (fwrite(&nIndexes, sizeof(uint32), 1, output) != 1)
        Abort("Error while writing file nIndexes");

    if (nIndexes > 0)
        if (fwrite(MOVI, sizeof(unsigned short), nIndexes, output) != nIndexes)
            Abort("Error while writing file indexArray");

    if (fwrite("VERT", 4, 1, output) != 1)
        Abort("Error while writing file nBranches ID");
    wsize = sizeof(int) + sizeof(float) * 3 * nVertices;
    if (fwrite(&wsize, sizeof(int), 1, output) != 1)
        Abort("Error while writing file wsize");

    if (fwrite(&nVertices, sizeof(int), 1, output) != 1)
        Abort("Error while writing file nVertices");
    if (nVertices > 0)
        if (fwrite(MOVT, sizeof(float) * 3, nVertices, output) != nVertices)
            Abort("Error while writing file vectors");

    if (liquidFlags & 3)
    {
        int LIQU_totalSize = sizeof(uint32);
        if (liquidFlags & 1)
        {
            LIQU_totalSize += sizeof(WMOLiquidHeader);
            LIQU_totalSize += LiquEx_size / sizeof(WMOLiquidVert) * sizeof(float);
            LIQU_totalSize += hlq->xtiles * hlq->ytiles;
        }

        const int LIQU_h[] = { 0x5551494C, LIQU_totalSize };  // "LIQU"
        fwrite(LIQU_h, 4, 2, output);

        fwrite(&groupLiquid, sizeof(uint32), 1, output);
        if (liquidFlags & 1)
        {
            fwrite(hlq, sizeof(WMOLiquidHeader), 1, output);

            // Only need height values, the other values are unknown anyway
            for (uint32 i = 0; i < LiquEx_size / sizeof(WMOLiquidVert); ++i)
                fwrite(&LiquEx[i].height, sizeof(float), 1, output);
            fwrite(LiquBytes, 1, hlq->xtiles * hlq->ytiles, output);
        }
    }

    return nTriangles;
}

uint32 WMOGroup::GetLiquidTypeId(const uint32 liquidTypeId)
{
    if (liquidTypeId < 21 && liquidTypeId)
    {
        switch ((static_cast<uint8>(liquidTypeId) - 1) & 3)
        {
            case 0: return ((mogpFlags & 0x80000) != 0) + 13;
            case 1: return 14;
            case 2: return 19;
            case 3: return 20;
            default: break;
        }
    }
    return liquidTypeId;
}

bool WMOGroup::ShouldSkip(WMORoot const* root) const
{
    // Skip unreachable
    if (mogpFlags & 0x80)
        return true;

    // Skip antiportals
    if (mogpFlags & 0x4000000)
        return true;

    if (groupName < static_cast<int32>(root->GroupNames.size()) && !strcmp(&root->GroupNames[groupName], "antiportal"))
        return true;

    return false;
}

WMOGroup::~WMOGroup()
{
    delete [] MOPY;
    delete [] MOVI;
    delete [] MOVT;
    delete [] MOBA;
    delete hlq;
    delete [] LiquEx;
    delete [] LiquBytes;
}

void MapObject::Extract(ADT::MODF const& mapObjDef, const std::string& WmoInstName, const uint32 mapID, const uint32 tileX, const uint32 tileY, FILE* pDirFile)
{
    // Destructible wmo, do not dump. We can handle the vMap for these in dynamic tree (gameobject vMaps)
    if ((mapObjDef.Flags & 0x1) != 0)
        return;

    // Get the correct number of vertices
    const fs::path filepath = szWorkDirWmo / WmoInstName;
    FILE* input = fopen(filepath.c_str(), "r+b");
    if (!input)
    {
        std::cerr << "MapObject::Extract: couldn't open file " << filepath.string() << std::endl;
        return;
    }
    fseek(input, 8, SEEK_SET);
    int nVertices;
    const int count = fread(&nVertices, sizeof(int), 1, input);
    fclose(input);

    if (count != 1 || nVertices == 0)
        return;

    G3D::Vector3 position = mapObjDef.Position;

    if (position.x == 0 && position.z == 0)
    {
        position.x = 533.33333f * 32;
        position.z = 533.33333f * 32;
    }

    position = fixCoords(position);
    const G3D::AABox bounds(fixCoords(mapObjDef.Bounds.low()), fixCoords(mapObjDef.Bounds.high()));

    constexpr float scale = 1.0f;
    const uint32 uniqueId = GenerateUniqueObjectId(mapObjDef.UniqueId, 0);
    uint32 flags = MOD_HAS_BOUND;
    if (tileX == 65 && tileY == 65) flags |= MOD_WORLD_SPAWN;
    // Write mapID, tileX, tileY, Flags, NameSet, UniqueId, Pos, Rot, Scale, Bounds, Name
    fwrite(&mapID, sizeof(uint32), 1, pDirFile);
    fwrite(&tileX, sizeof(uint32), 1, pDirFile);
    fwrite(&tileY, sizeof(uint32), 1, pDirFile);
    fwrite(&flags, sizeof(uint32), 1, pDirFile);
    fwrite(&mapObjDef.NameSet, sizeof(uint16), 1, pDirFile);
    fwrite(&uniqueId, sizeof(uint32), 1, pDirFile);
    fwrite(&position, sizeof(G3D::Vector3), 1, pDirFile);
    fwrite(&mapObjDef.Rotation, sizeof(G3D::Vector3), 1, pDirFile);
    fwrite(&scale, sizeof(float), 1, pDirFile);
    fwrite(&bounds, sizeof(G3D::AABox), 1, pDirFile);

    const uint32 nameLen = WmoInstName.size();
    fwrite(&nameLen, sizeof(uint32), 1, pDirFile);
    fwrite(WmoInstName.c_str(), sizeof(char), nameLen, pDirFile);
}
