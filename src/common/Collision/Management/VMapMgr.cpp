#include "VMapMgr.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <G3D/Vector3.h>

#include "MapDefines.h"
#include "MapTree.h"

using G3D::Vector3;

namespace VMAP
{
    VMapMgr::VMapMgr()
    {
        GetLiquidFlagsPtr = &GetLiquidFlagsDummy;
        IsVMAPDisabledForPtr = &IsVMAPDisabledForDummy;
    }

    VMapMgr::~VMapMgr()
    {
    }

    Vector3 VMapMgr::convertPositionToInternalRep(const float x, const float y, const float z)
    {
        Vector3 pos;
        constexpr float mid = 0.5f * MAX_NUMBER_OF_GRIDS * SIZE_OF_GRIDS;
        pos.x = mid - x;
        pos.y = mid - y;
        pos.z = z;

        return pos;
    }

    std::string VMapMgr::getMapFileName(const uint32 mapID)
    {
        std::stringstream fName;
        fName.width(3);
        fName << std::setfill('0') << mapID << std::string(MAP_FILENAME_EXTENSION);
        return fName.str();
    }

    LoadResult VMapMgr::existsMap(const char* basePath, const uint32 mapID, const int x, const int y)
    {
        return StaticMapTree::CanLoadMap(std::string(basePath), mapID, x, y);
    }

}
