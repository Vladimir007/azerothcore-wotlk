#include "model.h"

#include <cassert>
#include <cstdio>
#include <limits>
#include <G3D/Quat.h>

#include "adtfile.h"
#include "vMapExport.h"
#include "wmo.h"

Model::Model(std::string& filename) : filename(filename), header(), vertices(nullptr), indices(nullptr)
{
}

bool Model::open()
{
    MPQFile f(filename.c_str());

    if (f.isEof())
    {
        f.close();
        return false;
    }

    _unload();

    memcpy(&header, f.getBuffer(), sizeof(ModelHeader));
    if (!header.nBoundingTriangles)
    {
        f.close();
        return false;
    }

    f.seek(0);
    f.seekRelative(header.ofsBoundingVertices);
    vertices = new G3D::Vector3[header.nBoundingVertices];
    f.read(vertices, header.nBoundingVertices * 12);
    for (uint32 i = 0; i < header.nBoundingVertices; i++)
        vertices[i] = fixCoordSystem(vertices[i]);
    f.seek(0);
    f.seekRelative(header.ofsBoundingTriangles);
    indices = new uint16[header.nBoundingTriangles];
    f.read(indices, header.nBoundingTriangles * 2);
    f.close();
    return true;
}

bool Model::ConvertToVMAPModel(const std::string& outFileName)
{
    const int N[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    FILE* output = fopen(outFileName.c_str(), "wb");
    if (!output)
    {
        std::cerr << "Can't create the output file " << outFileName << std::endl;
        return false;
    }

    const uint32 nVertices = header.nBoundingVertices;
    const uint32 nIndexes = header.nBoundingTriangles;
    constexpr uint32 nBranches = 1;
    constexpr uint32 nGroups = 1;
    int wsize;

    fwrite(VMAP::RAW_VMAP_MAGIC, 8, 1, output);
    fwrite(&nVertices, sizeof(int), 1, output);
    fwrite(&nGroups, sizeof(uint32), 1, output);
    fwrite(N, 4 * 3, 1, output); // RootWmoID, Flags, GroupID
    fwrite(N, sizeof(float), 3 * 2, output); // bounds, only needed for WMO currently
    fwrite(N, 4, 1, output); // Liquid flags
    fwrite("GRP ", 4, 1, output);
    wsize = sizeof(nBranches) + sizeof(uint32) * nBranches;
    fwrite(&wsize, sizeof(int), 1, output);
    fwrite(&nBranches, sizeof(nBranches), 1, output);
    fwrite(&nIndexes, sizeof(uint32), 1, output);
    fwrite("INDX", 4, 1, output);
    wsize = sizeof(uint32) + sizeof(uint16) * nIndexes;
    fwrite(&wsize, sizeof(int), 1, output);
    fwrite(&nIndexes, sizeof(uint32), 1, output);
    if (nIndexes > 0)
    {
        for (uint32 i = 0; i < nIndexes; ++i)
        {
            if (i % 3 - 1 == 0 && i + 1 < nIndexes)
            {
                const uint16 tmp = indices[i];
                indices[i] = indices[i + 1];
                indices[i + 1] = tmp;
            }
        }
        fwrite(indices, sizeof(uint16), nIndexes, output);
    }
    fwrite("VERT", 4, 1, output);
    wsize = sizeof(int) + sizeof(float) * 3 * nVertices;
    fwrite(&wsize, sizeof(int), 1, output);
    fwrite(&nVertices, sizeof(int), 1, output);
    if (nVertices > 0)
    {
        for (uint32 vpos = 0; vpos < nVertices; ++vpos)
        {
            const float tmp = vertices[vpos].y;
            vertices[vpos].y = -vertices[vpos].z;
            vertices[vpos].z = tmp;
        }
        fwrite(vertices, sizeof(float) * 3, nVertices, output);
    }

    fclose(output);

    return true;
}

G3D::Vector3 fixCoordSystem(G3D::Vector3 const& v)
{
    return G3D::Vector3(v.x, v.z, -v.y);
}

void Doodad::Extract(ADT::MDDF const& doodadDef, const std::string& modelInstName, const uint32 mapID, const uint32 tileX, const uint32 tileY, FILE* pDirFile)
{
    // Get the correct number of vertices
    const std::string filepath = szWorkDirWmo / modelInstName;
    FILE* input = fopen(filepath.c_str(), "r+b");
    if (!input) return;
    fseek(input, 8, SEEK_SET);
    int nVertices;
    const int count = fread(&nVertices, sizeof (int), 1, input);
    fclose(input);
    if (count != 1 || nVertices == 0)
        return;

    // Scale factor
    const float sc = doodadDef.Scale / 1024.0f;

    const G3D::Vector3 position = fixCoords(doodadDef.Position);

    constexpr uint16 nameSet = 0;  // Not used for models
    const uint32 uniqueId = GenerateUniqueObjectId(doodadDef.UniqueId, 0);
    uint32 tcFlags = MOD_M2;
    if (tileX == 65 && tileY == 65)
        tcFlags |= MOD_WORLD_SPAWN;

    // Write mapID, tileX, tileY, Flags, NameSet, UniqueId, Pos, Rot, Scale, Name
    fwrite(&mapID, sizeof(uint32), 1, pDirFile);
    fwrite(&tileX, sizeof(uint32), 1, pDirFile);
    fwrite(&tileY, sizeof(uint32), 1, pDirFile);
    fwrite(&tcFlags, sizeof(uint32), 1, pDirFile);
    fwrite(&nameSet, sizeof(uint16), 1, pDirFile);
    fwrite(&uniqueId, sizeof(uint32), 1, pDirFile);
    fwrite(&position, sizeof(G3D::Vector3), 1, pDirFile);
    fwrite(&doodadDef.Rotation, sizeof(G3D::Vector3), 1, pDirFile);
    fwrite(&sc, sizeof(float), 1, pDirFile);

    const uint32 nameLen = modelInstName.size();
    fwrite(&nameLen, sizeof(uint32), 1, pDirFile);
    fwrite(modelInstName.c_str(), sizeof(char), nameLen, pDirFile);
}

void Doodad::ExtractSet(WMODoodadData const& doodadData, ADT::MODF const& wmo, const uint32 mapID, const uint32 tileX, const uint32 tileY, FILE* pDirFile)
{
    if (wmo.DoodadSet >= doodadData.Sets.size())
        return;

    const G3D::Vector3 wmoPosition(wmo.Position.z, wmo.Position.x, wmo.Position.y);
    const G3D::Matrix3 wmoRotation = G3D::Matrix3::fromEulerAnglesZYX(G3D::toRadians(wmo.Rotation.y), G3D::toRadians(wmo.Rotation.x), G3D::toRadians(wmo.Rotation.z));

    uint16 doodadId = 0;
    WMO::MODS const& doodadSetData = doodadData.Sets[wmo.DoodadSet];
    for (const uint16 doodadIndex : doodadData.References)
    {
        if (doodadIndex < doodadSetData.StartIndex ||
            doodadIndex >= doodadSetData.StartIndex + doodadSetData.Count)
            continue;

        WMO::MODD const& doodad = doodadData.Spawns[doodadIndex];

        std::string doodadPath(&doodadData.Paths[doodad.NameIndex]);
        fs::path modelInstName(GetPlainName(doodadPath));
        modelInstName.replace_extension(".m2");
        fs::path filepath = szWorkDirWmo / modelInstName.string();

        // Get the correct no of vertices
        FILE* input = fopen(filepath.c_str(), "r+b");
        if (!input)
            continue;
        fseek(input, 8, SEEK_SET);
        int nVertices;
        const int count = fread(&nVertices, sizeof(int), 1, input);
        fclose(input);
        if (count != 1 || nVertices == 0)
            continue;

        assert(doodadId < std::numeric_limits<uint16>::max());
        ++doodadId;

        G3D::Vector3 position = wmoPosition + wmoRotation * G3D::Vector3(doodad.Position.x, doodad.Position.y, doodad.Position.z);

        G3D::Vector3 rotation;
        (G3D::Quat(doodad.Rotation.x, doodad.Rotation.y, doodad.Rotation.z, doodad.Rotation.w).toRotationMatrix() * wmoRotation)
            .toEulerAnglesXYZ(rotation.z, rotation.x, rotation.y);

        rotation.z = G3D::toDegrees(rotation.z);
        rotation.x = G3D::toDegrees(rotation.x);
        rotation.y = G3D::toDegrees(rotation.y);

        uint16 nameSet = 0;  // Not used for models
        uint32 uniqueId = GenerateUniqueObjectId(wmo.UniqueId, doodadId);
        uint32 tcFlags = MOD_M2;
        if (tileX == 65 && tileY == 65)
            tcFlags |= MOD_WORLD_SPAWN;

        // Write mapID, tileX, tileY, Flags, NameSet, UniqueId, Pos, Rot, Scale, name
        fwrite(&mapID, sizeof(uint32), 1, pDirFile);
        fwrite(&tileX, sizeof(uint32), 1, pDirFile);
        fwrite(&tileY, sizeof(uint32), 1, pDirFile);
        fwrite(&tcFlags, sizeof(uint32), 1, pDirFile);
        fwrite(&nameSet, sizeof(uint16), 1, pDirFile);
        fwrite(&uniqueId, sizeof(uint32), 1, pDirFile);
        fwrite(&position, sizeof(G3D::Vector3), 1, pDirFile);
        fwrite(&rotation, sizeof(G3D::Vector3), 1, pDirFile);
        fwrite(&doodad.Scale, sizeof(float), 1, pDirFile);

        uint32 nameLen = modelInstName.string().size();
        fwrite(&nameLen, sizeof(uint32), 1, pDirFile);
        fwrite(modelInstName.c_str(), sizeof(char), nameLen, pDirFile);
    }
}
