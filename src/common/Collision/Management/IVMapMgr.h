#ifndef I_VMAP_MANAGER_H
#define I_VMAP_MANAGER_H

#include "Define.h"
#include "MapDefines.h"
#include "Optional.h"

namespace VMAP
{
    class StaticMapTree;

    enum VMAP_LOAD_RESULT
    {
        VMAP_LOAD_RESULT_ERROR,
        VMAP_LOAD_RESULT_OK,
        VMAP_LOAD_RESULT_IGNORED
    };

    enum class LoadResult : uint8
    {
        Success,
        FileNotFound,
        VersionMismatch
    };

    struct AreaAndLiquidData
    {
        struct AreaInfo
        {
            AreaInfo() = default;
            AreaInfo(const int32 _groupId, const int32 _adtId, const int32 _rootId, const uint32 _mogpFlags, const uint32 _uniqueId)
                : groupId(_groupId), adtId(_adtId), rootId(_rootId), mogpFlags(_mogpFlags), uniqueId(_uniqueId) { }
            int32 groupId = 0;
            int32 adtId = 0;
            int32 rootId = 0;
            uint32 mogpFlags = 0;
            uint32 uniqueId = 0;
        };

        struct LiquidInfo
        {
            LiquidInfo() = default;
            LiquidInfo(const uint32 _type, const float _level) : type(_type), level(_level) {}
            uint32 type = 0;
            float level = 0.0f;
        };

        float                floorZ = INVALID_HEIGHT;
        Optional<AreaInfo>   areaInfo;
        Optional<LiquidInfo> liquidInfo;
    };

    class IVMapMgr
    {
    public:
        IVMapMgr()  { }
        virtual ~IVMapMgr() = default;

        virtual LoadResult existsMap(const char* pBasePath, unsigned int pMapId, int x, int y) = 0;
    };
}

#endif
