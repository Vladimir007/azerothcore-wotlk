#ifndef WORLD_MODEL_STORE_H
#define WORLD_MODEL_STORE_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include "WorldModel.h"

class WorldModelStore
{
public:
    static WorldModelStore* instance()
    {
        static WorldModelStore instance;
        return &instance;
    }

    std::shared_ptr<VMAP::WorldModel> AcquireModelInstance(const std::string& basepath, const std::string& filename, uint32 flags);

private:
    typedef std::unordered_map<std::string, std::shared_ptr<VMAP::WorldModel>> ModelFileMap;
    ModelFileMap _loadedModels;

    std::mutex _lock;
};

#define sWorldModelStore WorldModelStore::instance()

#endif
