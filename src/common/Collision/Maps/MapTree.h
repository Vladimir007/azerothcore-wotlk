#ifndef MAP_TREE_H
#define MAP_TREE_H

#include <unordered_map>
#include "BoundingIntervalHierarchy.h"
#include "Define.h"

namespace VMAP
{
    class ModelInstance;
    class GroupModel;
    class VMapMgr;
    enum class ModelIgnoreFlags : uint32;
    enum class LoadResult : uint8;

    struct GroupLocationInfo
    {
        const GroupModel* hitModel = nullptr;
        int32 rootId = -1;
    };

    struct LocationInfo
    {
        LocationInfo():  ground_Z(-G3D::inf()) { }
        const ModelInstance* hitInstance{nullptr};
        const GroupModel* hitModel{nullptr};
        float ground_Z;
        int32 rootId = -1;
    };

    class StaticMapTree
    {
        typedef std::unordered_map<uint32, bool> loadedTileMap;
        typedef std::unordered_map<uint32, uint32> loadedSpawnMap;

        uint32 iMapID;
        bool iIsTiled;
        BIH iTree;
        ModelInstance* iTreeValues;
        uint32 iNTreeValues;

        // Store all the map tile idents that are loaded for that map.
        // Some maps are not split into tiles, and we have to make sure, not removing the map before all tiles are removed.
        // Empty tiles have no tile file, hence map with bool instead of just a set (consistency check).
        loadedTileMap iLoadedTiles;
        std::string iBasePath;

        bool GetIntersectionTime(const G3D::Ray& pRay, float& pMaxDist, bool StopAtFirstHit, ModelIgnoreFlags ignoreFlags) const;
    public:
        static std::string getTileFileName(uint32 mapID, uint32 tileX, uint32 tileY);
        static uint32 packTileID(const uint32 tileX, const uint32 tileY) { return tileX << 16 | tileY; }
        static void unpackTileID(const uint32 ID, uint32& tileX, uint32& tileY) { tileX = ID >> 16; tileY = ID & 0xFF; }
        static LoadResult CanLoadMap(const std::string& vmapPath, uint32 mapID, uint32 tileX, uint32 tileY);

        StaticMapTree(uint32 mapID, const std::string& basePath);
        ~StaticMapTree();

        [[nodiscard]] bool isInLineOfSight(const G3D::Vector3& pos1, const G3D::Vector3& pos2, ModelIgnoreFlags ignoreFlags) const;
        bool GetObjectHitPos(const G3D::Vector3& pos1, const G3D::Vector3& pos2, G3D::Vector3& pResultHitPos, float pModifyDist) const;
        [[nodiscard]] float getHeight(const G3D::Vector3& pPos, float maxSearchDist) const;
        bool GetLocationInfo(const G3D::Vector3& pos, LocationInfo& info) const;

        bool InitMap(const std::string& fName);
        void UnloadMap();
        bool LoadMapTile(uint32 tileX, uint32 tileY);
        void UnloadMapTile(uint32 tileX, uint32 tileY);
        [[nodiscard]] bool isTiled() const { return iIsTiled; }
        [[nodiscard]] uint32 numLoadedTiles() const { return iLoadedTiles.size(); }
        void GetModelInstances(ModelInstance*& models, uint32& count);
    };

    struct AreaInfo
    {
        AreaInfo():  ground_Z(-G3D::inf()) { }
        bool result{false};
        float ground_Z;
        uint32 flags{0};
        int32 adtId{0};
        int32 rootId{0};
        int32 groupId{0};
    };
}

#endif
