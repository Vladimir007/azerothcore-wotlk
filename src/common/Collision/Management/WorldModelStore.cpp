#include "WorldModelStore.h"
#include "Log.h"

std::shared_ptr<VMAP::WorldModel> WorldModelStore::AcquireModelInstance(std::string const& basepath, std::string const& filename, const uint32 flags)
{
    //! Critical section, thread safe access
    std::lock_guard lock(_lock);

    auto model = _loadedModels.find(filename);
    if (model == _loadedModels.end())
    {
        auto worldModel = std::make_shared<VMAP::WorldModel>();
        LOG_DEBUG("maps", "WorldModelStore: loading file '{}{}'", basepath, filename);
        if (!worldModel->readFile(basepath + filename + ".vmo"))
        {
            LOG_ERROR("maps", "WorldModelStore: could not load '{}{}.vmo'", basepath, filename);
            return nullptr;
        }

        worldModel->Flags = flags;
        model = _loadedModels.insert(std::pair(filename, worldModel)).first;
    }

    return model->second;
}
