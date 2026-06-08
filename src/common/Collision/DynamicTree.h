#ifndef DYNAMIC_TREE_H
#define DYNAMIC_TREE_H

#include "Define.h"
#include "Optional.h"

namespace G3D
{
    class Ray;
    class Vector3;
}

namespace VMAP
{
    struct AreaAndLiquidData;
    enum class ModelIgnoreFlags : uint32;
}

class GameObjectModel;
struct DynTreeImpl;

class DynamicMapTree
{
    DynTreeImpl* impl;

public:
    DynamicMapTree();
    ~DynamicMapTree();

    [[nodiscard]] bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2,
                                       uint32 phaseMask, VMAP::ModelIgnoreFlags ignoreFlags) const;

    bool GetIntersectionTime(uint32 phaseMask, const G3D::Ray& ray, const G3D::Vector3& endPos, float& maxDist) const;

    bool GetAreaAndLiquidData(float x, float y, float z, uint32 phaseMask, Optional<uint8> reqLiquidType, VMAP::AreaAndLiquidData& data) const;

    bool GetObjectHitPos(uint32 phaseMask, const G3D::Vector3& startPos, const G3D::Vector3& endPos,
                         G3D::Vector3& resultHit, float modifyDist) const;

    [[nodiscard]] float getHeight(float x, float y, float z, float maxSearchDist, uint32 phaseMask) const;

    void insert(const GameObjectModel&);
    void remove(const GameObjectModel&);
    [[nodiscard]] bool contains(const GameObjectModel&) const;
    [[nodiscard]] int size() const;

    void balance();
    void update(uint32 diff);
};

#endif
