#ifndef TILE_ASSEMBLER_H
#define TILE_ASSEMBLER_H

#include <map>
#include <set>
#include <G3D/Matrix3.h>
#include <G3D/Vector3.h>

#include "ModelInstance.h"
#include "WorldModel.h"

namespace VMAP
{
    class ModelPosition
    {
        G3D::Matrix3 iRotation;
    public:
        ModelPosition() { }
        G3D::Vector3 iPos;
        G3D::Vector3 iDir;
        float iScale{0.0f};
        void init()
        {
            iRotation = G3D::Matrix3::fromEulerAnglesZYX(G3D::pif() * iDir.y / 180.f, G3D::pif() * iDir.x / 180.f, G3D::pif() * iDir.z / 180.f);
        }
        [[nodiscard]] G3D::Vector3 transform(const G3D::Vector3& pIn) const;
        void moveToBasePos(const G3D::Vector3& pBasePos) { iPos -= pBasePos; }
    };

    typedef std::map<uint32, ModelSpawn> UniqueEntryMap;
    typedef std::multimap<uint32, uint32> TileMap;

    struct MapSpawns
    {
        UniqueEntryMap UniqueEntries;
        TileMap TileEntries;
    };

    typedef std::map<uint32, MapSpawns*> MapData;

    struct GroupModel_Raw
    {
        uint32 mogpFlags{0};
        uint32 GroupWMOid{0};

        G3D::AABox bounds;
        uint32 liquidFlags{0};
        std::vector<MeshTriangle> triangles;
        std::vector<G3D::Vector3> vertexArray;
        WmoLiquid* liquid;

        GroupModel_Raw() : liquid(nullptr) { }
        ~GroupModel_Raw();

        bool Read(FILE* rf);
    };

    struct WorldModel_Raw
    {
        uint32 RootWMOid;
        std::vector<GroupModel_Raw> groupsArray;

        bool Read(const char* path);
    };

    class TileAssembler
    {
        std::string iDestDir;
        std::string iSrcDir;
        G3D::Table<std::string, unsigned int > iUniqueNameIds;
        MapData mapData;
        std::set<std::string> spawnedModelFiles;

    public:
        TileAssembler(const std::string& pSrcDirName, const std::string& pDestDirName);
        virtual ~TileAssembler();

        bool convertWorld();
        bool readMapSpawns();
        bool calculateTransformedBound(ModelSpawn& spawn);
        void exportGameobjectModels();
        bool convertRawFile(const std::string& pModelFilename);
    };

}

#endif
