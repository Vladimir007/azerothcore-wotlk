#include "wdt.h"
#include "MapDefines.h"

uMapMagic MPHDMagic = { {'D', 'H', 'P', 'M'} };
uMapMagic MAINMagic = { {'N', 'I', 'A', 'M'} };

bool MPHD::prepareLoadedData()
{
    if (fcc != MPHDMagic.asUInt)
        return false;
    return true;
}

bool MAIN::prepareLoadedData()
{
    if (fcc != MAINMagic.asUInt)
        return false;
    return true;
}

FileWDT::FileWDT()
{
    mphd = nullptr;
    main = nullptr;
}

FileWDT::~FileWDT()
{
    FileWDT::free();
}

void FileWDT::free()
{
    mphd = nullptr;
    main = nullptr;
    FileLoader::free();
}

bool FileWDT::prepareLoadedData()
{
    if (!FileLoader::prepareLoadedData())
        return false;

    mphd = reinterpret_cast<MPHD*>(reinterpret_cast<uint8*>(version) + version->size + 8);
    if (!mphd->prepareLoadedData())
        return false;
    main = reinterpret_cast<MAIN*>(reinterpret_cast<uint8*>(mphd) + mphd->size + 8);
    if (!main->prepareLoadedData())
        return false;
    return true;
}
