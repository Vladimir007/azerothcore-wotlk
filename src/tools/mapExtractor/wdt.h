#ifndef WDT_H
#define WDT_H

#include "loadlib.h"

#define WDT_MAP_SIZE 64

struct MPHD
{
    uint32 fcc;
    uint32 size;
    uint32 data[8];

    bool prepareLoadedData();
};

struct MAIN
{
    uint32 fcc;
    uint32 size;

    struct adtData
    {
        uint32 exist;
        uint32 data1;
    } adt_list[64][64];

    bool prepareLoadedData();
};

class FileWDT : public FileLoader
{
public:
    bool   prepareLoadedData() override;

    FileWDT();
    ~FileWDT() override;
    void free() override;

    MPHD* mphd;
    MAIN* main;
};

#endif
