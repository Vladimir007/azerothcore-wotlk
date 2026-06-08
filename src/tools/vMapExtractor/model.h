#ifndef MODEL_H
#define MODEL_H

#include <filesystem>
#include <G3D/Vector3.h>
#include "ModelHeader.h"

namespace fs = std::filesystem;

class MPQFile;
struct WMODoodadData;
namespace ADT { struct MDDF; struct MODF; }

G3D::Vector3 fixCoordSystem(const G3D::Vector3& v);

class Model
{
    void _unload()
    {
        delete[] vertices;
        delete[] indices;
        vertices = nullptr;
        indices = nullptr;
    }
    std::string filename;
public:
    explicit Model(std::string& filename);
    ~Model() { _unload(); }

    bool open();
    bool ConvertToVMAPModel(const std::string& outFileName);

    ModelHeader header;
    G3D::Vector3* vertices;
    uint16* indices;
};

namespace Doodad
{
    void Extract(const ADT::MDDF& doodadDef, const std::string& modelInstName, uint32 mapID, uint32 tileX, uint32 tileY, FILE* pDirFile);
    void ExtractSet(const WMODoodadData& doodadData, const ADT::MODF& wmo, uint32 mapID, uint32 tileX, uint32 tileY, FILE* pDirFile);
}

#endif
