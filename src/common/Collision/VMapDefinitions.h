#ifndef VMAP_DEFINITIONS_H
#define VMAP_DEFINITIONS_H

#include "Define.h"

#define LIQUID_TILE_SIZE (533.333f / 128.f)

namespace VMAP
{
    constexpr char VMAP_MAGIC[] = "VMAP_4.8";
    constexpr char RAW_VMAP_MAGIC[] = "VMAP048";  // Used in extracted vmap files with raw data
    constexpr char GAMEOBJECT_MODELS[] = "GameObjectModels.dtree";

    bool readChunk(FILE* rf, char* dest, const char* compare, uint32 len);
}
#endif
