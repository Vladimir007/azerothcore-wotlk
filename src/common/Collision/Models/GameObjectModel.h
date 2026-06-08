#ifndef GAME_OBJECT_MODEL_H
#define GAME_OBJECT_MODEL_H

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
}

class GameObject;
struct GameObjectDisplayInfoEntry;

class GameObjectModelOwnerBase
{
public:
    virtual ~GameObjectModelOwnerBase() = default;

    [[nodiscard]] virtual bool IsSpawned() const = 0;
    [[nodiscard]] virtual uint32 GetDisplayId() const = 0;
    [[nodiscard]] virtual uint32 GetPhaseMask() const = 0;
    [[nodiscard]] virtual G3D::Vector3 GetPosition() const = 0;
    [[nodiscard]] virtual float GetOrientation() const = 0;
    [[nodiscard]] virtual float GetScale() const = 0;
    virtual void DebugVisualizeCorner(G3D::Vector3 const& /*corner*/) const = 0;
};

class GameObjectModel
{
    GameObjectModel()  = default;

public:
    std::string name;

    [[nodiscard]] const G3D::AABox& GetBounds() const { return iBound; }

    ~GameObjectModel() = default;

    [[nodiscard]] const G3D::Vector3& GetPosition() const { return iPos; }

    /// Enables/disables collision.
    void disable() { phaseMask = 0; }
    void enable(const uint32 phMask) { phaseMask = phMask; }

    [[nodiscard]] bool isEnabled() const { return phaseMask != 0; }
    [[nodiscard]] bool IsMapObject() const { return isWmo; }

    bool intersectRay(const G3D::Ray& ray, float& MaxDist, bool StopAtFirstHit, uint32 phMask, VMAP::ModelIgnoreFlags ignoreFlags) const;
    bool GetLocationInfo(const G3D::Vector3& point, VMAP::LocationInfo& info, uint32 phMask) const;
    bool GetLiquidLevel(const G3D::Vector3& point, const VMAP::LocationInfo& info, float& liqHeight) const;

    static GameObjectModel* Create(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, const std::string& dataPath);

    bool UpdatePosition();

private:
    bool initialize(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, const std::string& dataPath);

    uint32 phaseMask{0};
    G3D::AABox iBound;
    G3D::Matrix3 iInvRot;
    G3D::Vector3 iPos;
    float iInvScale{0};
    float iScale{0};
    std::shared_ptr<VMAP::WorldModel> iModel;
    std::unique_ptr<GameObjectModelOwnerBase> owner;
    bool isWmo{false};
};

void LoadGameObjectModelList(const std::string& dataPath);

#endif
