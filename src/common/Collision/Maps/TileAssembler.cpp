#include "TileAssembler.h"

#include <iomanip>
#include <set>
#include <sstream>
#include <boost/filesystem.hpp>

#include "BoundingIntervalHierarchy.h"
#include "MapDefines.h"
#include "MapTree.h"
#include "VMapDefinitions.h"

using G3D::Vector3;
using G3D::AABox;
using G3D::inf;
using std::pair;

template<> struct BoundsTrait<VMAP::ModelSpawn*>
{
    static void GetBounds(const VMAP::ModelSpawn* const& obj, AABox& out) { out = obj->GetBounds(); }
};

namespace VMAP
{
    Vector3 ModelPosition::transform(const Vector3& pIn) const
    {
        Vector3 out = pIn * iScale;
        out = iRotation * out;
        return out;
    }

    TileAssembler::TileAssembler(const std::string& pSrcDirName, const std::string& pDestDirName)
        : iDestDir(pDestDirName), iSrcDir(pSrcDirName)
    {
        boost::filesystem::create_directory(iDestDir);
    }

    TileAssembler::~TileAssembler()
    {
    }

    bool TileAssembler::convertWorld()
    {
        bool success = readMapSpawns();
        if (!success)
            return false;

        // Export Map data
        for (auto map_iter = mapData.begin(); map_iter != mapData.end() && success; ++map_iter)
        {
            // Build global map tree
            std::vector<ModelSpawn*> mapSpawns;
            UniqueEntryMap::iterator entry;
            printf("Calculating model bounds for map %u...\n", map_iter->first);
            for (entry = map_iter->second->UniqueEntries.begin(); entry != map_iter->second->UniqueEntries.end(); ++entry)
            {
                // M2 models don't have a bound set in WDT/ADT placement data, I still think they're not used for LoS at all on retail
                if (entry->second.flags & MOD_M2)
                {
                    if (!calculateTransformedBound(entry->second))
                        break;
                }
                else if (entry->second.flags & MOD_WORLD_SPAWN) // WMO maps and terrain maps use different origin, so we need to adapt :/
                    entry->second.iBound = entry->second.iBound + Vector3(533.33333f * 32, 533.33333f * 32, 0.f);
                mapSpawns.push_back(&entry->second);
                spawnedModelFiles.insert(entry->second.name);
            }

            printf("Creating map tree for map %u...\n", map_iter->first);
            BIH pTree;

            try
            {
                pTree.build(mapSpawns, BoundsTrait<ModelSpawn*>::GetBounds);
            }
            catch (std::exception& e)
            {
                printf("Exception ""%s"" when calling pTree.build", e.what());
                return false;
            }

            std::map<uint32, uint32> modelNodeIdx;
            for (uint32 i = 0; i < mapSpawns.size(); ++i)
                modelNodeIdx.insert(pair(mapSpawns[i]->ID, i));

            // write map tree file
            std::stringstream mapFilename;
            mapFilename << iDestDir << '/' << std::setfill('0') << std::setw(3) << map_iter->first << ".vmtree";
            FILE* mapFile = fopen(mapFilename.str().c_str(), "wb");
            if (!mapFile)
            {
                success = false;
                printf("Cannot open %s\n", mapFilename.str().c_str());
                break;
            }

            // General info
            if (fwrite(VMAP_MAGIC, 1, 8, mapFile) != 8) { success = false; }
            uint32 globalTileID = StaticMapTree::packTileID(65, 65);
            auto [globalFirst, globalEnd] = map_iter->second->TileEntries.equal_range(globalTileID);
            char isTiled = globalFirst == globalEnd; // Only maps without terrain (tiles) have global WMO
            if (success && fwrite(&isTiled, sizeof(char), 1, mapFile) != 1) { success = false; }
            // Nodes
            if (success && fwrite("NODE", 4, 1, mapFile) != 1) { success = false; }
            if (success) { success = pTree.writeToFile(mapFile); }
            // Global map spawns (WDT), if any (most instances)
            if (success && fwrite("GOBJ", 4, 1, mapFile) != 1) { success = false; }

            for (auto glob = globalFirst; glob != globalEnd && success; ++glob)
                success = ModelSpawn::writeToFile(mapFile, map_iter->second->UniqueEntries[glob->second]);

            fclose(mapFile);

            // Write map tile files, similar to ADT files, only with extra BSP tree node info
            TileMap& tileEntries = map_iter->second->TileEntries;
            TileMap::iterator tile;
            for (tile = tileEntries.begin(); tile != tileEntries.end(); ++tile)
            {
                if (map_iter->second->UniqueEntries[tile->second].flags & MOD_WORLD_SPAWN) // WDT spawn, saved as tile 65/65 currently...
                    continue;
                uint32 nSpawns = tileEntries.count(tile->first);
                std::stringstream tileFilename;
                tileFilename.fill('0');
                tileFilename << iDestDir << '/' << std::setw(3) << map_iter->first << '_';
                uint32 x, y;
                StaticMapTree::unpackTileID(tile->first, x, y);
                tileFilename << std::setw(2) << x << '_' << std::setw(2) << y << ".vmtile";
                if (FILE* tileFile = fopen(tileFilename.str().c_str(), "wb"))
                {
                    // File header
                    if (success && fwrite(VMAP_MAGIC, 1, 8, tileFile) != 8) { success = false; }
                    // Write number of tile spawns
                    if (success && fwrite(&nSpawns, sizeof(uint32), 1, tileFile) != 1) { success = false; }
                    // Write tile spawns
                    for (uint32 s = 0; s < nSpawns; ++s)
                    {
                        if (s)
                            ++tile;
                        const ModelSpawn& spawn2 = map_iter->second->UniqueEntries[tile->second];
                        success = success && ModelSpawn::writeToFile(tileFile, spawn2);
                        // MapTree nodes to update when loading tile
                        auto nIdx = modelNodeIdx.find(spawn2.ID);
                        if (success && fwrite(&nIdx->second, sizeof(uint32), 1, tileFile) != 1) { success = false; }
                    }
                    fclose(tileFile);
                }
            }
        }

        // Add an object models, listed in temp_gameobject_models file
        exportGameobjectModels();

        // Export objects
        std::cout << "\nConverting Model Files" << std::endl;
        for (auto mFile = spawnedModelFiles.begin(); mFile != spawnedModelFiles.end(); ++mFile)
        {
            std::cout << "Converting " << *mFile << std::endl;
            if (!convertRawFile(*mFile))
            {
                std::cout << "error converting " << *mFile << std::endl;
                success = false;
                break;
            }
        }

        // Cleanup
        for (auto map_iter = mapData.begin(); map_iter != mapData.end(); ++map_iter)
            delete map_iter->second;
        return success;
    }

    bool TileAssembler::readMapSpawns()
    {
        const std::string fName = iSrcDir + "/dir_bin";
        FILE* dirF = fopen(fName.c_str(), "rb");
        if (!dirF)
        {
            printf("Could not read dir_bin file!\n");
            return false;
        }
        printf("Read coordinate mapping...\n");
        uint32 mapID, tileX, tileY;
        ModelSpawn spawn;
        while (!feof(dirF))
        {
            // Read mapID, tileX, tileY, Flags, NameSet, UniqueId, Pos, Rot, Scale, Bound_lo, Bound_hi, name
            if (!fread(&mapID, sizeof(uint32), 1, dirF)) // EoF
                break;
            fread(&tileX, sizeof(uint32), 1, dirF);
            fread(&tileY, sizeof(uint32), 1, dirF);
            if (!ModelSpawn::readFromFile(dirF, spawn))
                break;

            MapSpawns* current;
            if (auto map_iter = mapData.find(mapID); map_iter == mapData.end())
            {
                printf("spawning Map %d\n", mapID);
                mapData[mapID] = new MapSpawns();
                current = mapData[mapID];
            }
            else
                current = map_iter->second;

            current->UniqueEntries.emplace(spawn.ID, spawn);
            current->TileEntries.insert(pair(StaticMapTree::packTileID(tileX, tileY), spawn.ID));
        }
        const bool success = ferror(dirF) == 0;
        fclose(dirF);
        return success;
    }

    bool TileAssembler::calculateTransformedBound(ModelSpawn& spawn)
    {
        std::string modelFilename(iSrcDir);
        modelFilename.push_back('/');
        modelFilename.append(spawn.name);

        ModelPosition modelPosition;
        modelPosition.iDir = spawn.iRot;
        modelPosition.iScale = spawn.iScale;
        modelPosition.init();

        WorldModel_Raw raw_model;
        if (!raw_model.Read(modelFilename.c_str()))
            return false;

        const uint32 groups = raw_model.groupsArray.size();
        if (groups != 1)
            printf("Warning: '%s' does not seem to be a M2 model!\n", modelFilename.c_str());

        AABox modelBound;
        bool boundEmpty = true;

        for (uint32 g = 0; g < groups; ++g) // Should be only one for M2 files
        {
            std::vector<Vector3>& vertices = raw_model.groupsArray[g].vertexArray;

            if (vertices.empty())
            {
                std::cout << "error: model '" << spawn.name << "' has no geometry!" << std::endl;
                continue;
            }

            for (uint32 i = 0; i < vertices.size(); ++i)
            {
                Vector3 v = modelPosition.transform(vertices[i]);

                if (boundEmpty)
                    modelBound = AABox(v, v), boundEmpty = false;
                else
                    modelBound.merge(v);
            }
        }
        spawn.iBound = modelBound + spawn.iPos;
        spawn.flags |= MOD_HAS_BOUND;
        return true;
    }

#pragma pack(push, 1)
    struct WMOLiquidHeader
    {
        int xverts, yverts, xtiles, ytiles;
        float pos_x;
        float pos_y;
        float pos_z;
        [[maybe_unused]] short material;
    };
#pragma pack(pop)

    bool TileAssembler::convertRawFile(const std::string& pModelFilename)
    {
        std::string filename = iSrcDir;
        if (filename.length() > 0)
            filename.push_back('/');
        filename.append(pModelFilename);

        WorldModel_Raw raw_model;
        if (!raw_model.Read(filename.c_str()))
            return false;

        // Write WorldModel
        WorldModel model;
        model.setRootWmoID(raw_model.RootWMOid);
        if (!raw_model.groupsArray.empty())
        {
            std::vector<GroupModel> groupsArray;

            for (uint32 g = 0; g < raw_model.groupsArray.size(); ++g)
            {
                GroupModel_Raw& raw_group = raw_model.groupsArray[g];
                groupsArray.push_back(GroupModel(raw_group.mogpFlags, raw_group.GroupWMOid, raw_group.bounds ));
                groupsArray.back().setMeshData(raw_group.vertexArray, raw_group.triangles);
                groupsArray.back().setLiquidData(raw_group.liquid);
            }

            model.setGroupModels(groupsArray);
        }

        // Write WorldModel file
        FILE* wf = fopen((iDestDir + "/" + pModelFilename + ".vmo").c_str(), "wb");
        if (!wf)
            return false;
        const bool result = model.writeFile(wf);
        fclose(wf);
        return result;
    }

    void TileAssembler::exportGameobjectModels()
    {
        FILE* model_list = fopen((iSrcDir + "/" + "temp_gameobject_models").c_str(), "rb");
        if (!model_list)
            return;

        char ident[8];
        if (fread(ident, 1, 8, model_list) != 8 || memcmp(ident, RAW_VMAP_MAGIC, 8) != 0)
        {
            fclose(model_list);
            return;
        }

        FILE* model_list_copy = fopen((iDestDir + "/" + GAMEOBJECT_MODELS).c_str(), "wb");
        if (!model_list_copy)
        {
            fclose(model_list);
            return;
        }

        fwrite(VMAP_MAGIC, 1, 8, model_list_copy);

        uint32 name_length, displayId;
        uint8 isWmo;
        char buff[500];
        while (!feof(model_list))
        {
            if (fread(&displayId, sizeof(uint32), 1, model_list) != 1)
                if (feof(model_list))   // EOF flag is only set after failed reading attempt
                    break;

            if (fread(&isWmo, sizeof(uint8), 1, model_list) != 1 ||
                fread(&name_length, sizeof(uint32), 1, model_list) != 1 ||
                name_length >= sizeof(buff) ||
                fread(&buff, sizeof(char), name_length, model_list) != name_length)
            {
                std::cout << "\nFile 'temp_gameobject_models' seems to be corrupted" << std::endl;
                break;
            }

            std::string model_name(buff, name_length);

            WorldModel_Raw raw_model;
            if (!raw_model.Read((iSrcDir + "/" + model_name).c_str()))
                continue;

            spawnedModelFiles.insert(model_name);
            AABox bounds;
            bool boundEmpty = true;
            for (uint32 g = 0; g < raw_model.groupsArray.size(); ++g)
            {
                std::vector<Vector3>& vertices = raw_model.groupsArray[g].vertexArray;

                for (uint32 i = 0; i < vertices.size(); ++i)
                {
                    Vector3& v = vertices[i];
                    if (boundEmpty)
                        bounds = AABox(v, v), boundEmpty = false;
                    else
                        bounds.merge(v);
                }
            }

            fwrite(&displayId, sizeof(uint32), 1, model_list_copy);
            fwrite(&isWmo, sizeof(uint8), 1, model_list_copy);
            fwrite(&name_length, sizeof(uint32), 1, model_list_copy);
            fwrite(&buff, sizeof(char), name_length, model_list_copy);
            fwrite(&bounds.low(), sizeof(Vector3), 1, model_list_copy);
            fwrite(&bounds.high(), sizeof(Vector3), 1, model_list_copy);
        }

        fclose(model_list);
        fclose(model_list_copy);
    }

// Temporary use defines to simplify read/check code (close file and return at fail)
#define READ_OR_RETURN(V, S) if (fread((V), (S), 1, rf) != 1) { fclose(rf); printf("readfail\n"); return false; }
#define READ_OR_RETURN_WITH_DELETE(V, S) if (fread((V), (S), 1, rf) != 1) { fclose(rf); printf("readfail\n"); delete[] V; return false; };
#define CMP_OR_RETURN(V, S)  if (strcmp((V), (S)) != 0) { fclose(rf); printf("cmpfail, %s!=%s\n", V, S); return false; }

    bool GroupModel_Raw::Read(FILE* rf)
    {
        char blockId[5];
        blockId[4] = 0;
        int blocksize;

        READ_OR_RETURN(&mogpFlags, sizeof(uint32));
        READ_OR_RETURN(&GroupWMOid, sizeof(uint32));

        Vector3 vec1, vec2;
        READ_OR_RETURN(&vec1, sizeof(Vector3));
        READ_OR_RETURN(&vec2, sizeof(Vector3));
        bounds.set(vec1, vec2);

        READ_OR_RETURN(&liquidFlags, sizeof(uint32));

        // Will this ever be used? What is it good for anyway?
        uint32 branches;
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "GRP ");
        READ_OR_RETURN(&blocksize, sizeof(int));
        READ_OR_RETURN(&branches, sizeof(uint32));
        for (uint32 b = 0; b < branches; ++b)
        {
            uint32 indexes;
            // Indexes for each branch (not used jet)
            READ_OR_RETURN(&indexes, sizeof(uint32));
        }

        // Indexes
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "INDX");
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nIndexes;
        READ_OR_RETURN(&nIndexes, sizeof(uint32));
        if (nIndexes > 0)
        {
            const auto indexArray = new uint16[nIndexes];
            READ_OR_RETURN_WITH_DELETE(indexArray, nIndexes * sizeof(uint16));
            triangles.reserve(nIndexes / 3);
            for (uint32 i = 0; i < nIndexes; i += 3)
                triangles.push_back(MeshTriangle(indexArray[i], indexArray[i + 1], indexArray[i + 2]));
            delete[] indexArray;
        }

        // Vectors
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "VERT");
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nVectors;
        READ_OR_RETURN(&nVectors, sizeof(uint32));

        if (nVectors > 0)
        {
            const auto vectorArray = new float[nVectors * 3];
            READ_OR_RETURN_WITH_DELETE(vectorArray, nVectors * sizeof(float) * 3);
            for (uint32 i = 0; i < nVectors; ++i)
                vertexArray.push_back( Vector3(vectorArray + 3 * i));
            delete[] vectorArray;
        }

        // Liquid
        liquid = nullptr;
        if (liquidFlags & 3)
        {
            READ_OR_RETURN(&blockId, 4);
            CMP_OR_RETURN(blockId, "LIQU");
            READ_OR_RETURN(&blocksize, sizeof(int));
            uint32 liquidType;
            READ_OR_RETURN(&liquidType, sizeof(uint32));
            if (liquidFlags & 1)
            {
                WMOLiquidHeader hlq;
                READ_OR_RETURN(&hlq, sizeof(WMOLiquidHeader));
                liquid = new WmoLiquid(hlq.xtiles, hlq.ytiles, Vector3(hlq.pos_x, hlq.pos_y, hlq.pos_z), liquidType);
                uint32 size = hlq.xverts * hlq.yverts;
                READ_OR_RETURN(liquid->GetHeightStorage(), size * sizeof(float));
                size = hlq.xtiles * hlq.ytiles;
                READ_OR_RETURN(liquid->GetFlagsStorage(), size);
            }
            else
            {
                liquid = new WmoLiquid(0, 0, Vector3::zero(), liquidType);
                liquid->GetHeightStorage()[0] = bounds.high().z;
            }
        }

        return true;
    }

    GroupModel_Raw::~GroupModel_Raw()
    {
        delete liquid;
    }

    bool WorldModel_Raw::Read(const char* path)
    {
        FILE* rf = fopen(path, "rb");
        if (!rf)
        {
            printf("ERROR: Can't open raw model file: %s\n", path);
            return false;
        }

        char ident[9];
        ident[8] = '\0';

        READ_OR_RETURN(&ident, 8);
        CMP_OR_RETURN(ident, RAW_VMAP_MAGIC);

        // We have to read one int. This is needed during the export, and we have to skip it here
        uint32 tempNVectors;
        READ_OR_RETURN(&tempNVectors, sizeof(tempNVectors));

        uint32 groups;
        READ_OR_RETURN(&groups, sizeof(uint32));
        READ_OR_RETURN(&RootWMOid, sizeof(uint32));

        groupsArray.resize(groups);
        bool succeed = true;
        for (uint32 g = 0; g < groups && succeed; ++g)
            succeed = groupsArray[g].Read(rf);

        if (succeed)
            fclose(rf);
        return succeed;
    }

// Drop of temporary use defines
#undef READ_OR_RETURN
#undef READ_OR_RETURN_WITH_DELETE
#undef CMP_OR_RETURN
}
