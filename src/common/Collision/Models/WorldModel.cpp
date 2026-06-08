#include "WorldModel.h"
#include <array>

#include "MapTree.h"
#include "ModelIgnoreFlags.h"
#include "ModelInstance.h"
#include "VMapDefinitions.h"

using G3D::Vector3;

template<> struct BoundsTrait<VMAP::GroupModel>
{
    static void GetBounds(const VMAP::GroupModel& obj, G3D::AABox& out) { out = obj.GetBound(); }
};

namespace VMAP
{
    bool IntersectTriangle(const MeshTriangle& tri, const std::vector<Vector3>::const_iterator points, const G3D::Ray& ray, float& distance)
    {
        static constexpr float EPS = 1e-5f;

        const Vector3 e1 = points[tri.idx1] - points[tri.idx0];
        const Vector3 e2 = points[tri.idx2] - points[tri.idx0];
        const Vector3 p(ray.direction().cross(e2));
        const float a = e1.dot(p);

        if (std::fabs(a) < EPS)
            return false;  // Determinant is ill-conditioned; abort early

        const float f = 1.0f / a;
        const Vector3 s(ray.origin() - points[tri.idx0]);
        const float u = f * s.dot(p);

        if (u < 0.0f || u > 1.0f)
            return false;  // We hit the plane of the m_geometry, but outside the m_geometry

        const Vector3 q(s.cross(e1));

        if (const float v = f * ray.direction().dot(q); v < 0.0f || u + v > 1.0f)
            return false;  // We hit the plane of the triangle, but outside the triangle

        if (const float t = f * e2.dot(q); t > 0.0f && t < distance)
        {
            // This is a new hit, closer than the previous one
            distance = t;
            return true;
        }

        // This hit is after the previous hit, so ignore it
        return false;
    }

    class TriBoundFunc
    {
    public:
        explicit TriBoundFunc(std::vector<Vector3>& vert): vertices(vert.begin()) { }
        void operator()(const MeshTriangle& tri, G3D::AABox& out) const
        {
            Vector3 lo = vertices[tri.idx0];
            Vector3 hi = lo;

            lo = lo.min(vertices[tri.idx1]).min(vertices[tri.idx2]);
            hi = hi.max(vertices[tri.idx1]).max(vertices[tri.idx2]);

            out = G3D::AABox(lo, hi);
        }
    protected:
        const std::vector<Vector3>::const_iterator vertices;
    };

    WmoLiquid::WmoLiquid(const uint32 width, const uint32 height, const Vector3& corner, const uint32 type):
        iTilesX(width), iTilesY(height), iCorner(corner), iType(type)
    {
        if (width && height)
        {
            iHeight = new float[(width + 1) * (height + 1)];
            iFlags = new uint8[width * height];
        }
        else
        {
            iHeight = new float[1];
            iFlags = nullptr;
        }
    }

    WmoLiquid::WmoLiquid(const WmoLiquid& other)
    {
        *this = other; // Use assignment operator
    }

    WmoLiquid::~WmoLiquid()
    {
        delete[] iHeight;
        delete[] iFlags;
    }

    WmoLiquid& WmoLiquid::operator=(const WmoLiquid& other)
    {
        if (this == &other)
            return *this;
        iTilesX = other.iTilesX;
        iTilesY = other.iTilesY;
        iCorner = other.iCorner;
        iType = other.iType;
        delete[] iHeight;
        delete[] iFlags;
        if (other.iHeight)
        {
            iHeight = new float[(iTilesX + 1) * (iTilesY + 1)];
            memcpy(iHeight, other.iHeight, (iTilesX + 1) * (iTilesY + 1)*sizeof(float));
        }
        else
            iHeight = nullptr;
        if (other.iFlags)
        {
            iFlags = new uint8[iTilesX * iTilesY];
            memcpy(iFlags, other.iFlags, iTilesX * iTilesY);
        }
        else
            iFlags = nullptr;
        return *this;
    }

    bool WmoLiquid::GetLiquidHeight(const Vector3& pos, float& liqHeight) const
    {
        if (!iHeight)
            return false;

        // Simple case
        if (!iFlags)
        {
            liqHeight = iHeight[0];
            return true;
        }

        const float tx_f = (pos.x - iCorner.x) / LIQUID_TILE_SIZE;
        const uint32 tx = static_cast<uint32>(tx_f);
        if (tx_f < 0.0f || tx >= iTilesX)
            return false;

        const float ty_f = (pos.y - iCorner.y) / LIQUID_TILE_SIZE;
        const uint32 ty = static_cast<uint32>(ty_f);
        if (ty_f < 0.0f || ty >= iTilesY)
            return false;

        // Check if tile shall be used for liquid level.
        // Checking for 0x08 *might* be enough, but disabled tiles always are 0x?F.
        if ((iFlags[tx + ty * iTilesX] & 0x0F) == 0x0F)
            return false;

        // (dx, dy) coordinates inside tile, in [0, 1]^2
        const float dx = tx_f - static_cast<float>(tx);
        const float dy = ty_f - static_cast<float>(ty);

        /* Tessellate tile to two triangles (not sure if client does it exactly like this)
            ^ dy
            |
          1 x---------x (1, 1)
            | (b)   / |
            |     /   |
            |   /     |
            | /   (a) |
            x---------x---> dx
          0           1
        */
        const uint32 rowOffset = iTilesX + 1;
        if (dx > dy) // Case (a)
        {
            const float sx = iHeight[tx + 1 +  ty    * rowOffset] - iHeight[tx   + ty * rowOffset];
            const float sy = iHeight[tx + 1 + (ty + 1) * rowOffset] - iHeight[tx + 1 + ty * rowOffset];
            liqHeight = iHeight[tx + ty * rowOffset] + dx * sx + dy * sy;
        }
        else // Case (b)
        {
            const float sx = iHeight[tx + 1 + (ty + 1) * rowOffset] - iHeight[tx + (ty + 1) * rowOffset];
            const float sy = iHeight[tx   + (ty + 1) * rowOffset] - iHeight[tx +  ty    * rowOffset];
            liqHeight = iHeight[tx + ty * rowOffset] + dx * sx + dy * sy;
        }
        return true;
    }

    uint32 WmoLiquid::GetFileSize()
    {
        return 2 * sizeof(uint32) + sizeof(Vector3) + sizeof(uint32) +
            (iFlags ? (iTilesX + 1) * (iTilesY + 1) * sizeof(float) + iTilesX * iTilesY : sizeof(float));
    }

    bool WmoLiquid::writeToFile(FILE* wf)
    {
        bool result = false;
        if (fwrite(&iTilesX, sizeof(uint32), 1, wf) == 1 &&
            fwrite(&iTilesY, sizeof(uint32), 1, wf) == 1 &&
            fwrite(&iCorner, sizeof(Vector3), 1, wf) == 1 &&
            fwrite(&iType, sizeof(uint32), 1, wf) == 1)
        {
            if (iTilesX && iTilesY)
            {
                uint32 size = (iTilesX + 1) * (iTilesY + 1);
                if (fwrite(iHeight, sizeof(float), size, wf) == size)
                {
                    size = iTilesX * iTilesY;
                    result = fwrite(iFlags, sizeof(uint8), size, wf) == size;
                }
            }
            else
                result = fwrite(iHeight, sizeof(float), 1, wf) == 1;
        }

        return result;
    }

    bool WmoLiquid::readFromFile(FILE* rf, WmoLiquid*& out)
    {
        bool result = false;
        const auto liquid = new WmoLiquid();

        if (fread(&liquid->iTilesX, sizeof(uint32), 1, rf) == 1 &&
            fread(&liquid->iTilesY, sizeof(uint32), 1, rf) == 1 &&
            fread(&liquid->iCorner, sizeof(Vector3), 1, rf) == 1 &&
            fread(&liquid->iType, sizeof(uint32), 1, rf) == 1)
        {
            if (liquid->iTilesX && liquid->iTilesY)
            {
                uint32 size = (liquid->iTilesX + 1) * (liquid->iTilesY + 1);
                liquid->iHeight = new float[size];
                if (fread(liquid->iHeight, sizeof(float), size, rf) == size)
                {
                    size = liquid->iTilesX * liquid->iTilesY;
                    liquid->iFlags = new uint8[size];
                    result = fread(liquid->iFlags, sizeof(uint8), size, rf) == size;
                }
            }
            else
            {
                liquid->iHeight = new float[1];
                result = fread(liquid->iHeight, sizeof(float), 1, rf) == 1;
            }
        }

        if (!result)
            delete liquid;
        else
            out = liquid;

        return result;
    }

    void WmoLiquid::GetPosInfo(uint32& tilesX, uint32& tilesY, Vector3& corner) const
    {
        tilesX = iTilesX;
        tilesY = iTilesY;
        corner = iCorner;
    }

    GroupModel::GroupModel(const GroupModel& other):
        iBound(other.iBound), iMogpFlags(other.iMogpFlags), iGroupWMOid(other.iGroupWMOid),
        vertices(other.vertices), triangles(other.triangles), meshTree(other.meshTree)
    {
        if (other.iLiquid)
            iLiquid = new WmoLiquid(*other.iLiquid);
    }

    void GroupModel::setMeshData(std::vector<Vector3>& vert, std::vector<MeshTriangle>& tri)
    {
        vertices.swap(vert);
        triangles.swap(tri);
        TriBoundFunc bFunc(vertices);
        meshTree.build(triangles, bFunc);
    }

    bool GroupModel::writeToFile(FILE* wf)
    {
        uint32 chunkSize, count;

        if (fwrite(&iBound, sizeof(G3D::AABox), 1, wf) != 1) return false;
        if (fwrite(&iMogpFlags, sizeof(uint32), 1, wf) != 1) return false;
        if (fwrite(&iGroupWMOid, sizeof(uint32), 1, wf) != 1) return false;

        // Write vertices
        if (fwrite("VERT", 1, 4, wf) != 4) return false;
        count = vertices.size();
        chunkSize = sizeof(uint32) + sizeof(Vector3) * count;
        if (fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) return false;
        if (fwrite(&count, sizeof(uint32), 1, wf) != 1) return false;

        // Models without (collision) geometry end here, unsure if they are useful
        if (!count) return true;

        if (fwrite(&vertices[0], sizeof(Vector3), count, wf) != count) return false;

        // Write triangle mesh
        if (fwrite("TRIM", 1, 4, wf) != 4) return false;
        count = triangles.size();
        chunkSize = sizeof(uint32) + sizeof(MeshTriangle) * count;
        if (fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) return false;
        if (fwrite(&count, sizeof(uint32), 1, wf) != 1) return false;
        if (fwrite(&triangles[0], sizeof(MeshTriangle), count, wf) != count) return false;

        // Write mesh BIH
        if (fwrite("MBIH", 1, 4, wf) != 4) return false;
        if (!meshTree.writeToFile(wf)) return false;

        // Write liquid data
        if (fwrite("LIQU", 1, 4, wf) != 4) return false;
        if (!iLiquid)
        {
            chunkSize = 0;
            if (fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) return false;
            return true;
        }

        chunkSize = iLiquid->GetFileSize();
        if (fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) return false;
        if (!iLiquid->writeToFile(wf)) return false;
        return true;
    }

    bool GroupModel::readFromFile(FILE* rf)
    {
        char chunk[8];
        uint32 chunkSize = 0;
        uint32 count = 0;
        triangles.clear();
        vertices.clear();
        delete iLiquid;
        iLiquid = nullptr;

        if (fread(&iBound, sizeof(G3D::AABox), 1, rf) != 1) return false;
        if (fread(&iMogpFlags, sizeof(uint32), 1, rf) != 1) return false;
        if (fread(&iGroupWMOid, sizeof(uint32), 1, rf) != 1) return false;

        // Read vertices
        if (!readChunk(rf, chunk, "VERT", 4)) return false;
        if (fread(&chunkSize, sizeof(uint32), 1, rf) != 1) return false;
        if (fread(&count, sizeof(uint32), 1, rf) != 1) return false;

        // Models without (collision) geometry end here, unsure if they are useful
        if (!count) return true;

        vertices.resize(count);
        if (fread(&vertices[0], sizeof(Vector3), count, rf) != count) return false;

        // Read triangle mesh
        if (!readChunk(rf, chunk, "TRIM", 4)) return false;
        if (fread(&chunkSize, sizeof(uint32), 1, rf) != 1) return false;
        if (fread(&count, sizeof(uint32), 1, rf) != 1) return false;
        triangles.resize(count);
        if (fread(&triangles[0], sizeof(MeshTriangle), count, rf) != count) return false;

        // Read mesh BIH
        if (!readChunk(rf, chunk, "MBIH", 4)) return false;
        if (!meshTree.readFromFile(rf)) return false;

        // Write liquid data
        if (!readChunk(rf, chunk, "LIQU", 4)) return false;
        if (fread(&chunkSize, sizeof(uint32), 1, rf) != 1) return false;
        if (chunkSize > 0)
            if (!WmoLiquid::readFromFile(rf, iLiquid)) return false;
        return true;
    }

    struct GModelRayCallback
    {
        GModelRayCallback(const std::vector<MeshTriangle>& tris, const std::vector<Vector3>& vert):
            vertices(vert.begin()), triangles(tris.begin()) { }

        bool operator()(const G3D::Ray& ray, const uint32 entry, float& distance, bool /*StopAtFirstHit*/)
        {
            return IntersectTriangle(triangles[entry], vertices, ray, distance);
        }

        std::vector<Vector3>::const_iterator vertices;
        std::vector<MeshTriangle>::const_iterator triangles;
    };

    bool GroupModel::IntersectRay(const G3D::Ray& ray, float& distance, const bool stopAtFirstHit) const
    {
        if (triangles.empty())
            return false;
        GModelRayCallback callback(triangles, vertices);
        return meshTree.intersectRay(ray, callback, distance, stopAtFirstHit);
    }

    inline bool IsInsideOrAboveBound(G3D::AABox const& bounds, const G3D::Point3& point)
    {
        return point.x >= bounds.low().x && point.y >= bounds.low().y && point.z >= bounds.low().z &&
            point.x <= bounds.high().x && point.y <= bounds.high().y;
    }

    GroupModel::InsideResult GroupModel::IsInsideObject(G3D::Ray const& ray, float& z_dist) const
    {
        if (triangles.empty() || !IsInsideOrAboveBound(iBound, ray.origin()))
            return OUT_OF_BOUNDS;

        if (meshTree.bound().high().z >= ray.origin().z)
        {
            float dist = G3D::finf();
            if (IntersectRay(ray, dist, false))
            {
                z_dist = dist - 0.1f;
                return INSIDE;
            }
            if (meshTree.bound().contains(ray.origin()))
                return MAYBE_INSIDE;
        }
        else
        {
            // Some group models don't have any floor to intersect with,
            // so we should attempt to intersect with a model part below this group,
            // then find back where we originated from (in WorldModel::GetLocationInfo)
            float dist = G3D::finf();
            float delta = ray.origin().z - meshTree.bound().high().z;
            if (IntersectRay(ray.bumpedRay(delta), dist, false))
            {
                z_dist = dist - 0.1f + delta;
                return ABOVE;
            }
        }

        return OUT_OF_BOUNDS;
    }

    bool GroupModel::GetLiquidLevel(const Vector3& pos, float& liqHeight) const
    {
        if (iLiquid)
            return iLiquid->GetLiquidHeight(pos, liqHeight);
        return false;
    }

    uint32 GroupModel::GetLiquidType() const
    {
        if (iLiquid)
            return iLiquid->GetType();
        return 0;
    }

    void GroupModel::GetMeshData(std::vector<Vector3>& outVertices, std::vector<MeshTriangle>& outTriangles, WmoLiquid*& liquid)
    {
        outVertices = vertices;
        outTriangles = triangles;
        liquid = iLiquid;
    }

    void WorldModel::setGroupModels(std::vector<GroupModel>& models)
    {
        groupModels.swap(models);
        groupTree.build(groupModels, BoundsTrait<GroupModel>::GetBounds, 1);
    }

    struct WModelRayCallBack
    {
        explicit WModelRayCallBack(const std::vector<GroupModel>& mod): models(mod.begin()) { }

        bool operator()(const G3D::Ray& ray, const uint32 entry, float& distance, const bool StopAtFirstHit)
        {
            return models[entry].IntersectRay(ray, distance, StopAtFirstHit);
        }

        std::vector<GroupModel>::const_iterator models;
    };

    bool WorldModel::IntersectRay(const G3D::Ray& ray, float& distance, const bool stopAtFirstHit, const ModelIgnoreFlags ignoreFlags) const
    {
        // If the caller asked us to ignore certain objects we should check flags
        if ((ignoreFlags & ModelIgnoreFlags::M2) != ModelIgnoreFlags::Nothing)
        {
            // M2 models are not taken into account for LoS calculation if caller requested their ignoring.
            if (Flags & MOD_M2)
                return false;
        }

        // Small M2 workaround, maybe better make separate class with virtual intersection funcs
        // in any case, there's no need to use a bound tree if we only have one submodel
        if (groupModels.size() == 1)
            return groupModels[0].IntersectRay(ray, distance, stopAtFirstHit);

        WModelRayCallBack isc(groupModels);
        return groupTree.intersectRay(ray, isc, distance, stopAtFirstHit);
    }

    class WModelAreaCallback
    {
    public:
        explicit WModelAreaCallback(std::vector<GroupModel> const& vals) : prims(vals), hit() { }

        std::vector<GroupModel> const& prims;
        std::array<GroupModel const*, 3> hit;

        bool operator()(G3D::Ray const& ray, const uint32 entry, float& distance, bool /*stopAtFirstHit*/)
        {
            float group_Z;
            if (const GroupModel::InsideResult result = prims[entry].IsInsideObject(ray, group_Z); result != GroupModel::OUT_OF_BOUNDS)
            {
                if (result != GroupModel::MAYBE_INSIDE)
                {
                    if (group_Z < distance)
                    {
                        distance = group_Z;
                        hit[result] = &prims[entry];
                        return true;
                    }
                }
                else
                    hit[result] = &prims[entry];
            }
            return false;
        }
    };

    bool WorldModel::GetLocationInfo(const Vector3& p, const Vector3& down, float& dist, GroupLocationInfo& info) const
    {
        if (groupModels.empty())
        {
            return false;
        }

        WModelAreaCallback callback(groupModels);
        const G3D::Ray r(p - down * 0.1f, down);
        float zDist = groupTree.bound().extent().length();
        groupTree.intersectRay(r, callback, zDist, false);
        if (callback.hit[GroupModel::INSIDE])
        {
            info.rootId = RootWMOid;
            info.hitModel = callback.hit[GroupModel::INSIDE];
            dist = zDist;
            return true;
        }

        // Some group models don't have any floor to intersect with,
        // so we should attempt to intersect with a model part below the group `p` is in (stored in GroupModel::ABOVE),
        // then find back where we originated from (GroupModel::MAYBE_INSIDE).
        if (callback.hit[GroupModel::MAYBE_INSIDE] && callback.hit[GroupModel::ABOVE])
        {
            info.rootId   = RootWMOid;
            info.hitModel = callback.hit[GroupModel::MAYBE_INSIDE];
            dist = zDist;
            return true;
        }
        return false;
    }

    bool WorldModel::writeFile(FILE* wf)
    {
        uint32 chunkSize;
        if (fwrite(VMAP_MAGIC, 1, 8, wf) != 8) return false;
        if (fwrite("WMOD", 1, 4, wf) != 4) return false;
        chunkSize = sizeof(uint32) + sizeof(uint32);
        if (fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) return false;
        if (fwrite(&RootWMOid, sizeof(uint32), 1, wf) != 1) return false;

        // Write group models
        const uint32 count = groupModels.size();
        if (!count)
            return true;
        if (fwrite("GMOD", 1, 4, wf) != 4) return false;
        if (fwrite(&count, sizeof(uint32), 1, wf) != 1) return false;
        for (uint32 i = 0; i < groupModels.size(); ++i)
            if (!groupModels[i].writeToFile(wf)) return false;

        // Write group BIH
        if (fwrite("GBIH", 1, 4, wf) != 4) return false;
        return groupTree.writeToFile(wf);
    }

#define RAD_MAGIC_OR_RETURN(V, L) if (!readChunk(rf, chunk, V, L)) { fclose(rf); return false; }
#define READ_OR_RETURN(V) if (!(V)) { fclose(rf); return false; }
    bool WorldModel::readFile(const std::string& filename)
    {
        FILE* rf = fopen(filename.c_str(), "rb");
        if (!rf)
            return false;

        uint32 chunkSize = 0;
        uint32 count = 0;
        char chunk[8];

        RAD_MAGIC_OR_RETURN(VMAP_MAGIC, 8)
        RAD_MAGIC_OR_RETURN("WMOD", 4)
        READ_OR_RETURN(fread(&chunkSize, sizeof(uint32), 1, rf) == 1)
        READ_OR_RETURN(fread(&RootWMOid, sizeof(uint32), 1, rf) == 1)

        // Read group models
        RAD_MAGIC_OR_RETURN("GMOD", 4)
        READ_OR_RETURN(fread(&count, sizeof(uint32), 1, rf) == 1)

        groupModels.resize(count);
        for (uint32 i = 0; i < count; ++i)
        {
            READ_OR_RETURN(groupModels[i].readFromFile(rf))
        }

        // Read group BIH
        RAD_MAGIC_OR_RETURN("GBIH", 4)
        READ_OR_RETURN(groupTree.readFromFile(rf))

        fclose(rf);
        return true;
    }
#undef RAD_MAGIC_OR_RETURN
#undef READ_OR_RETURN

    void WorldModel::GetGroupModels(std::vector<GroupModel>& outGroupModels)
    {
        outGroupModels = groupModels;
    }
}
