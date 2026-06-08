#include "adtfile.h"
#include <cstdio>

#include "MapDefines.h"
#include "vMapExport.h"

uMapMagic MMDXMagic = { { 'X', 'D', 'M', 'M' } };
uMapMagic MWMOMagic = { { 'O', 'M', 'W', 'M' } };
uMapMagic MDDFMagic = { { 'F', 'D', 'D', 'M' } };
uMapMagic MODFMagic = { { 'F', 'D', 'O', 'M' } };

ADTFile::ADTFile(const std::string& _filename) : file(_filename.c_str())
{
}

ADTFile::~ADTFile()
{
    file.close();
}

bool ADTFile::init(uint32 map_num, uint32 tileX, uint32 tileY)
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

        if (token == MMDXMagic.asUInt)
        {
            const auto buf = new char[size];
            file.read(buf, size);
            const char* p = buf;
            while (p < buf + size)
            {
                std::string path(p);
                std::string name;
                ExtractSingleModel(path, name);
                ModelInstanceNames.emplace_back(name);
                p += path.size() + 1;
            }
            delete[] buf;
        }
        else if (token == MWMOMagic.asUInt)
        {
            const auto buf = new char[size];
            file.read(buf, size);
            const char* p = buf;
            while (p < buf + size)
            {
                std::string path(p);
                std::string name;
                ExtractSingleWmo(path, name);
                WmoInstanceNames.emplace_back(name);
                p += path.size() + 1;
            }
            delete[] buf;
        }
        else if (token == MDDFMagic.asUInt)
        {
            const uint32 doodadCount = size / sizeof(ADT::MDDF);
            for (uint32 i = 0; i < doodadCount; ++i)
            {
                ADT::MDDF doodadDef;
                file.read(&doodadDef, sizeof(ADT::MDDF));
                Doodad::Extract(doodadDef, ModelInstanceNames[doodadDef.Id], map_num, tileX, tileY, dirFile);
            }
        }
        else if (token == MODFMagic.asUInt)
        {
            const uint32 mapObjectCount = size / sizeof(ADT::MODF);
            for (uint32 i = 0; i < mapObjectCount; ++i)
            {
                ADT::MODF mapObjDef;
                file.read(&mapObjDef, sizeof(ADT::MODF));
                MapObject::Extract(mapObjDef, WmoInstanceNames[mapObjDef.Id], map_num, tileX, tileY, dirFile);
                Doodad::ExtractSet(WmoDoodads[WmoInstanceNames[mapObjDef.Id]], mapObjDef, map_num, tileX, tileY, dirFile);
            }
        }
        file.seek(static_cast<int>(nextPos));
    }
    file.close();
    fclose(dirFile);
    return true;
}
