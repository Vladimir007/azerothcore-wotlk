#ifndef NCORE_DETOUR_EXTENDED_H
#define NCORE_DETOUR_EXTENDED_H

#include "DetourNavMeshQuery.h"

class dtQueryFilterExt: public dtQueryFilter
{
public:
    float getCost(const float* pa, const float* pb,
        dtPolyRef prevRef, const dtMeshTile* prevTile, const dtPoly* prevPoly,
        dtPolyRef curRef, const dtMeshTile* curTile, const dtPoly* curPoly,
        dtPolyRef nextRef, const dtMeshTile* nextTile, const dtPoly* nextPoly) const override;
};

#endif
