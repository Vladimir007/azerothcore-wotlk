#include "adt.h"
#include "MapDefines.h"

uMapMagic MHDRMagic = { { 'R', 'D', 'H', 'M' } };
uMapMagic MCINMagic = { { 'N', 'I', 'C', 'M' } };
uMapMagic MH2OMagic = { { 'O', '2', 'H', 'M' } };
uMapMagic MCNKMagic = { { 'K', 'N', 'C', 'M' } };
uMapMagic MCVTMagic = { { 'T', 'V', 'C', 'M' } };
uMapMagic MCLQMagic = { { 'Q', 'L', 'C', 'M' } };
uMapMagic MFBOMagic = { { 'O', 'B', 'F', 'M' } };

FileADT::FileADT()
{
    grid = nullptr;
}

FileADT::~FileADT()
{
    FileADT::free();
}

void FileADT::free()
{
    grid = nullptr;
    FileLoader::free();
}

bool FileADT::prepareLoadedData()
{
    // Check parent
    if (!FileLoader::prepareLoadedData())
        return false;

    // Check and prepare MHDR
    grid = reinterpret_cast<MHDR*>(GetData() + 8 + version->size);
    if (!grid->prepareLoadedData())
        return false;

    return true;
}

bool MHDR::prepareLoadedData()
{
    if (fcc != MHDRMagic.asUInt)
        return false;

    if (size != sizeof(MHDR) - 8)
        return false;

    // Check and prepare MCIN
    if (offsMCIN && !getMCIN()->prepareLoadedData())
        return false;

    // Check and prepare MH2O
    if (offsMH2O && !getMH2O()->prepareLoadedData())
        return false;

    if (offsMFBO && flags & 1 && !getMFBO()->prepareLoadedData())
        return false;

    return true;
}

bool MCIN::prepareLoadedData()
{
    if (fcc != MCINMagic.asUInt)
        return false;

    // Check cells data
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
            if (cells[i][j].offsMCNK && !getMCNK(i, j)->prepareLoadedData())
                return false;

    return true;
}

bool MH2O::prepareLoadedData()
{
    if (fcc != MH2OMagic.asUInt)
        return false;
    return true;
}

bool MCNK::prepareLoadedData()
{
    if (fcc != MCNKMagic.asUInt)
        return false;

    // Check height map
    if (offsMCVT && !getMCVT()->prepareLoadedData())
        return false;

    // Check liquid data
    if (offsMCLQ && !getMCLQ()->prepareLoadedData())
        return false;

    return true;
}

bool MCVT::prepareLoadedData()
{
    if (fcc != MCVTMagic.asUInt)
        return false;
    if (size != sizeof(MCVT) - 8)
        return false;
    return true;
}

bool MCLQ::prepareLoadedData()
{
    if (fcc != MCLQMagic.asUInt)
        return false;
    return true;
}

bool MFBO::prepareLoadedData()
{
    return fcc == MFBOMagic.asUInt;
}
