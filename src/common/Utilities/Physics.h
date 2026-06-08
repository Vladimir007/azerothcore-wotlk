#ifndef NCORE_PHYSICS_H
#define NCORE_PHYSICS_H

#include "Geometry.h"

using namespace std;

[[nodiscard]] inline float getWeight(const float height, const float width, const float specificWeight)
{
    return getCylinderVolume(height, width / 2.0f) * specificWeight;
}

/// Get the height immersed in water
[[nodiscard]] inline float getOutOfWater(const float width, const float weight, const float density)
{
    return weight / (getCircleAreaByRadius(width / 2.0f) * density);
}

#endif
