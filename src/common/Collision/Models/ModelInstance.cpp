#include "ModelInstance.h"
#include "MapTree.h"
#include "WorldModel.h"

using G3D::Vector3;
using G3D::Ray;

namespace VMAP
{
    ModelInstance::ModelInstance(const ModelSpawn& spawn, const std::shared_ptr<WorldModel>& model): ModelSpawn(spawn), iModel(model)
    {
        iInvRot = G3D::Matrix3::fromEulerAnglesZYX(G3D::pi() * iRot.y / 180.f, G3D::pi() * iRot.x / 180.f, G3D::pi() * iRot.z / 180.f).inverse();
        iInvScale = 1.f / iScale;
    }

    bool ModelInstance::intersectRay(const Ray& pRay, float& pMaxDist, const bool StopAtFirstHit, const ModelIgnoreFlags ignoreFlags) const
    {
        if (!iModel)
            return false;

        if (const float time = pRay.intersectionTime(iBound); time == G3D::inf())
            return false;

        // Child bounds are defined in object space
        const Vector3 p = iInvRot * (pRay.origin() - iPos) * iInvScale;
        const Ray modRay(p, iInvRot * pRay.direction());
        float distance = pMaxDist * iInvScale;
        const bool hit = iModel->IntersectRay(modRay, distance, StopAtFirstHit, ignoreFlags);
        if (hit)
            pMaxDist = distance * iScale;
        return hit;
    }

    bool ModelInstance::GetLocationInfo(const Vector3& p, LocationInfo& info) const
    {
        if (!iModel)
            return false;
        if (flags & MOD_M2)
            return false;  // M2 files don't contain area info, only WMO files
        if (!iBound.contains(p))
            return false;

        // Child bounds are defined in object space
        const Vector3 pModel = iInvRot * (p - iPos) * iInvScale;
        const Vector3 zDirModel = iInvRot * Vector3(0.f, 0.f, -1.f);

        float zDist;
        GroupLocationInfo groupInfo;
        if (!iModel->GetLocationInfo(pModel, zDirModel, zDist, groupInfo))
            return false;

        const Vector3 modelGround = pModel + zDist * zDirModel;

        // Transform back to world space.
        // Note that: Mat * vec == vec * Mat.transpose()
        // and for rotation matrices: Mat.inverse() == Mat.transpose()
        const float world_Z = (modelGround * iInvRot * iScale + iPos).z;
        if (info.ground_Z >= world_Z) // Could it be handled automatically with zDist at intersection?
            return false;

        info.rootId = groupInfo.rootId;
        info.hitModel = groupInfo.hitModel;
        info.ground_Z = world_Z;
        info.hitInstance = this;
        return true;
    }

    bool ModelInstance::GetLiquidLevel(const Vector3& p, const LocationInfo& info, float& liqHeight) const
    {
        // Child bounds are defined in object space
        const Vector3 pModel = iInvRot * (p - iPos) * iInvScale;
        float zDist;
        if (!info.hitModel->GetLiquidLevel(pModel, zDist))
            return false;

        // Calculate world height (zDist in model coords):
        liqHeight = (Vector3(pModel.x, pModel.y, zDist) * iInvRot * iScale + iPos).z;
        return true;
    }

#define READ_OR_RETURN(V, S, N) if (fread(V, S, N, rf) != N) return false;
    bool ModelSpawn::readFromFile(FILE* rf, ModelSpawn& spawn)
    {
        READ_OR_RETURN(&spawn.flags, sizeof(uint32), 1)
        READ_OR_RETURN(&spawn.adtId, sizeof(uint16), 1)
        READ_OR_RETURN(&spawn.ID, sizeof(uint32), 1)
        READ_OR_RETURN(&spawn.iPos, sizeof(float), 3)
        READ_OR_RETURN(&spawn.iRot, sizeof(float), 3)
        READ_OR_RETURN(&spawn.iScale, sizeof(float), 1)

        // Only WMOs have bound in MPQ, only available after computation
        if (spawn.flags & MOD_HAS_BOUND)
        {
            Vector3 bLow, bHigh;
            READ_OR_RETURN(&bLow, sizeof(float), 3)
            READ_OR_RETURN(&bHigh, sizeof(float), 3)
            spawn.iBound = G3D::AABox(bLow, bHigh);
        }
        uint32 nameLen;
        READ_OR_RETURN(&nameLen, sizeof(uint32), 1)

        char nameBuff[500];
        if (nameLen > 500) // File names should never be that long, must be file error
        {
            std::cout << "Error reading ModelSpawn: file name too long!\n";
            return false;
        }
        READ_OR_RETURN(nameBuff, sizeof(char), nameLen)
        spawn.name = std::string(nameBuff, nameLen);
        return true;
    }
#undef READ_OR_RETURN

#define WRITE_OR_RETURN(V, S, N) if (fwrite(V, S, N, wf) != N) return false;
    bool ModelSpawn::writeToFile(FILE* wf, const ModelSpawn& spawn)
    {
        WRITE_OR_RETURN(&spawn.flags, sizeof(uint32), 1)
        WRITE_OR_RETURN(&spawn.adtId, sizeof(uint16), 1)
        WRITE_OR_RETURN(&spawn.ID, sizeof(uint32), 1)
        WRITE_OR_RETURN(&spawn.iPos, sizeof(float), 3)
        WRITE_OR_RETURN(&spawn.iRot, sizeof(float), 3)
        WRITE_OR_RETURN(&spawn.iScale, sizeof(float), 1)

        // Only WMOs have bound in MPQ, only available after computation
        if (spawn.flags & MOD_HAS_BOUND)
        {
            WRITE_OR_RETURN(&spawn.iBound.low(), sizeof(float), 3)
            WRITE_OR_RETURN(&spawn.iBound.high(), sizeof(float), 3)
        }

        const uint32 nameLen = spawn.name.length();
        WRITE_OR_RETURN(&nameLen, sizeof(uint32), 1)
        WRITE_OR_RETURN(spawn.name.c_str(), sizeof(char), nameLen)
        return true;
    }
#undef WRITE_OR_RETURN
}
