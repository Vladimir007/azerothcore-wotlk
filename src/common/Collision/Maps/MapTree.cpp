#include "MapTree.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "Errors.h"
#include "Log.h"
#include "ModelIgnoreFlags.h"
#include "ModelInstance.h"
#include "VMapDefinitions.h"
#include "VMapMgr.h"
#include "WorldModelStore.h"

using G3D::Vector3;

namespace VMAP
{
    class MapRayCallback
    {
    public:
        MapRayCallback(ModelInstance* val, const ModelIgnoreFlags ignoreFlags): prims(val), flags(ignoreFlags) { }
        bool operator()(const G3D::Ray& ray, const uint32 entry, float& distance, const bool StopAtFirstHit)
        {
            return prims[entry].intersectRay(ray, distance, StopAtFirstHit, flags);
        }
    protected:
        ModelInstance* prims;
        ModelIgnoreFlags flags;
    };

    class LocationInfoCallback
    {
    public:
        LocationInfoCallback(ModelInstance* val, LocationInfo& info): prims(val), locInfo(info) {}

        bool operator()(const Vector3& point, const uint32 entry)
        {
            return prims[entry].GetLocationInfo(point, locInfo);
        }

        ModelInstance* prims;
        LocationInfo& locInfo;
    };

    StaticMapTree::StaticMapTree(const uint32 mapID, const std::string& basePath)
        : iMapID(mapID), iIsTiled(false), iTreeValues(nullptr), iNTreeValues(0), iBasePath(basePath)
    {
        if (iBasePath.length() > 0 && iBasePath[iBasePath.length() - 1] != '/' && iBasePath[iBasePath.length() - 1] != '\\')
            iBasePath.push_back('/');
    }

    StaticMapTree::~StaticMapTree()
    {
        delete[] iTreeValues;
    }

    std::string StaticMapTree::getTileFileName(const uint32 mapID, const uint32 tileX, const uint32 tileY)
    {
        std::stringstream tileFilename;
        tileFilename.fill('0');
        tileFilename << std::setw(3) << mapID << '_';
        tileFilename << std::setw(2) << tileY << '_' << std::setw(2) << tileX << ".vmtile";
        return tileFilename.str();
    }

    bool StaticMapTree::GetLocationInfo(const Vector3& pos, LocationInfo& info) const
    {
        LocationInfoCallback intersectionCallBack(iTreeValues, info);
        return iTree.intersectPoint(pos, intersectionCallBack);
    }

    bool StaticMapTree::GetIntersectionTime(const G3D::Ray& pRay, float& pMaxDist, const bool StopAtFirstHit, const ModelIgnoreFlags ignoreFlags) const
    {
        float distance = pMaxDist;
        MapRayCallback intersectionCallBack(iTreeValues, ignoreFlags);
        const bool didHit = iTree.intersectRay(pRay, intersectionCallBack, distance, StopAtFirstHit);
        if (didHit)
            pMaxDist = distance;
        return didHit;
    }

    bool StaticMapTree::isInLineOfSight(const Vector3& pos1, const Vector3& pos2, const ModelIgnoreFlags ignoreFlags) const
    {
        float maxDist = (pos2 - pos1).magnitude();

        // Return false if distance is over max float, in case of cheater teleporting to the end of the universe
        if (maxDist == std::numeric_limits<float>::max() || !std::isfinite(maxDist))
            return false;

        // Valid map coords should *never ever* produce float overflow, but this would produce NaNs too
        ASSERT(maxDist < std::numeric_limits<float>::max());

        // Prevent NaN values which can cause BIH intersection to enter infinite loop
        if (maxDist < 1e-10f)
            return true;

        // Direction with length of 1
        const G3D::Ray ray = G3D::Ray::fromOriginAndDirection(pos1, (pos2 - pos1) / maxDist);

        return !GetIntersectionTime(ray, maxDist, true, ignoreFlags);
    }

    /**
    When moving from pos1 to pos2 check if we hit an object. Return true and the position if we hit one
    Return the hit pos or the original dest pos
    */
    bool StaticMapTree::GetObjectHitPos(const Vector3& pos1, const Vector3& pos2, Vector3& pResultHitPos, const float pModifyDist) const
    {
        bool result = false;
        const float maxDist = (pos2 - pos1).magnitude();

        // Valid map coords should *never ever* produce float overflow, but this would produce NaNs too
        ASSERT(maxDist < std::numeric_limits<float>::max());

        // Prevent NaN values which can cause BIH intersection to enter infinite loop
        if (maxDist < 1e-10f)
        {
            pResultHitPos = pos2;
            return false;
        }

        const Vector3 dir = (pos2 - pos1) / maxDist;  // Direction with length of 1
        const G3D::Ray ray(pos1, dir);
        float dist = maxDist;
        if (GetIntersectionTime(ray, dist, false, ModelIgnoreFlags::Nothing))
        {
            pResultHitPos = pos1 + dir * dist;
            if (pModifyDist < 0)
            {
                if ((pResultHitPos - pos1).magnitude() > -pModifyDist)
                    pResultHitPos = pResultHitPos + dir * pModifyDist;
                else
                    pResultHitPos = pos1;
            }
            else
                pResultHitPos = pResultHitPos + dir * pModifyDist;
            result = true;
        }
        else
        {
            pResultHitPos = pos2;
            result = false;
        }
        return result;
    }

    float StaticMapTree::getHeight(const Vector3& pPos, const float maxSearchDist) const
    {
        float height = G3D::finf();
        const auto dir = Vector3(0, 0, -1);
        const G3D::Ray ray(pPos, dir);  // Direction with length of 1
        float maxDist = maxSearchDist;
        if (GetIntersectionTime(ray, maxDist, false, ModelIgnoreFlags::Nothing))
            height = pPos.z - maxDist;
        return height;
    }

    LoadResult StaticMapTree::CanLoadMap(const std::string& vmapPath, const uint32 mapID, const uint32 tileX, const uint32 tileY)
    {
        std::string basePath = vmapPath;
        if (basePath.length() > 0 && basePath[basePath.length() - 1] != '/' && basePath[basePath.length() - 1] != '\\')
            basePath.push_back('/');
        const std::string fullname = basePath + VMapMgr::getMapFileName(mapID);

        auto result = LoadResult::Success;

        FILE* rf = fopen(fullname.c_str(), "rb");
        if (!rf)
            return LoadResult::FileNotFound;

        char tiled;
        char chunk[8];
        if (!readChunk(rf, chunk, VMAP_MAGIC, 8) || fread(&tiled, sizeof(char), 1, rf) != 1)
        {
            fclose(rf);
            return LoadResult::VersionMismatch;
        }
        if (tiled)
        {
            const std::string tileFile = basePath + getTileFileName(mapID, tileX, tileY);
            FILE* tf = fopen(tileFile.c_str(), "rb");
            if (!tf)
                result = LoadResult::FileNotFound;
            else
            {
                if (!readChunk(tf, chunk, VMAP_MAGIC, 8))
                    result = LoadResult::VersionMismatch;
                fclose(tf);
            }
        }
        fclose(rf);
        return result;
    }

    bool StaticMapTree::InitMap(const std::string& fName)
    {
        bool success = false;
        const std::string fullname = iBasePath + fName;
        FILE* rf = fopen(fullname.c_str(), "rb");
        if (!rf)
            return false;

        char chunk[8];
        char tiled = '\0';

        if (readChunk(rf, chunk, VMAP_MAGIC, 8) &&
            fread(&tiled, sizeof(char), 1, rf) == 1 &&
            readChunk(rf, chunk, "NODE", 4) && iTree.readFromFile(rf))
        {
            iNTreeValues = iTree.primCount();
            iTreeValues = new ModelInstance[iNTreeValues];
            success = readChunk(rf, chunk, "GOBJ", 4);
        }

        iIsTiled = static_cast<bool>(tiled);

        // Global model spawns.
        // Only non-tiled maps have them, and if so exactly one (so far at least...)
        ModelSpawn spawn;
        if (!iIsTiled && ModelSpawn::readFromFile(rf, spawn))
        {
            if (const std::shared_ptr<WorldModel> model = sWorldModelStore->AcquireModelInstance(iBasePath, spawn.name, spawn.flags))
                // Assume that global model always is the first and only tree value (could be improved...)
                iTreeValues[0] = ModelInstance(spawn, model);
            else
                success = false;
        }

        fclose(rf);
        return success;
    }

    void StaticMapTree::UnloadMap()
    {
        iLoadedTiles.clear();
    }

    bool StaticMapTree::LoadMapTile(uint32 tileX, uint32 tileY)
    {
        if (!iIsTiled)
        {
            // Currently, core creates grids for all maps, whether it has terrain tiles or not
            // so we need "fake" tile loads to know when we can unload map geometry
            iLoadedTiles[packTileID(tileX, tileY)] = false;
            return true;
        }
        if (!iTreeValues)
        {
            LOG_ERROR("maps", "StaticMapTree::LoadMapTile() : tree has not been initialized [{}, {}]", tileX, tileY);
            return false;
        }

        const std::string tileFile = iBasePath + getTileFileName(iMapID, tileX, tileY);
        FILE* tf = fopen(tileFile.c_str(), "rb");
        if (!tf)
        {
            iLoadedTiles[packTileID(tileX, tileY)] = false;
            return true;
        }

        bool result = true;
        char chunk[8];
        if (!readChunk(tf, chunk, VMAP_MAGIC, 8))
            result = false;
        uint32 numSpawns = 0;
        if (result && fread(&numSpawns, sizeof(uint32), 1, tf) != 1)
            result = false;
        for (uint32 i = 0; i < numSpawns && result; ++i)
        {
            // Read model spawns
            ModelSpawn spawn;
            if (!ModelSpawn::readFromFile(tf, spawn))
            {
                result = false;
                break;
            }

            const std::shared_ptr<WorldModel> model = sWorldModelStore->AcquireModelInstance(iBasePath, spawn.name, spawn.flags);
            if (!model)
            {
                LOG_ERROR("maps", "StaticMapTree::LoadMapTile() : could not acquire WorldModel pointer [{}, {}]", tileX, tileY);
                // Why do we continue to try to load if the model was unsuccessful here?
            }

            // Update tree
            uint32 referencedVal;
            if (fread(&referencedVal, sizeof(uint32), 1, tf) == 1)
            {
                if (referencedVal >= iNTreeValues)
                {
                    LOG_DEBUG("maps", "StaticMapTree::LoadMapTile() : invalid tree element ({}/{})", referencedVal, iNTreeValues);
                    continue;
                }

                // This looks odd and is confusing, took some research to figure it out:
                // the first WorldModel will create a "GroupModel" of all other same-models in the tile
                // we don't actually care about anything else
                if (!iTreeValues[referencedVal].getWorldModel())
                    iTreeValues[referencedVal] = ModelInstance(spawn, model);
            }
            else
                result = false;
        }

        iLoadedTiles[packTileID(tileX, tileY)] = true;
        fclose(tf);
        return result;
    }

    void StaticMapTree::UnloadMapTile(uint32 tileX, uint32 tileY)
    {
        const uint32 tileID = packTileID(tileX, tileY);
        const auto tile = iLoadedTiles.find(tileID);
        if (tile == iLoadedTiles.end())
        {
            LOG_ERROR("maps", "StaticMapTree::UnloadMapTile() : trying to unload non-loaded tile - Map:{} X:{} Y:{}", iMapID, tileX, tileY);
            return;
        }
        iLoadedTiles.erase(tile);
    }

    void StaticMapTree::GetModelInstances(ModelInstance*& models, uint32& count)
    {
        models = iTreeValues;
        count = iNTreeValues;
    }
}
