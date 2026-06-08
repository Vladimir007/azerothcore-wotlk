#ifndef NCORE_GEOMETRY_H
#define NCORE_GEOMETRY_H

#include <cstdlib>

[[nodiscard]] inline float getAngle(const float startX, const float startY, const float destX, const float destY)
{
    const auto dx = destX - startX;
    const auto dy = destY - startY;

    auto ang = std::atan2(dy, dx);
    ang = ang >= 0 ? ang : 2 * static_cast<float>(M_PI) + ang;
    return ang;
}

[[nodiscard]] inline float getSlopeAngle(const float startX, const float startY, const float startZ, const float destX, const float destY, const float destZ)
{
    const float floorDist = std::sqrt(pow(startY - destY, 2.0f) + pow(startX - destX, 2.0f));
    return atan(std::abs(destZ - startZ) / std::abs(floorDist));
}

[[nodiscard]] inline float getSlopeAngleAbs(const float startX, const float startY, const float startZ, const float destX, const float destY, const float destZ)
{
    return std::abs(getSlopeAngle(startX, startY, startZ, destX, destY, destZ));
}

[[nodiscard]] inline double getCircleAreaByRadius(const double radius)
{
    return radius * radius * M_PI;
}

[[nodiscard]] inline double getCirclePerimeterByRadius(const double radius)
{
    return radius * M_PI;
}

[[nodiscard]] inline double getCylinderVolume(const double height, const double radius)
{
    return height * getCircleAreaByRadius(radius);
}

#endif
