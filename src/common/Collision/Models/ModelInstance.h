#ifndef MODEL_INSTANCE_H
#define MODEL_INSTANCE_H

#include <memory>
#include <G3D/AABox.h>
#include <G3D/Matrix3.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>
#include "Define.h"

namespace VMAP
{
    class WorldModel;
    struct AreaInfo;
    struct LocationInfo;
    enum class ModelIgnoreFlags : uint32;

    enum ModelFlags
    {
        MOD_M2          = 0x1,
        MOD_WORLD_SPAWN = 0x2,
        MOD_HAS_BOUND   = 0x4
    };

    class ModelSpawn
    {
    public:
        uint32 flags;
        uint16 adtId;
        uint32 ID;
        G3D::Vector3 iPos;
        G3D::Vector3 iRot;
        float iScale;
        G3D::AABox iBound;
        std::string name;
        bool operator==(const ModelSpawn& other) const { return ID == other.ID; }
        [[nodiscard]] const G3D::AABox& GetBounds() const { return iBound; }

        static bool readFromFile(FILE* rf, ModelSpawn& spawn);
        static bool writeToFile(FILE* wf, const ModelSpawn& spawn);
    };

    class ModelInstance: public ModelSpawn
    {
    public:
        ModelInstance() { }
        ModelInstance(const ModelSpawn& spawn, const std::shared_ptr<WorldModel>& model);
        bool intersectRay(const G3D::Ray& pRay, float& pMaxDist, bool StopAtFirstHit, ModelIgnoreFlags ignoreFlags) const;
        bool GetLocationInfo(const G3D::Vector3& p, LocationInfo& info) const;
        bool GetLiquidLevel(const G3D::Vector3& p, const LocationInfo& info, float& liqHeight) const;
        WorldModel* getWorldModel() { return iModel.get(); }
    protected:
        G3D::Matrix3 iInvRot;
        float iInvScale{0.0f};
        std::shared_ptr<WorldModel> iModel;
    };
}

#endif
