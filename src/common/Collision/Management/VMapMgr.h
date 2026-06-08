#ifndef VMAP_MANAGER_H
#define VMAP_MANAGER_H

#include "Define.h"
#include "IVMapMgr.h"

#define MAP_FILENAME_EXTENSION ".vmtree"

namespace G3D
{
    class Vector3;
}

namespace VMAP
{
    enum DisableTypes
    {
        VMAP_DISABLE_AREA_FLAG      = 0x1,
        VMAP_DISABLE_HEIGHT         = 0x2,
        VMAP_DISABLE_LOS            = 0x4,
        VMAP_DISABLE_LIQUID_STATUS  = 0x8
    };

    class VMapMgr : public IVMapMgr
    {
    protected:
        static uint32 GetLiquidFlagsDummy(uint32) { return 0; }
        static bool IsVMAPDisabledForDummy(uint32 /*entry*/, uint8 /*flags*/) { return false; }

    public:
        static G3D::Vector3 convertPositionToInternalRep(float x, float y, float z);
        static std::string getMapFileName(uint32 mapID);

        VMapMgr();
        ~VMapMgr() override;

        LoadResult existsMap(const char* basePath, uint32 mapID, int x, int y) override;

        typedef uint32(*GetLiquidFlagsFn)(uint32 liquidType);
        GetLiquidFlagsFn GetLiquidFlagsPtr;

        typedef bool(*IsVMAPDisabledForFn)(uint32 entry, uint8 flags);
        IsVMAPDisabledForFn IsVMAPDisabledForPtr;
    };
}

#endif
