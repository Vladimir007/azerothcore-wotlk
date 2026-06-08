#include "wdtfile.h"

#include <cstdio>
#include <format>

#include "adtfile.h"
#include "MapDefines.h"
#include "vMapExport.h"

uMapMagic MWMOMagic = { {'O', 'M', 'W', 'M'} };
uMapMagic MODFMagic = { {'F', 'D', 'O', 'M'} };

WDTFile::WDTFile(const char* mpqFileName, const std::string& _filename) : file(mpqFileName), filename(_filename)
{
}

WDTFile::~WDTFile()
{
    file.close();
}

bool WDTFile::init(const uint32 mapID)
{
    if (file.isEof())
        return false;

    uint32 token;
    uint32 size;

    const fs::path dirname = szWorkDirWmo / DIR_BIN_FILE;
    FILE* dirFile = fopen(dirname.c_str(), "ab");
    if (!dirFile)
    {
        std::cerr << "Can't open dirFile: " << dirname.string() << std::endl;
        return false;
    }

    while (!file.isEof())
    {
        file.read(&token, 4);
        file.read(&size, 4);
        if (size == 0)
            continue;

        const std::size_t nextPos = file.getPos() + size;

        if (token == MWMOMagic.asUInt)
        {
            // Global map objects
            const auto buf = new char[size];
            file.read(buf, size);
            const char* p = buf;
            while (p < buf + size)
            {
                std::string path(p);
                std::string name;
                ExtractSingleWmo(path, name);
                _wmoNames.push_back(name);
                p += path.size() + 1;
            }
            delete[] buf;
        }
        else if (token == MODFMagic.asUInt)
        {
            // Global wmo instance data
            const uint32 mapObjectCount = size / sizeof(ADT::MODF);
            for (uint32 i = 0; i < mapObjectCount; ++i)
            {
                ADT::MODF mapObjDef;
                file.read(&mapObjDef, sizeof(ADT::MODF));
                MapObject::Extract(mapObjDef, _wmoNames[mapObjDef.Id], mapID, 65, 65, dirFile);
                Doodad::ExtractSet(WmoDoodads[_wmoNames[mapObjDef.Id]], mapObjDef, mapID, 65, 65, dirFile);
            }
        }
        file.seek(static_cast<int>(nextPos));
    }

    file.close();
    fclose(dirFile);
    return true;
}

ADTFile* WDTFile::GetMap(const int x, const int z)
{
    if (!(x >= 0 && z >= 0 && x < 64 && z < 64))
        return nullptr;
    return new ADTFile(std::format(R"(World\Maps\{0}\{0}_{1}_{2}.adt)", filename, x, z));
}
