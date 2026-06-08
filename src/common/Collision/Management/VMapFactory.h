#ifndef VMAP_FACTORY_H
#define VMAP_FACTORY_H

namespace VMAP
{
    class VMapMgr;

    class VMapFactory
    {
    public:
        static VMapMgr* createOrGetVMapMgr();
        static void clear();
    };
}
#endif
