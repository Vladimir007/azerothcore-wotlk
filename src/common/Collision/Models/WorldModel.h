#ifndef WORLD_MODEL_H
#define WORLD_MODEL_H

#include <G3D/AABox.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>
#include "BoundingIntervalHierarchy.h"
#include "Define.h"

namespace VMAP
{
    class TreeNode;
    struct AreaInfo;
    struct LocationInfo;
    struct GroupLocationInfo;
    enum class ModelIgnoreFlags : uint32;

    class MeshTriangle
    {
    public:
        MeshTriangle()  { }
        MeshTriangle(const uint32 na, const uint32 nb, const uint32 nc): idx0(na), idx1(nb), idx2(nc) { }

        uint32 idx0{0};
        uint32 idx1{0};
        uint32 idx2{0};
    };

    class WmoLiquid
    {
    public:
        WmoLiquid(uint32 width, uint32 height, const G3D::Vector3& corner, uint32 type);
        WmoLiquid(const WmoLiquid& other);
        ~WmoLiquid();
        WmoLiquid& operator=(const WmoLiquid& other);
        bool GetLiquidHeight(const G3D::Vector3& pos, float& liqHeight) const;
        [[nodiscard]] uint32 GetType() const { return iType; }
        float* GetHeightStorage() { return iHeight; }
        uint8* GetFlagsStorage() { return iFlags; }
        uint32 GetFileSize();
        bool writeToFile(FILE* wf);
        static bool readFromFile(FILE* rf, WmoLiquid*& out);
        void GetPosInfo(uint32& tilesX, uint32& tilesY, G3D::Vector3& corner) const;
    private:
        WmoLiquid() { }
        uint32 iTilesX{0};
        uint32 iTilesY{0};
        G3D::Vector3 iCorner;    // The lower corner
        uint32 iType{0};         // Liquid type
        float* iHeight{nullptr}; // (tilesX + 1) * (tilesY + 1) height values
        uint8* iFlags{nullptr};  // Info if liquid tile is used
    };

    class GroupModel
    {
    public:
        GroupModel() { }
        GroupModel(const GroupModel& other);
        GroupModel(const uint32 mogpFlags, const uint32 groupWMOid, const G3D::AABox& bound):
            iBound(bound), iMogpFlags(mogpFlags), iGroupWMOid(groupWMOid) { }
        ~GroupModel() { delete iLiquid; }

        // Pass mesh data to object and create BIH. Passed vectors get swapped with old geometry!
        void setMeshData(std::vector<G3D::Vector3>& vert, std::vector<MeshTriangle>& tri);
        void setLiquidData(WmoLiquid*& liquid) { iLiquid = liquid; liquid = nullptr; }
        bool IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit) const;
        enum InsideResult { INSIDE = 0, MAYBE_INSIDE = 1, ABOVE = 2, OUT_OF_BOUNDS = -1 };
        InsideResult IsInsideObject(G3D::Ray const& ray, float& z_dist) const;
        bool GetLiquidLevel(const G3D::Vector3& pos, float& liqHeight) const;
        [[nodiscard]] uint32 GetLiquidType() const;
        bool writeToFile(FILE* wf);
        bool readFromFile(FILE* rf);
        [[nodiscard]] G3D::AABox const& GetBound() const { return iBound; }
        [[nodiscard]] G3D::AABox const& GetMeshTreeBound() const { return meshTree.bound(); }
        [[nodiscard]] uint32 GetMogpFlags() const { return iMogpFlags; }
        [[nodiscard]] uint32 GetWmoID() const { return iGroupWMOid; }
        void GetMeshData(std::vector<G3D::Vector3>& outVertices, std::vector<MeshTriangle>& outTriangles, WmoLiquid*& liquid);
    protected:
        G3D::AABox iBound;
        uint32 iMogpFlags{0};// 0x8 outdoor; 0x2000 indoor
        uint32 iGroupWMOid{0};
        std::vector<G3D::Vector3> vertices;
        std::vector<MeshTriangle> triangles;
        BIH meshTree;
        WmoLiquid* iLiquid{nullptr};
    };

    /// Holds a model (converted M2 or WMO) in its original coordinate space
    class WorldModel
    {
    public:
        WorldModel() { }

        // Pass group models to WorldModel and create BIH. Passed vector is swapped with old geometry!
        void setGroupModels(std::vector<GroupModel>& models);
        void setRootWmoID(const uint32 id) { RootWMOid = id; }
        bool IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit, ModelIgnoreFlags ignoreFlags) const;
        bool GetLocationInfo(const G3D::Vector3& p, const G3D::Vector3& down, float& dist, GroupLocationInfo& info) const;
        bool writeFile(FILE* wf);
        bool readFile(const std::string& filename);
        void GetGroupModels(std::vector<GroupModel>& outGroupModels);
        uint32 Flags{0};
    protected:
        uint32 RootWMOid{0};
        std::vector<GroupModel> groupModels;
        BIH groupTree;
    };
}

#endif
