#include "MapExport.h"

#include <iostream>
#include "adt.h"
#include "MapDefines.h"

DBCStorage<MapEntry> sMapStore;
DBCStorage<LiquidEntry> sLiquidStore;

// Temporary grid data store
uint16 area_ids[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

float V8[ADT_GRID_SIZE][ADT_GRID_SIZE];
float V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];
uint16 uint16_V8[ADT_GRID_SIZE][ADT_GRID_SIZE];
uint16 uint16_V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];
uint8 uint8_V8[ADT_GRID_SIZE][ADT_GRID_SIZE];
uint8 uint8_V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];

uint16 liquid_entry[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];
uint8 liquid_flags[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];
bool liquid_show[ADT_GRID_SIZE][ADT_GRID_SIZE];
float liquid_height[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];
uint16 holes[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

int16 flight_box_max[3][3];
int16 flight_box_min[3][3];

void Abort(const std::string& message)
{
    std::cerr << message << std::endl;
    CloseMPQFiles();
    exit(1);
}

void CreateDir(const fs::path& dir)
{
    if (fs::is_directory(dir))
        return;
    if (fs::create_directories(dir))
        return;
    Abort(std::format("Fatal Error: Could not create directory '{}'. Check your permissions.", dir.string()));
}

uint32 ReadBuild(const std::string& locale)
{
    // Include build info file also
    const std::string filename = std::format("component.wow-{}.txt", locale);

    MPQFile m(filename.c_str());
    if (m.isEof())
        Abort(std::format("Fatal error: build file not found: {}!", filename));

    auto text = std::string(m.getPointer(), m.getSize());
    m.close();

    const std::size_t pos = text.find("version=\"");
    const std::size_t pos1 = pos + strlen("version=\"");
    const std::size_t pos2 = text.find("\"", pos1);
    if (pos == text.npos || pos2 == text.npos || pos1 >= pos2)
        Abort(std::format("Fatal error: invalid file '{}' format!", filename));

    const std::string buildStr = text.substr(pos1, pos2 - pos1);

    const int build = atoi(buildStr.c_str());
    if (build <= 0)
        Abort(std::format("Fatal error: invalid file '{}' format!", filename));

    return build;
}

bool ConvertADT(const std::string& inputPath, const std::string& outputPath, uint32 build, const MapEntry* mapEntry)
{
    FileADT adt;

    if (!adt.loadFile(inputPath))
        return false;

    MCIN* cells = adt.grid->getMCIN();
    if (!cells)
    {
        printf("Can't find cells in '%s'\n", inputPath.c_str());
        return false;
    }

    memset(liquid_show, 0, sizeof(liquid_show));
    memset(liquid_flags, 0, sizeof(liquid_flags));
    memset(liquid_entry, 0, sizeof(liquid_entry));
    memset(holes, 0, sizeof(holes));

    // Prepare map header
    MapFileHeader map;
    map.mapMagic = MapMagic.asUInt;
    map.versionMagic = MapVersionMagic;
    map.buildMagic = build;

    // Get area flags data
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
            area_ids[i][j] = cells->getMCNK(i, j)->areaID;

    // Try pack area data
    bool fullAreaData = false;
    uint32 areaID = area_ids[0][0];
    for (auto & area_id : area_ids)
    {
        for (int x = 0; x < ADT_CELLS_PER_GRID; ++x)
        {
            if (area_id[x] != areaID)
            {
                fullAreaData = true;
                break;
            }
        }
    }

    map.areaMapOffset = sizeof(map);
    map.areaMapSize   = sizeof(MapAreaHeader);

    MapAreaHeader areaHeader;
    areaHeader.fourcc = MapAreaMagic.asUInt;
    areaHeader.flags = 0;
    if (fullAreaData)
    {
        areaHeader.gridArea = 0;
        map.areaMapSize += sizeof(area_ids);
    }
    else
    {
        areaHeader.flags |= MAP_AREA_NO_AREA;
        areaHeader.gridArea = static_cast<uint16>(areaID);
    }

    // Get Height map from grid
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            MCNK* cell = cells->getMCNK(i, j);
            if (!cell)
                continue;
            // Height values for triangles stored in order:
            // 1     2     3     4     5     6     7     8     9
            //    10    11    12    13    14    15    16    17
            // 18    19    20    21    22    23    24    25    26
            //    27    28    29    30    31    32    33    34
            // . . . . . . . .
            // For better get height values merge it to V9 and V8 map
            // V9 height map:
            // 1     2     3     4     5     6     7     8     9
            // 18    19    20    21    22    23    24    25    26
            // . . . . . . . .
            // V8 height map:
            //    10    11    12    13    14    15    16    17
            //    27    28    29    30    31    32    33    34
            // . . . . . . . .

            // Set map height as grid height
            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V9[cy][cx] = cell->ypos;
                }
            }
            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V8[cy][cx] = cell->ypos;
                }
            }

            // Get custom height
            MCVT* v = cell->getMCVT();
            if (!v)
                continue;

            // Get V9 height map
            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V9[cy][cx] += v->heightMap[y * (ADT_CELL_SIZE * 2 + 1) + x];
                }
            }

            // Get V8 height map
            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V8[cy][cx] += v->heightMap[y * (ADT_CELL_SIZE * 2 + 1) + ADT_CELL_SIZE + 1 + x];
                }
            }
        }
    }

    // Try pack height data
    float maxHeight = -20000;
    float minHeight =  20000;
    for (auto & y : V8)
    {
        for (int x = 0; x < ADT_GRID_SIZE; x++)
        {
            float h = y[x];
            if (maxHeight < h) maxHeight = h;
            if (minHeight > h) minHeight = h;
        }
    }
    for (int y = 0; y <= ADT_GRID_SIZE; y++)
    {
        for (int x = 0; x <= ADT_GRID_SIZE; x++)
        {
            float h = V9[y][x];
            if (maxHeight < h) maxHeight = h;
            if (minHeight > h) minHeight = h;
        }
    }

    // Check for allow limit minimum height (not store height in deep ocean - allow save some memory)
    if (minHeight < EXTRACT_MIN_HEIGHT)
    {
        for (auto & y : V8)
            for (int x = 0; x < ADT_GRID_SIZE; x++)
                if (y[x] < EXTRACT_MIN_HEIGHT)
                    y[x] = EXTRACT_MIN_HEIGHT;
        for (int y = 0; y <= ADT_GRID_SIZE; y++)
            for (int x = 0; x <= ADT_GRID_SIZE; x++)
                if (V9[y][x] < EXTRACT_MIN_HEIGHT)
                    V9[y][x] = EXTRACT_MIN_HEIGHT;
        if (minHeight < EXTRACT_MIN_HEIGHT)
            minHeight = EXTRACT_MIN_HEIGHT;
        if (maxHeight < EXTRACT_MIN_HEIGHT)
            maxHeight = EXTRACT_MIN_HEIGHT;
    }

    bool hasFlightBox = false;
    if (MFBO* mfbo = adt.grid->getMFBO())
    {
        memcpy(flight_box_max, &mfbo->max, sizeof(flight_box_max));
        memcpy(flight_box_min, &mfbo->min, sizeof(flight_box_min));
        hasFlightBox = true;
    }

    map.heightMapOffset = map.areaMapOffset + map.areaMapSize;
    map.heightMapSize = sizeof(MapHeightHeader);

    MapHeightHeader heightHeader;
    heightHeader.fourcc = MapHeightMagic.asUInt;
    heightHeader.flags = 0;
    heightHeader.gridHeight = minHeight;
    heightHeader.gridMaxHeight = maxHeight;

    if (maxHeight == minHeight)
        heightHeader.flags |= MAP_HEIGHT_NO_HEIGHT;

    // Not need store if flat surface
    if (maxHeight - minHeight < EXTRACT_FLAT_HEIGHT_DELTA_LIMIT)
        heightHeader.flags |= MAP_HEIGHT_NO_HEIGHT;

    if (hasFlightBox)
    {
        heightHeader.flags |= MAP_HEIGHT_HAS_FLIGHT_BOUNDS;
        map.heightMapSize += sizeof(flight_box_max) + sizeof(flight_box_min);
    }

    // Try store as packed in uint16 or uint8 values
    if (!(heightHeader.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        float step = 0;

        // Try Store as uint values
        float diff = maxHeight - minHeight;
        if (diff < EXTRACT_FLOAT_TO_INT8_LIMIT)
        {
            heightHeader.flags |= MAP_HEIGHT_AS_INT8;
            step = 255 / diff;
        }
        else if (diff < EXTRACT_FLOAT_TO_INT16_LIMIT)
        {
            heightHeader.flags |= MAP_HEIGHT_AS_INT16;
            step = 65535 / diff;
        }

        // Pack it to int values, if needed
        if (heightHeader.flags & MAP_HEIGHT_AS_INT8)
        {
            for (int y = 0; y < ADT_GRID_SIZE; y++)
                for (int x = 0; x < ADT_GRID_SIZE; x++)
                    uint8_V8[y][x] = static_cast<uint8>((V8[y][x] - minHeight) * step + 0.5f);
            for (int y = 0; y <= ADT_GRID_SIZE; y++)
                for (int x = 0; x <= ADT_GRID_SIZE; x++)
                    uint8_V9[y][x] = static_cast<uint8>((V9[y][x] - minHeight) * step + 0.5f);
            map.heightMapSize += sizeof(uint8_V9) + sizeof(uint8_V8);
        }
        else if (heightHeader.flags & MAP_HEIGHT_AS_INT16)
        {
            for (int y = 0; y < ADT_GRID_SIZE; y++)
                for (int x = 0; x < ADT_GRID_SIZE; x++)
                    uint16_V8[y][x] = static_cast<uint16>((V8[y][x] - minHeight) * step + 0.5f);
            for (int y = 0; y <= ADT_GRID_SIZE; y++)
                for (int x = 0; x <= ADT_GRID_SIZE; x++)
                    uint16_V9[y][x] = static_cast<uint16>((V9[y][x] - minHeight) * step + 0.5f);
            map.heightMapSize += sizeof(uint16_V9) + sizeof(uint16_V8);
        }
        else
            map.heightMapSize += sizeof(V9) + sizeof(V8);
    }

    // Get from MCLQ chunk (old)
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            MCNK* cell = cells->getMCNK(i, j);
            if (!cell)
                continue;

            MCLQ* liquid = cell->getMCLQ();
            int count = 0;
            if (!liquid || cell->sizeMCLQ <= 8)
                continue;

            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    if (liquid->flags[y][x] != 0x0F)
                    {
                        liquid_show[cy][cx] = true;
                        if (liquid->flags[y][x] & (1 << 7))
                            liquid_flags[i][j] |= MAP_LIQUID_TYPE_DARK_WATER;
                        ++count;
                    }
                }
            }

            uint32 c_flag = cell->flags;
            if (c_flag & (1 << 2))
            {
                liquid_entry[i][j] = 1;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_WATER;
            }
            if (c_flag & (1 << 3))
            {
                liquid_entry[i][j] = 2;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_OCEAN;
            }
            if (c_flag & (1 << 4))
            {
                liquid_entry[i][j] = 3;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_MAGMA;  // Magma / Slime
            }

            if (!count && liquid_flags[i][j])
                fprintf(stderr, "Wrong liquid detect in MCLQ chunk");

            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    liquid_height[cy][cx] = liquid->liquid[y][x].height;
                }
            }
        }
    }

    // Get liquid map for grid (in WotLK used MH2O chunk)
    if (MH2O* h2o = adt.grid->getMH2O())
    {
        for (int32 i = 0; i < ADT_CELLS_PER_GRID; i++)
        {
            for (int32 j = 0; j < ADT_CELLS_PER_GRID; j++)
            {
                LiquidInstance const* h = h2o->GetLiquidInstance(i,j);
                if (!h)
                    continue;

                int32 count = 0;
                uint64 existsMask = h2o->GetLiquidExistsBitmap(h);
                for (int32 y = 0; y < h->GetHeight(); y++)
                {
                    int32 cy = i * ADT_CELL_SIZE + y + h->GetOffsetY();
                    for (int32 x = 0; x < h->GetWidth(); x++)
                    {
                        int32 cx = j * ADT_CELL_SIZE + x + h->GetOffsetX();
                        if (existsMask & 1)
                        {
                            liquid_show[cy][cx] = true;
                            ++count;
                        }
                        existsMask >>= 1;
                    }
                }

                liquid_entry[i][j] = h->LiquidType;
                if (const auto entry = sLiquidStore.LookupEntry(h->LiquidType); entry)
                {
                    switch (entry->Type)
                    {
                    case LIQUID_TYPE_WATER:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_WATER;
                        break;
                    case LIQUID_TYPE_OCEAN:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_OCEAN;
                        if (auto [Fishable, Deep] = h2o->GetLiquidAttributes(i, j); Deep)
                            liquid_flags[i][j] |= MAP_LIQUID_TYPE_DARK_WATER;
                        break;
                    case LIQUID_TYPE_MAGMA:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_MAGMA;
                        break;
                    case LIQUID_TYPE_SLIME:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_SLIME;
                        break;
                    default:
                        std::cerr << std::format("Invalid Liquid type {} for '{}' (chunk {}, {})", h->LiquidType, mapEntry->Name, i, j) << std::endl;
                        break;
                    }
                }
                else
                    std::cerr << std::format("Invalid Liquid type {} for '{}' (chunk {}, {})", h->LiquidType, mapEntry->Name, i, j) << std::endl;


                if (!count && liquid_flags[i][j])
                    std::cerr << "Wrong liquid detect in MH2O chunk" << std::endl;

                int32 pos = 0;
                for (int32 y = 0; y <= h->GetHeight(); y++)
                {
                    int cy = i * ADT_CELL_SIZE + y + h->GetOffsetY();
                    for (int32 x = 0; x <= h->GetWidth(); x++)
                    {
                        int32 cx = j * ADT_CELL_SIZE + x + h->GetOffsetX();
                        liquid_height[cy][cx] = h2o->GetLiquidHeight(h, pos);

                        pos++;
                    }
                }
            }
        }
    }

    // Pack liquid data
    uint16 firstLiquidType = liquid_entry[0][0];
    uint8 firstLiquidFlag = liquid_flags[0][0];
    bool fullType = false;
    for (int y = 0; y < ADT_CELLS_PER_GRID; y++)
    {
        for (int x = 0; x < ADT_CELLS_PER_GRID; x++)
        {
            if (liquid_entry[y][x] != firstLiquidType || liquid_flags[y][x] != firstLiquidFlag)
            {
                fullType = true;
                y = ADT_CELLS_PER_GRID;
                break;
            }
        }
    }

    MapLiquidHeader liquidHeader;

    // No water data (if all grid have 0 liquid type)
    if (firstLiquidFlag == 0 && !fullType)
    {
        // No liquid data
        map.liquidMapOffset = 0;
        map.liquidMapSize   = 0;
    }
    else
    {
        int minX = 255, minY = 255;
        int maxX = 0, maxY = 0;
        maxHeight = -20000;
        minHeight = 20000;
        for (int y = 0; y < ADT_GRID_SIZE; y++)
        {
            for (int x = 0; x < ADT_GRID_SIZE; x++)
            {
                if (liquid_show[y][x])
                {
                    if (minX > x) minX = x;
                    if (maxX < x) maxX = x;
                    if (minY > y) minY = y;
                    if (maxY < y) maxY = y;
                    float h = liquid_height[y][x];
                    if (maxHeight < h) maxHeight = h;
                    if (minHeight > h) minHeight = h;
                }
                else
                {
                    liquid_height[y][x] = EXTRACT_MIN_HEIGHT;
                    if (minHeight > EXTRACT_MIN_HEIGHT)
                        minHeight = EXTRACT_MIN_HEIGHT;
                }
            }
        }
        map.liquidMapOffset = map.heightMapOffset + map.heightMapSize;
        map.liquidMapSize = sizeof(MapLiquidHeader);
        liquidHeader.fourcc = MapLiquidMagic.asUInt;
        liquidHeader.flags = 0;
        liquidHeader.liquidType = 0;
        liquidHeader.offsetX = minX;
        liquidHeader.offsetY = minY;
        liquidHeader.width   = maxX - minX + 1 + 1;
        liquidHeader.height  = maxY - minY + 1 + 1;
        liquidHeader.liquidLevel = minHeight;

        if (maxHeight == minHeight)
            liquidHeader.flags |= MAP_LIQUID_NO_HEIGHT;

        // No need to store if flat surface
        if (maxHeight - minHeight < EXTRACT_FLAT_LIQUID_DELTA_LIMIT)
            liquidHeader.flags |= MAP_LIQUID_NO_HEIGHT;

        if (!fullType)
            liquidHeader.flags |= MAP_LIQUID_NO_TYPE;

        if (liquidHeader.flags & MAP_LIQUID_NO_TYPE)
        {
            liquidHeader.liquidFlags = firstLiquidFlag;
            liquidHeader.liquidType = firstLiquidType;
        }
        else
            map.liquidMapSize += sizeof(liquid_entry) + sizeof(liquid_flags);

        if (!(liquidHeader.flags & MAP_LIQUID_NO_HEIGHT))
            map.liquidMapSize += sizeof(float) * liquidHeader.width * liquidHeader.height;
    }

    bool hasHoles = false;

    for (int i = 0; i < ADT_CELLS_PER_GRID; ++i)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; ++j)
        {
            MCNK* cell = cells->getMCNK(i, j);
            if (!cell)
                continue;
            holes[i][j] = cell->holes;
            if (!hasHoles && cell->holes != 0)
                hasHoles = true;
        }
    }

    if (hasHoles)
    {
        if (map.liquidMapOffset)
            map.holesOffset = map.liquidMapOffset + map.liquidMapSize;
        else
            map.holesOffset = map.heightMapOffset + map.heightMapSize;

        map.holesSize = sizeof(holes);
    }
    else
    {
        map.holesOffset = 0;
        map.holesSize = 0;
    }

    // All data prepared - store it
    FILE* output = fopen(outputPath.c_str(), "wb");
    if (!output)
    {
        std::cerr << "Can't create the output file: " << outputPath << std::endl;
        return false;
    }
    fwrite(&map, sizeof(map), 1, output);

    // Store area data
    fwrite(&areaHeader, sizeof(areaHeader), 1, output);
    if (!(areaHeader.flags & MAP_AREA_NO_AREA))
        fwrite(area_ids, sizeof(area_ids), 1, output);

    // Store height data
    fwrite(&heightHeader, sizeof(heightHeader), 1, output);
    if (!(heightHeader.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        if (heightHeader.flags & MAP_HEIGHT_AS_INT16)
        {
            fwrite(uint16_V9, sizeof(uint16_V9), 1, output);
            fwrite(uint16_V8, sizeof(uint16_V8), 1, output);
        }
        else if (heightHeader.flags & MAP_HEIGHT_AS_INT8)
        {
            fwrite(uint8_V9, sizeof(uint8_V9), 1, output);
            fwrite(uint8_V8, sizeof(uint8_V8), 1, output);
        }
        else
        {
            fwrite(V9, sizeof(V9), 1, output);
            fwrite(V8, sizeof(V8), 1, output);
        }
    }

    if (heightHeader.flags & MAP_HEIGHT_HAS_FLIGHT_BOUNDS)
    {
        fwrite(flight_box_max, sizeof(flight_box_max), 1, output);
        fwrite(flight_box_min, sizeof(flight_box_min), 1, output);
    }

    // Store liquid data
    if (map.liquidMapOffset)
    {
        fwrite(&liquidHeader, sizeof(liquidHeader), 1, output);
        if (!(liquidHeader.flags & MAP_LIQUID_NO_TYPE))
        {
            fwrite(liquid_entry, sizeof(liquid_entry), 1, output);
            fwrite(liquid_flags, sizeof(liquid_flags), 1, output);
        }
        if (!(liquidHeader.flags & MAP_LIQUID_NO_HEIGHT))
        {
            for (int y = 0; y < liquidHeader.height; y++)
                fwrite(&liquid_height[y + liquidHeader.offsetY][liquidHeader.offsetX], sizeof(float), liquidHeader.width, output);
        }
    }

    // Store hole data
    if (hasHoles)
        fwrite(holes, map.holesSize, 1, output);

    fclose(output);

    return true;
}
