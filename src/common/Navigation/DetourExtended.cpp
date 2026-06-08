#include "DetourExtended.h"
#include "DetourCommon.h"
#include "Geometry.h"

float dtQueryFilterExt::getCost(const float* pa, const float* pb,
                const dtPolyRef /*prevRef*/, const dtMeshTile* /*prevTile*/, const dtPoly* /*prevPoly*/,
                const dtPolyRef /*curRef*/, const dtMeshTile* /*curTile*/, const dtPoly* curPoly,
                const dtPolyRef /*nextRef*/, const dtMeshTile* /*nextTile*/, const dtPoly* /*nextPoly*/) const
{
    const float startX = pa[2];
    const float startY = pa[0];
    const float startZ = pa[1];
    const float destX = pb[2];
    const float destY = pb[0];
    const float destZ = pb[1];
    const float slopeAngle = getSlopeAngle(startX, startY, startZ, destX, destY, destZ);
    const float slopeAngleDegree = slopeAngle * 180.0f / M_PI;
    const float cost = slopeAngleDegree > 0 ? 1.0f + (1.0f * (slopeAngleDegree / 100)) : 1.0f;
    const float dist = dtVdist(pa, pb);
    const auto totalCost = dist * cost * getAreaCost(curPoly->getArea());
    return totalCost;
}
