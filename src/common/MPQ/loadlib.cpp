#include "loadlib.h"
#include <cstdio>

#include "MapDefines.h"
#include "MPQ.h"

class MPQFile;

uMapMagic MverMagic = { {'R', 'E', 'V', 'M'} };

FileLoader::FileLoader()
{
    data = nullptr;
    size = 0;
    version = nullptr;
}

FileLoader::~FileLoader()
{
    FileLoader::free();
}

bool FileLoader::loadFile(const std::string& filename, const bool log)
{
    free();
    MPQFile mf(filename.c_str());
    if (mf.isEof())
    {
        if (log)
            printf("No such file %s\n", filename.c_str());
        return false;
    }

    size = mf.getSize();

    data = new uint8 [size];
    mf.read(data, size);
    mf.close();
    if (prepareLoadedData())
        return true;

    printf("Error loading %s", filename.c_str());
    mf.close();
    free();
    return false;
}

bool FileLoader::prepareLoadedData()
{
    // Check version
    version = reinterpret_cast<MVER*>(data);
    if (version->fcc != MverMagic.asUInt)
        return false;
    if (version->ver != FILE_FORMAT_VERSION)
        return false;
    return true;
}

void FileLoader::free()
{
    delete[] data;
    data = nullptr;
    size = 0;
    version = nullptr;
}
