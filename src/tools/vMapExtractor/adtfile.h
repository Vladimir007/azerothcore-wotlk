#ifndef ADT_H
#define ADT_H

#include "model.h"
#include "wmo.h"
#include "G3D/AABox.h"
#include "G3D/Vector3.h"

#pragma pack(push, 1)
namespace ADT
{
    struct MDDF
    {
        uint32 Id;
        uint32 UniqueId;
        G3D::Vector3 Position;
        G3D::Vector3 Rotation;
        uint16 Scale;
        uint16 Flags;
    };

    struct MODF
    {
        uint32 Id;
        uint32 UniqueId;
        G3D::Vector3 Position;
        G3D::Vector3 Rotation;
        G3D::AABox Bounds;
        uint16 Flags;
        uint16 DoodadSet;
        uint16 NameSet;
        uint16 Scale;
    };
}
#pragma pack(pop)

class ADTFile
{
    MPQFile file;
public:
    explicit ADTFile(const std::string& _filename);
    ~ADTFile();
    bool init(uint32 map_num, uint32 tileX, uint32 tileY);

    std::vector<std::string> WmoInstanceNames;
    std::vector<std::string> ModelInstanceNames;
};

#endif
