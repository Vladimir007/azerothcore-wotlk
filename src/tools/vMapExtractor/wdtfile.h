#ifndef WDT_FILE_H
#define WDT_FILE_H

#include <string>
#include "wmo.h"

class ADTFile;

class WDTFile
{
public:
    WDTFile(const char* mpqFileName, const std::string& _filename);
    ~WDTFile();

    bool init(uint32 mapID);
    ADTFile* GetMap(int x, int z);

    std::vector<std::string> _wmoNames;

private:
    MPQFile file;
    std::string filename;
};

#endif
