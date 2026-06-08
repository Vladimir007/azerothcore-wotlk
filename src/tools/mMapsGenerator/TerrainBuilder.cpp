#include "TerrainBuilder.h"

#include <vector>

#include "MapBuilder.h"
#include "MapDefines.h"
#include "MapTree.h"
#include "ModelInstance.h"
#include "PathCommon.h"
#include "StringFormat.h"
#include "VMapMgr.h"

namespace MMAP
{
    constexpr uint32 MAP_VERSION_MAGIC = 9;
    static uint16 holeTabH[4] = {0x1111, 0x2222, 0x4444, 0x8888};
    static uint16 holeTabV[4] = {0x000F, 0x00F0, 0x0F00, 0xF000};

    TerrainBuilder::TerrainBuilder(const Config* config): m_MapsPath(config->MapsPath()), m_vMapsPath(config->VMapsPath()) {}
    TerrainBuilder::~TerrainBuilder() = default;

    void TerrainBuilder::getLoopVars(const Spot portion, int& loopStart, int& loopEnd, int& loopInc)
    {
        switch (portion)
        {
            case ENTIRE:
                loopStart = 0;
                loopEnd = V8_SIZE_SQ;
                loopInc = 1;
                break;
            case TOP:
                loopStart = 0;
                loopEnd = V8_SIZE;
                loopInc = 1;
                break;
            case LEFT:
                loopStart = 0;
                loopEnd = V8_SIZE_SQ - V8_SIZE + 1;
                loopInc = V8_SIZE;
                break;
            case RIGHT:
                loopStart = V8_SIZE - 1;
                loopEnd = V8_SIZE_SQ;
                loopInc = V8_SIZE;
                break;
            case BOTTOM:
                loopStart = V8_SIZE_SQ - V8_SIZE;
                loopEnd = V8_SIZE_SQ;
                loopInc = 1;
                break;
        }
    }

    void TerrainBuilder::getHeightCoord(const int index, const Grid grid, const float xOffset, const float yOffset, float* coord, const float* v)
    {
        // Game coords: x, y, height.
        // Coord is mirrored about the horizontal axes
        switch (grid)
        {
        case GRID_V9:
            coord[0] = (xOffset + index % V9_SIZE * GRID_PART_SIZE) * -1.f;
            coord[1] = (yOffset + index / V9_SIZE * GRID_PART_SIZE) * -1.f;
            coord[2] = v[index];
            break;
        case GRID_V8:
            coord[0] = (xOffset + index % V8_SIZE * GRID_PART_SIZE + GRID_PART_SIZE / 2.f) * -1.f;
            coord[1] = (yOffset + index / V8_SIZE * GRID_PART_SIZE + GRID_PART_SIZE / 2.f) * -1.f;
            coord[2] = v[index];
            break;
        }
    }

    void TerrainBuilder::getLiquidCoord(const int index, const int index2, const float xOffset, const float yOffset, float* coord, const float* v)
    {
        // Game coords: x, y, height.
        // Coord is mirrored about the horizontal axes
        coord[0] = (xOffset + index % V9_SIZE * GRID_PART_SIZE) * -1.f;
        coord[1] = (yOffset + index / V9_SIZE * GRID_PART_SIZE) * -1.f;
        coord[2] = v[index2];
    }

    uint8 TerrainBuilder::getLiquidType(const int square, const uint8 liquid_type[16][16])
    {
        const int row = square / 128;
        const int col = square % 128;
        const int cellRow = row / 8;  // 8 squares per cell
        const int cellCol = col / 8;
        return liquid_type[cellRow][cellCol];
    }

    void TerrainBuilder::getHeightTriangle(const int square, const Spot triangle, int* indices, const bool liquid/* = false*/)
    {
        const int rowOffset = square / V8_SIZE;
        if (!liquid)
            switch (triangle)
            {
                case TOP:
                    indices[0] = square + rowOffset;                //           0-----1 .... 128
                    indices[1] = square + 1 + rowOffset;            //           |\ T /|
                    indices[2] = V9_SIZE_SQ + square;               //           | \ / |
                    break;                                          //           |L 0 R| ... 127
                case LEFT:                                          //           | / \ |
                    indices[0] = square + rowOffset;                //           |/ B \|
                    indices[1] = V9_SIZE_SQ + square;               //          129---130 ... 386
                    indices[2] = square + V9_SIZE + rowOffset;      //           |\   /|
                    break;                                          //           | \ / |
                case RIGHT:                                         //           | 128 | ... 255
                    indices[0] = square + 1 + rowOffset;            //           | / \ |
                    indices[1] = square + V9_SIZE + 1 + rowOffset;  //           |/   \|
                    indices[2] = V9_SIZE_SQ + square;               //          258---259 ... 515
                    break;
                case BOTTOM:
                    indices[0] = V9_SIZE_SQ + square;
                    indices[1] = square + V9_SIZE + 1 + rowOffset;
                    indices[2] = square + V9_SIZE + rowOffset;
                    break;
                default:
                    break;
            }
        else
            /*
                0-----1 .... 128
                |\    |
                | \ T |
                |  \  |
                | B \ |
                |    \|
                129---130 ... 386
                |\    |
                | \   |
                |  \  |
                |   \ |
                |    \|
                258---259 ... 515
            */
            switch (triangle)
            {
                case TOP:
                    indices[0] = square + rowOffset;
                    indices[1] = square + 1 + rowOffset;
                    indices[2] = square + V9_SIZE + 1 + rowOffset;
                    break;
                case BOTTOM:
                    indices[0] = square + rowOffset;
                    indices[1] = square + V9_SIZE + 1 + rowOffset;
                    indices[2] = square + V9_SIZE + rowOffset;
                    break;
                default:
                    break;
            }
    }

    bool TerrainBuilder::isHole(const int square, const uint16 holes[16][16])
    {
        const int row = square / 128;
        const int col = square % 128;
        const int cellRow = row / 8;  // 8 squares per cell
        const int cellCol = col / 8;
        const int holeRow = row % 8 / 2;
        const int holeCol = (square - (row * 128 + cellCol * 8)) / 2;

        const uint16 hole = holes[cellRow][cellCol];
        return (hole & holeTabH[holeCol] & holeTabV[holeRow]) != 0;
    }

    void TerrainBuilder::loadMap(const uint32 mapID, const uint32 tileX, const uint32 tileY, MeshData& meshData)
    {
        if (loadMap(mapID, tileX, tileY, meshData, ENTIRE))
        {
            loadMap(mapID, tileX + 1, tileY, meshData, LEFT);
            loadMap(mapID, tileX - 1, tileY, meshData, RIGHT);
            loadMap(mapID, tileX, tileY + 1, meshData, TOP);
            loadMap(mapID, tileX, tileY - 1, meshData, BOTTOM);
        }
    }

    bool TerrainBuilder::loadMap(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData, Spot portion)
    {
        fs::path mapFileName = m_MapsPath / std::format(MMAP_TILE_FILE_NAME_FORMAT, mapID, tileY, tileX);

        FILE* mapFile = fopen(mapFileName.c_str(), "rb");
        if (!mapFile)
            return false;

        MapFileHeader fHeader;
        if (fread(&fHeader, sizeof(MapFileHeader), 1, mapFile) != 1 ||
                fHeader.versionMagic != MAP_VERSION_MAGIC)
        {
            fclose(mapFile);
            printf("%s is the wrong version, please extract new .map files\n", mapFileName.c_str());
            return false;
        }

        MapHeightHeader hHeader;
        fseek(mapFile, fHeader.heightMapOffset, SEEK_SET);

        bool haveTerrain = false;
        bool haveLiquid = false;
        if (fread(&hHeader, sizeof(MapHeightHeader), 1, mapFile) == 1)
        {
            haveTerrain = !(hHeader.flags & MAP_HEIGHT_NO_HEIGHT);
            haveLiquid = fHeader.liquidMapOffset > 0;
        }

        // No data in this map file
        if (!haveTerrain && !haveLiquid)
        {
            fclose(mapFile);
            return false;
        }

        // Data used later
        uint16 holes[16][16] = {};
        uint16 liquid_entry[16][16] = {};
        uint8 liquid_flags[16][16] = {};
        G3D::Array<int> tTriangles;
        G3D::Array<int> lTriangles;

        // Terrain data
        if (haveTerrain)
        {
            float heightMultiplier;
            float V9[V9_SIZE_SQ], V8[V8_SIZE_SQ];
            int expected = V9_SIZE_SQ + V8_SIZE_SQ;

            if (hHeader.flags & MAP_HEIGHT_AS_INT8)
            {
                uint8 v9[V9_SIZE_SQ];
                uint8 v8[V8_SIZE_SQ];
                int count = 0;
                count += fread(v9, sizeof(uint8), V9_SIZE_SQ, mapFile);
                count += fread(v8, sizeof(uint8), V8_SIZE_SQ, mapFile);
                if (count != expected)
                    printf("TerrainBuilder::loadMap: Failed to read some data expected %d, read %d\n", expected, count);

                heightMultiplier = (hHeader.gridMaxHeight - hHeader.gridHeight) / 255;

                for (int i = 0; i < V9_SIZE_SQ; ++i)
                    V9[i] = static_cast<float>(v9[i]) * heightMultiplier + hHeader.gridHeight;

                for (int i = 0; i < V8_SIZE_SQ; ++i)
                    V8[i] = static_cast<float>(v8[i]) * heightMultiplier + hHeader.gridHeight;
            }
            else if (hHeader.flags & MAP_HEIGHT_AS_INT16)
            {
                uint16 v9[V9_SIZE_SQ];
                uint16 v8[V8_SIZE_SQ];
                int count = 0;
                count += fread(v9, sizeof(uint16), V9_SIZE_SQ, mapFile);
                count += fread(v8, sizeof(uint16), V8_SIZE_SQ, mapFile);
                if (count != expected)
                    printf("TerrainBuilder::loadMap: Failed to read some data expected %d, read %d\n", expected, count);

                heightMultiplier = (hHeader.gridMaxHeight - hHeader.gridHeight) / 65535;

                for (int i = 0; i < V9_SIZE_SQ; ++i)
                    V9[i] = static_cast<float>(v9[i]) * heightMultiplier + hHeader.gridHeight;

                for (int i = 0; i < V8_SIZE_SQ; ++i)
                    V8[i] = static_cast<float>(v8[i]) * heightMultiplier + hHeader.gridHeight;
            }
            else
            {
                int count = 0;
                count += fread(V9, sizeof(float), V9_SIZE_SQ, mapFile);
                count += fread(V8, sizeof(float), V8_SIZE_SQ, mapFile);
                if (count != expected)
                    printf("TerrainBuilder::loadMap: Failed to read some data expected %d, read %d\n", expected, count);
            }

            // Hole data
            if (fHeader.holesSize != 0)
            {
                fseek(mapFile, fHeader.holesOffset, SEEK_SET);
                if (fread(holes, fHeader.holesSize, 1, mapFile) != 1)
                    printf("TerrainBuilder::loadMap: Failed to read some data expected 1, read 0\n");
            }

            int count = meshData.solidVertices.size() / 3;
            float xOffset = (static_cast<float>(tileX) - 32) * GRID_SIZE;
            float yOffset = (static_cast<float>(tileY) - 32) * GRID_SIZE;

            float coord[3];

            for (int i = 0; i < V9_SIZE_SQ; ++i)
            {
                getHeightCoord(i, GRID_V9, xOffset, yOffset, coord, V9);
                meshData.solidVertices.append(coord[0]);
                meshData.solidVertices.append(coord[2]);
                meshData.solidVertices.append(coord[1]);
            }

            for (int i = 0; i < V8_SIZE_SQ; ++i)
            {
                getHeightCoord(i, GRID_V8, xOffset, yOffset, coord, V8);
                meshData.solidVertices.append(coord[0]);
                meshData.solidVertices.append(coord[2]);
                meshData.solidVertices.append(coord[1]);
            }

            int indices[] = { 0, 0, 0 };
            int loopStart = 0, loopEnd = 0, loopInc = 0;
            getLoopVars(portion, loopStart, loopEnd, loopInc);
            for (int i = loopStart; i < loopEnd; i += loopInc)
                for (int j = TOP; j <= BOTTOM; j += 1)
                {
                    getHeightTriangle(i, static_cast<Spot>(j), indices);
                    tTriangles.append(indices[2] + count);
                    tTriangles.append(indices[1] + count);
                    tTriangles.append(indices[0] + count);
                }
        }

        // Liquid data
        if (haveLiquid)
        {
            MapLiquidHeader lHeader;
            fseek(mapFile, fHeader.liquidMapOffset, SEEK_SET);
            if (fread(&lHeader, sizeof(MapLiquidHeader), 1, mapFile) != 1)
                printf("TerrainBuilder::loadMap: Failed to read some data expected 1, read 0\n");

            if (!(lHeader.flags & MAP_LIQUID_NO_TYPE))
            {
                if (fread(liquid_entry, sizeof(liquid_entry), 1, mapFile) != 1)
                    printf("TerrainBuilder::loadMap: Failed to read some data expected 1, read 0\n");
                if (fread(liquid_flags, sizeof(liquid_flags), 1, mapFile) != 1)
                    printf("TerrainBuilder::loadMap: Failed to read some data expected 1, read 0\n");
            }
            else
            {
                std::fill_n(&liquid_entry[0][0], 16 * 16, lHeader.liquidType);
                std::fill_n(&liquid_flags[0][0], 16 * 16, lHeader.liquidFlags);
            }

            int count = meshData.liquidVertices.size() / 3;
            const float xOffset = (static_cast<float>(tileX) - 32) * GRID_SIZE;
            const float yOffset = (static_cast<float>(tileY) - 32) * GRID_SIZE;

            float coord[3];
            int row, col;

            if (!(lHeader.flags & MAP_LIQUID_NO_HEIGHT))
            {
                uint32 toRead = lHeader.width * lHeader.height;
                auto liquid_map = new float[toRead];
                if (fread(liquid_map, sizeof(float), toRead, mapFile) != toRead)
                    printf("TerrainBuilder::loadMap: Failed to read some data expected 1, read 0\n");
                else
                {
                    int j = 0;
                    for (int i = 0; i < V9_SIZE_SQ; ++i)
                    {
                        row = i / V9_SIZE;
                        col = i % V9_SIZE;

                        if (row < lHeader.offsetY || row >= lHeader.offsetY + lHeader.height ||
                            col < lHeader.offsetX || col >= lHeader.offsetX + lHeader.width)
                        {
                            // Dummy vert using invalid height
                            meshData.liquidVertices.append(
                                (xOffset + col * GRID_PART_SIZE) * -1,
                                INVALID_MAP_LIQ_HEIGHT,
                                (yOffset + row * GRID_PART_SIZE) * -1);
                            continue;
                        }

                        getLiquidCoord(i, j, xOffset, yOffset, coord, liquid_map);
                        meshData.liquidVertices.append(coord[0]);
                        meshData.liquidVertices.append(coord[2]);
                        meshData.liquidVertices.append(coord[1]);
                        j++;
                    }
                }
                delete[] liquid_map;
            }
            else
            {
                for (int i = 0; i < V9_SIZE_SQ; ++i)
                {
                    row = i / V9_SIZE;
                    col = i % V9_SIZE;
                    meshData.liquidVertices.append(
                        (xOffset + col * GRID_PART_SIZE) * -1,
                        lHeader.liquidLevel,
                        (yOffset + row * GRID_PART_SIZE) * -1);
                }
            }


            int indices[] = { 0, 0, 0 };
            int loopStart = 0, loopEnd = 0, loopInc = 0, triInc = BOTTOM - TOP;
            getLoopVars(portion, loopStart, loopEnd, loopInc);

            // Generate triangles
            for (int i = loopStart; i < loopEnd; i += loopInc)
            {
                for (int j = TOP; j <= BOTTOM; j += triInc)
                {
                    getHeightTriangle(i, static_cast<Spot>(j), indices, true);
                    lTriangles.append(indices[2] + count);
                    lTriangles.append(indices[1] + count);
                    lTriangles.append(indices[0] + count);
                }
            }
        }

        fclose(mapFile);

        // Now that we have gathered the data, we can figure out which parts to keep:
        // liquid above ground, ground above liquid
        int loopStart = 0, loopEnd = 0, loopInc = 0;
        bool useTerrain, useLiquid;

        float* lVertices = meshData.liquidVertices.getCArray();
        int* lTris = lTriangles.getCArray();

        float* tVertices = meshData.solidVertices.getCArray();
        int* tTris = tTriangles.getCArray();

        if (lTriangles.size() + tTriangles.size() == 0)
            return false;

        // Make a copy of liquid vertices.
        // Used to pad right-bottom frame due to lost vertex data at extraction
        float* lVerticesCopy = nullptr;
        if (meshData.liquidVertices.size())
        {
            lVerticesCopy = new float[meshData.liquidVertices.size()];
            memcpy(lVerticesCopy, lVertices, sizeof(float) * meshData.liquidVertices.size());
        }

        getLoopVars(portion, loopStart, loopEnd, loopInc);
        for (int i = loopStart; i < loopEnd; i += loopInc)
        {
            for (int j = 0; j < 2; ++j)
            {
                // Default is true, will change to false if needed
                useTerrain = true;
                useLiquid = true;
                uint8 liquidType = MAP_LIQUID_TYPE_NO_WATER;

                // If there is no liquid, don't use liquid
                if (!meshData.liquidVertices.size() || !lTriangles.size())
                    useLiquid = false;
                else
                {
                    liquidType = getLiquidType(i, liquid_flags);
                    switch (liquidType)
                    {
                    case MAP_LIQUID_TYPE_WATER:
                    case MAP_LIQUID_TYPE_OCEAN:
                        // Merge different types of water
                        liquidType = NAV_WATER;
                        break;
                    case MAP_LIQUID_TYPE_MAGMA:
                        liquidType = NAV_MAGMA;
                        break;
                    case MAP_LIQUID_TYPE_SLIME:
                        liquidType = NAV_SLIME;
                        break;
                    case MAP_LIQUID_TYPE_DARK_WATER:
                        // Players should not be here, so logically neither should creatures
                        useTerrain = false;
                        useLiquid = false;
                        break;
                    default:
                        useLiquid = false;
                        break;
                    }
                }

                // If there is no terrain, don't use terrain
                if (!tTriangles.size())
                    useTerrain = false;

                // While extracting ADT data we are losing right-bottom vertices.
                // This code adds fair approximation of lost data.
                if (useLiquid && lVerticesCopy != nullptr)
                {
                    float quadHeight = 0;
                    uint32 validCount = 0;
                    for (uint32 idx = 0; idx < 3; idx++)
                    {
                        if (float h = lVerticesCopy[lTris[idx] * 3 + 1]; h != INVALID_MAP_LIQ_HEIGHT && h < INVALID_MAP_LIQ_HEIGHT_MAX)
                        {
                            quadHeight += h;
                            validCount++;
                        }
                    }

                    // Update vertex height data
                    if (validCount > 0 && validCount < 3)
                    {
                        quadHeight /= validCount;
                        for (uint32 idx = 0; idx < 3; idx++)
                        {
                            if (float h = lVertices[lTris[idx] * 3 + 1]; h == INVALID_MAP_LIQ_HEIGHT || h > INVALID_MAP_LIQ_HEIGHT_MAX)
                                lVertices[lTris[idx] * 3 + 1] = quadHeight;
                        }
                    }

                    // No valid vertexes - don't use this poly at all
                    if (validCount == 0)
                        useLiquid = false;
                }

                // If there is a hole here, don't use the terrain
                if (useTerrain && fHeader.holesSize != 0)
                    useTerrain = !isHole(i, holes);

                // We use only one terrain kind per quad - pick higher one
                if (useTerrain && useLiquid)
                {
                    float minLLevel = INVALID_MAP_LIQ_HEIGHT_MAX;
                    float maxLLevel = INVALID_MAP_LIQ_HEIGHT;
                    for (uint32 x = 0; x < 3; x++)
                    {
                        float h = lVertices[lTris[x] * 3 + 1];
                        if (minLLevel > h)
                            minLLevel = h;

                        if (maxLLevel < h)
                            maxLLevel = h;
                    }

                    float maxTLevel = INVALID_MAP_LIQ_HEIGHT;
                    float minTLevel = INVALID_MAP_LIQ_HEIGHT_MAX;
                    for (uint32 x = 0; x < 6; x++)
                    {
                        float h = tVertices[tTris[x] * 3 + 1];
                        if (maxTLevel < h)
                            maxTLevel = h;

                        if (minTLevel > h)
                            minTLevel = h;
                    }

                    // Terrain under the liquid?
                    if (minLLevel > maxTLevel)
                        useTerrain = false;

                    // Liquid under the terrain?
                    if (minTLevel > maxLLevel)
                        useLiquid = false;
                }

                // Store the result
                if (useLiquid)
                {
                    meshData.liquidType.append(liquidType);
                    for (int k = 0; k < 3; ++k)
                        meshData.liquidTris.append(lTris[k]);
                }

                if (useTerrain)
                    for (int k = 0; k < 6; ++k)
                        meshData.solidTris.append(tTris[k]);

                // Advance to next set of triangles
                lTris += 3;
                tTris += 6;
            }
        }

        if (lVerticesCopy)
            delete [] lVerticesCopy;

        return meshData.solidTris.size() || meshData.liquidTris.size();
    }

    bool TerrainBuilder::loadVMap(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData)
    {
        std::string const mapFileName = VMapMgr::getMapFileName(mapID);
        auto staticTree = std::make_unique<StaticMapTree>(mapID, m_vMapsPath);
        if (!staticTree->InitMap(mapFileName))
            return false;

        staticTree->LoadMapTile(tileX, tileY);

        ModelInstance* models = nullptr;
        uint32 count = 0;
        staticTree->GetModelInstances(models, count);

        if (!models)
            return false;

        bool retval = false;
        for (uint32 i = 0; i < count; ++i)
        {
            ModelInstance instance = models[i];

            // Model instances exist in tree even though there are instances of that model in this tile
            WorldModel* worldModel = instance.getWorldModel();
            if (!worldModel)
                continue;

            // Now we have a model to add to the mesh data
            retval = true;

            std::vector<GroupModel> groupModels;
            worldModel->GetGroupModels(groupModels);

            // All M2s need to have triangle indices reversed
            bool isM2 = instance.name.find(".m2") != std::string::npos || instance.name.find(".M2") != std::string::npos;

            // transform data
            float scale = instance.iScale;
            G3D::Matrix3 rotation = G3D::Matrix3::fromEulerAnglesXYZ(G3D::pi() * instance.iRot.z / -180.f, G3D::pi() * instance.iRot.x / -180.f, G3D::pi() * instance.iRot.y / -180.f);
            G3D::Vector3 position = instance.iPos;
            position.x -= 32 * GRID_SIZE;
            position.y -= 32 * GRID_SIZE;

            for (auto & groupModel : groupModels)
            {
                std::vector<G3D::Vector3> tempVertices;
                std::vector<G3D::Vector3> transformedVertices;
                std::vector<MeshTriangle> tempTriangles;
                WmoLiquid* liquid = nullptr;

                groupModel.GetMeshData(tempVertices, tempTriangles, liquid);

                // First handle collision mesh
                transform(tempVertices, transformedVertices, scale, rotation, position);

                int offset = meshData.solidVertices.size() / 3;

                copyVertices(transformedVertices, meshData.solidVertices);
                copyIndices(tempTriangles, meshData.solidTris, offset, isM2);

                // Now handle liquid data
                if (liquid && liquid->GetFlagsStorage())
                {
                    std::vector<G3D::Vector3> liqVertices;
                    std::vector<int> liqTris;
                    uint32 tilesX, tilesY, verticesX, verticesY;
                    G3D::Vector3 corner;
                    liquid->GetPosInfo(tilesX, tilesY, corner);
                    verticesX = tilesX + 1;
                    verticesY = tilesY + 1;
                    uint8* flags = liquid->GetFlagsStorage();
                    float* data = liquid->GetHeightStorage();
                    uint8 type = NAV_EMPTY;

                    switch (liquid->GetType() & 3)
                    {
                    case 0:
                    case 1:
                        type = NAV_WATER;
                        break;
                    case 2:
                        type = NAV_MAGMA;
                        break;
                    case 3:
                        type = NAV_SLIME;
                        break;
                    default:
                        break;
                    }

                    G3D::Vector3 vert;
                    for (uint32 x = 0; x < verticesX; ++x)
                    {
                        for (uint32 y = 0; y < verticesY; ++y)
                        {
                            vert = G3D::Vector3(corner.x + x * GRID_PART_SIZE, corner.y + y * GRID_PART_SIZE, data[y * verticesX + x]);
                            vert = vert * rotation * scale + position;
                            vert.x *= -1.f;
                            vert.y *= -1.f;
                            liqVertices.push_back(vert);
                        }
                    }

                    int idx1, idx2, idx3, idx4;
                    uint32 square;
                    for (uint32 x = 0; x < tilesX; ++x)
                    {
                        for (uint32 y = 0; y < tilesY; ++y)
                        {
                            if ((flags[x + y * tilesX] & 0x0f) != 0x0f)
                            {
                                square = x * tilesY + y;
                                idx1 = square + x;
                                idx2 = square + 1 + x;
                                idx3 = square + tilesY + 1 + 1 + x;
                                idx4 = square + tilesY + 1 + x;

                                // Top triangle
                                liqTris.push_back(idx3);
                                liqTris.push_back(idx2);
                                liqTris.push_back(idx1);

                                // Bottom triangle
                                liqTris.push_back(idx4);
                                liqTris.push_back(idx3);
                                liqTris.push_back(idx1);
                            }
                        }
                    }

                    uint32 liqOffset = meshData.liquidVertices.size() / 3;
                    for (auto & liqVert : liqVertices)
                        meshData.liquidVertices.append(liqVert.y, liqVert.z, liqVert.x);

                    for (uint32 j = 0; j < liqTris.size() / 3; ++j)
                    {
                        meshData.liquidTris.append(liqTris[j * 3 + 1] + liqOffset, liqTris[j * 3 + 2] + liqOffset, liqTris[j * 3] + liqOffset);
                        meshData.liquidType.append(type);
                    }
                }
            }
        }

        return retval;
    }

    void TerrainBuilder::transform(const std::vector<G3D::Vector3>& original, std::vector<G3D::Vector3>& transformedVertices,
        const float scale, const G3D::Matrix3& rotation, const G3D::Vector3& position)
    {
        for (auto & it : original)
        {
            // Apply transform, then mirror along the horizontal axes
            G3D::Vector3 v(it * rotation * scale + position);
            v.x *= -1.f;
            v.y *= -1.f;
            transformedVertices.push_back(v);
        }
    }

    void TerrainBuilder::copyVertices(const std::vector<G3D::Vector3>& source, G3D::Array<float>& dest)
    {
        for (auto & it : source)
        {
            dest.push_back(it.y);
            dest.push_back(it.z);
            dest.push_back(it.x);
        }
    }

    void TerrainBuilder::copyIndices(const std::vector<MeshTriangle>& source, G3D::Array<int>& dest, const int offset, const bool flip)
    {
        if (flip)
        {
            for (auto & it : source)
            {
                dest.push_back(it.idx2 + offset);
                dest.push_back(it.idx1 + offset);
                dest.push_back(it.idx0 + offset);
            }
        }
        else
        {
            for (auto & it : source)
            {
                dest.push_back(it.idx0 + offset);
                dest.push_back(it.idx1 + offset);
                dest.push_back(it.idx2 + offset);
            }
        }
    }

    void TerrainBuilder::copyIndices(G3D::Array<int>& source, G3D::Array<int>& dest, const int offset)
    {
        const int* src = source.getCArray();
        for (int32 i = 0; i < source.size(); ++i)
            dest.append(src[i] + offset);
    }
}
