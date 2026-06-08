#ifndef MMAP_TERRAIN_BUILDER_H
#define MMAP_TERRAIN_BUILDER_H

#include <filesystem>
#include <G3D/Array.h>
#include <G3D/Vector3.h>

#include "Config.h"
#include "WorldModel.h"

namespace fs = std::filesystem;

namespace MMAP
{
    enum Spot
    {
        TOP    = 1,
        RIGHT  = 2,
        LEFT   = 3,
        BOTTOM = 4,
        ENTIRE = 5
    };

    enum Grid
    {
        GRID_V8,
        GRID_V9
    };

    static constexpr int V9_SIZE = 129;
    static constexpr int V9_SIZE_SQ = V9_SIZE * V9_SIZE;
    static constexpr int V8_SIZE = 128;
    static constexpr int V8_SIZE_SQ = V8_SIZE * V8_SIZE;
    static constexpr float GRID_SIZE = 533.3333f;
    static constexpr float GRID_PART_SIZE = GRID_SIZE / V8_SIZE;
    static constexpr float INVALID_MAP_LIQ_HEIGHT = -500.f;
    static constexpr float INVALID_MAP_LIQ_HEIGHT_MAX = 5000.0f;

    struct MeshData
    {
        G3D::Array<float> solidVertices;
        G3D::Array<int> solidTris;
        G3D::Array<float> liquidVertices;
        G3D::Array<int> liquidTris;
        G3D::Array<uint8> liquidType;
    };

    class TerrainBuilder
    {
    public:
        explicit TerrainBuilder(const Config* config);
        TerrainBuilder(const TerrainBuilder& tb) = delete;
        ~TerrainBuilder();

        void loadMap(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData);
        bool loadVMap(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData);

        static void transform(const std::vector<G3D::Vector3>& original, std::vector<G3D::Vector3>& transformedVertices,
                              float scale, const G3D::Matrix3& rotation, const G3D::Vector3& position);
        static void copyVertices(const std::vector<G3D::Vector3>& source, G3D::Array<float>& dest);
        static void copyIndices(const std::vector<VMAP::MeshTriangle>& source, G3D::Array<int>& dest, int offset, bool flip);
        static void copyIndices(G3D::Array<int>& source, G3D::Array<int>& dest, int offset);
    private:
        static void getLoopVars(Spot portion, int& loopStart, int& loopEnd, int& loopInc);
        static void getHeightCoord(int index, Grid grid, float xOffset, float yOffset, float* coord, const float* v);
        static void getLiquidCoord(int index, int index2, float xOffset, float yOffset, float* coord, const float* v);
        static uint8 getLiquidType(int square, const uint8 liquid_type[16][16]);
        static void getHeightTriangle(int square, Spot triangle, int* indices, bool liquid = false);
        static bool isHole(int square, const uint16 holes[16][16]);

        bool loadMap(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData, Spot portion);


        fs::path m_MapsPath;
        fs::path m_vMapsPath;
    };
}

#endif
