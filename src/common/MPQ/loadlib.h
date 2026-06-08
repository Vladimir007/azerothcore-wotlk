#ifndef NCORE_MPQ_LOAD_LIB_H
#define NCORE_MPQ_LOAD_LIB_H

#include <string>
#include "Define.h"

constexpr auto FILE_FORMAT_VERSION = 18;

/// File version chunk
struct MVER
{
    uint32 fcc;
    uint32 size;
    uint32 ver;
};

class FileLoader
{
    uint8*  data;
    uint32  size;
public:
    MVER* version;

    FileLoader();
    virtual ~FileLoader();
    virtual bool prepareLoadedData();
    virtual void free();

    uint8* GetData() { return data; }
    uint32 GetSize() { return size; }
    bool loadFile(std::string const& filename, bool log = true);

};

#endif
