#ifndef MODEL_IGNORE_FLAGS_H
#define MODEL_IGNORE_FLAGS_H

#include "Define.h"

namespace VMAP
{
    enum class ModelIgnoreFlags : uint32
    {
        Nothing = 0x00,
        M2      = 0x01
    };

    inline ModelIgnoreFlags operator&(ModelIgnoreFlags left, ModelIgnoreFlags right)
    {
        return static_cast<ModelIgnoreFlags>(static_cast<uint32>(left) & static_cast<uint32>(right));
    }
}

#endif
