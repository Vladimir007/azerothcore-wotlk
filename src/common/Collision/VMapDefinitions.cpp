#include "VMapDefinitions.h"
#include <boost/filesystem.hpp>

bool VMAP::readChunk(FILE* rf, char* dest, const char* compare, const uint32 len)
{
    if (fread(dest, sizeof(char), len, rf) != len) { return false; }
    return memcmp(dest, compare, len) == 0;
}
