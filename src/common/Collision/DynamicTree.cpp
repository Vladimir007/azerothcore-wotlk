#include "DynamicTree.h"

#include <functional>
#include <G3D/AABox.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>

#include "BoundingIntervalHierarchyWrapper.h"
#include "GameObjectModel.h"
#include "MapTree.h"
#include "ModelIgnoreFlags.h"
#include "ModelInstance.h"
#include "RegularGrid.h"
#include "Timer.h"
#include "VMapFactory.h"
#include "VMapMgr.h"
#include "WorldModel.h"

using VMAP::ModelInstance;

namespace
{
    int CHECK_TREE_PERIOD = 200;
}

template<> struct HashTrait<GameObjectModel>
{
    static std::size_t hashCode(const GameObjectModel& g)
    {
        return std::hash<const GameObjectModel*>{}(&g);
    }
};

template<> struct PositionTrait<GameObjectModel>
{
    static void GetPosition(const GameObjectModel& g, G3D::Vector3& p) { p = g.GetPosition(); }
};

template<> struct BoundsTrait<GameObjectModel>
{
    static void GetBounds(const GameObjectModel* g, G3D::AABox& out) { out = g->GetBounds();}
};

typedef RegularGrid2D<GameObjectModel, BIHWrap<GameObjectModel>> ParentTree;

struct DynTreeImpl : ParentTree
{
    typedef GameObjectModel Model;
    typedef ParentTree base;

    DynTreeImpl() :
        rebalance_timer(CHECK_TREE_PERIOD),
        unbalanced_times(0)
    {
    }

    void insert(const Model& mdl)
    {
        base::insert(mdl);
        ++unbalanced_times;
    }

    void remove(const Model& mdl)
    {
        base::remove(mdl);
        ++unbalanced_times;
    }

    void balance()
    {
        base::balance();
        unbalanced_times = 0;
    }

    void update(const uint32 diffTime)
    {
        if (!size())
            return;

        rebalance_timer.Update(diffTime);
        if (!rebalance_timer.Passed())
            return;

        rebalance_timer.Reset(CHECK_TREE_PERIOD);
        if (unbalanced_times > 0)
            balance();
    }

    TimeTrackerSmall rebalance_timer;
    int unbalanced_times;
};

DynamicMapTree::DynamicMapTree() : impl(new DynTreeImpl()) { }

DynamicMapTree::~DynamicMapTree()
{
    delete impl;
}

void DynamicMapTree::insert(const GameObjectModel& mdl)
{
    impl->insert(mdl);
}

void DynamicMapTree::remove(const GameObjectModel& mdl)
{
    impl->remove(mdl);
}

bool DynamicMapTree::contains(const GameObjectModel& mdl) const
{
    return impl->contains(mdl);
}

void DynamicMapTree::balance()
{
    impl->balance();
}

int DynamicMapTree::size() const
{
    return impl->size();
}

void DynamicMapTree::update(const uint32 diff)
{
    impl->update(diff);
}

struct DynamicTreeIntersectionCallback
{
    DynamicTreeIntersectionCallback(const uint32 phaseMask, const VMAP::ModelIgnoreFlags ignoreFlags) :
        _phaseMask(phaseMask), _ignoreFlags(ignoreFlags) { }

    bool operator()(const G3D::Ray& r, const GameObjectModel& obj, float& distance, const bool stopAtFirstHit)
    {
        return obj.intersectRay(r, distance, stopAtFirstHit, _phaseMask, _ignoreFlags);
    }

private:
    uint32 _phaseMask;
    VMAP::ModelIgnoreFlags _ignoreFlags;
};

struct DynamicTreeLocationInfoCallback
{
    explicit DynamicTreeLocationInfoCallback(const uint32 phaseMask) : _phaseMask(phaseMask), _hitModel(nullptr) {}

    bool operator()(G3D::Vector3 const& p, GameObjectModel const& obj)
    {
        if (obj.GetLocationInfo(p, _locationInfo, _phaseMask))
        {
            _hitModel = &obj;
            return true;
        }
        return false;
    }

    VMAP::LocationInfo& GetLocationInfo()
    {
        return _locationInfo;
    }

    GameObjectModel const* GetHitModel() const
    {
        return _hitModel;
    }

private:
    uint32                 _phaseMask;
    VMAP::LocationInfo     _locationInfo;
    const GameObjectModel* _hitModel;
};

bool DynamicMapTree::GetIntersectionTime(const uint32 phaseMask, const G3D::Ray& ray, const G3D::Vector3& endPos, float& maxDist) const
{
    float distance = maxDist;
    DynamicTreeIntersectionCallback callback(phaseMask, VMAP::ModelIgnoreFlags::Nothing);
    const auto didHit = impl->intersectRay(ray, callback, distance, endPos, false);
    if (didHit)
        maxDist = distance;
    return didHit;
}

bool DynamicMapTree::GetObjectHitPos(const uint32 phaseMask, const G3D::Vector3& startPos,
                                     const G3D::Vector3& endPos, G3D::Vector3& resultHit,
                                     const float modifyDist) const
{
    bool result = false;
    const float maxDist = (endPos - startPos).magnitude();
    // Valid map coords should *never ever* produce float overflow, but this would produce NaNs too
    ASSERT(maxDist < std::numeric_limits<float>::max());
    // Prevent NaN values which can cause BIH intersection to enter infinite loop
    if (maxDist < 1e-10f)
    {
        resultHit = endPos;
        return false;
    }
    const G3D::Vector3 dir = (endPos - startPos) / maxDist;  // Direction with length of 1
    const G3D::Ray ray(startPos, dir);
    float dist = maxDist;
    if (GetIntersectionTime(phaseMask, ray, endPos, dist))
    {
        resultHit = startPos + dir * dist;
        if (modifyDist < 0)
        {
            if ((resultHit - startPos).magnitude() > -modifyDist)
                resultHit = resultHit + dir * modifyDist;
            else
                resultHit = startPos;
        }
        else
            resultHit = resultHit + dir * modifyDist;

        result = true;
    }
    else
    {
        resultHit = endPos;
        result = false;
    }
    return result;
}

bool DynamicMapTree::isInLineOfSight(const float x1, const float y1, const float z1, const float x2, const float y2, const float z2,
    const uint32 phaseMask, const VMAP::ModelIgnoreFlags ignoreFlags) const
{
    const G3D::Vector3 v1(x1, y1, z1);
    const G3D::Vector3 v2(x2, y2, z2);

    float maxDist = (v2 - v1).magnitude();

    if (!G3D::fuzzyGt(maxDist, 0))
        return true;

    const G3D::Ray r(v1, (v2 - v1) / maxDist);
    DynamicTreeIntersectionCallback callback(phaseMask, ignoreFlags);
    return !impl->intersectRay(r, callback, maxDist, v2, true);
}

float DynamicMapTree::getHeight(const float x, const float y, const float z, float maxSearchDist, const uint32 phaseMask) const
{
    const G3D::Vector3 v(x, y, z);
    const G3D::Ray r(v, G3D::Vector3(0, 0, -1));
    DynamicTreeIntersectionCallback callback(phaseMask, VMAP::ModelIgnoreFlags::Nothing);
    if (impl->intersectZAlignedRay(r, callback, maxSearchDist))
        return v.z - maxSearchDist;
    return -G3D::finf();
}

bool DynamicMapTree::GetAreaAndLiquidData(const float x, const float y, const float z, const uint32 phaseMask,
    const Optional<uint8> reqLiquidType, VMAP::AreaAndLiquidData& data) const
{
    const G3D::Vector3 v(x, y, z + 0.5f);
    DynamicTreeLocationInfoCallback intersectionCallBack(phaseMask);
    impl->intersectPoint(v, intersectionCallBack);
    if (intersectionCallBack.GetLocationInfo().hitModel)
    {
        data.floorZ = intersectionCallBack.GetLocationInfo().ground_Z;
        uint32 liquidType = intersectionCallBack.GetLocationInfo().hitModel->GetLiquidType();
        float liquidLevel;
        if (!reqLiquidType || VMAP::VMapFactory::createOrGetVMapMgr()->GetLiquidFlagsPtr(liquidType) & *reqLiquidType)
            if (intersectionCallBack.GetHitModel()->GetLiquidLevel(v, intersectionCallBack.GetLocationInfo(), liquidLevel))
                data.liquidInfo.emplace(liquidType, liquidLevel);

        data.areaInfo.emplace(intersectionCallBack.GetLocationInfo().hitModel->GetWmoID(),
            0,
            intersectionCallBack.GetLocationInfo().rootId,
            intersectionCallBack.GetLocationInfo().hitModel->GetMogpFlags(),
            0);
        return true;
    }
    return false;
}
