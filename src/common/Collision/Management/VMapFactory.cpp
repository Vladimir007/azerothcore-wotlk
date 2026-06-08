#include "VMapFactory.h"
#include "VMapMgr.h"

namespace VMAP
{
    VMapMgr* gVMapMgr = nullptr;

    VMapMgr* VMapFactory::createOrGetVMapMgr()
    {
        if (!gVMapMgr)
            gVMapMgr = new VMapMgr();
        return gVMapMgr;
    }

    void VMapFactory::clear()
    {
        delete gVMapMgr;
        gVMapMgr = nullptr;
    }
}
