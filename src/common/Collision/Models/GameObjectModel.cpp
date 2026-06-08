#include "GameObjectModel.h"

#include "Log.h"
#include "MapTree.h"
#include "ModelInstance.h"
#include "Timer.h"
#include "VMapDefinitions.h"
#include "WorldModel.h"
#include "WorldModelStore.h"

using G3D::Vector3;
using G3D::Ray;
using G3D::AABox;

struct GameobjectModelData
{
    GameobjectModelData(const char* name_, const uint32 nameLength, const Vector3& lowBound, const Vector3& highBound, const bool _isWmo) :
        bound(lowBound, highBound), name(name_, nameLength), isWmo(_isWmo) { }

    AABox bound;
    std::string name;
    bool isWmo;
};

typedef std::unordered_map<uint32, GameobjectModelData> ModelList;
ModelList model_list;

void LoadGameObjectModelList(std::string const& dataPath)
{
    const uint32 oldMSTime = getMSTime();

    FILE* model_list_file = fopen((dataPath + "vMaps/" + VMAP::GAMEOBJECT_MODELS).c_str(), "rb");
    if (!model_list_file)
    {
        LOG_ERROR("maps", "Unable to open '{}' file.", VMAP::GAMEOBJECT_MODELS);
        return;
    }

    char magic[8];
    if (fread(magic, 1, 8, model_list_file) != 8 || memcmp(magic, VMAP::VMAP_MAGIC, 8) != 0)
    {
        LOG_ERROR("maps", "File '{}' has wrong header, expected {}.", VMAP::GAMEOBJECT_MODELS, VMAP::VMAP_MAGIC);
        fclose(model_list_file);
        return;
    }

    uint32 name_length, displayId;
    uint8 isWmo;
    char buff[500];
    while (true)
    {
        Vector3 v1, v2;
        if (fread(&displayId, sizeof(uint32), 1, model_list_file) != 1)
            if (feof(model_list_file))  // EOF flag is only set after failed reading attempt
                break;

        if (fread(&isWmo, sizeof(uint8), 1, model_list_file) != 1 ||
            fread(&name_length, sizeof(uint32), 1, model_list_file) != 1 ||
            name_length >= sizeof(buff) ||
            fread(&buff, sizeof(char), name_length, model_list_file) != name_length ||
            fread(&v1, sizeof(Vector3), 1, model_list_file) != 1 ||
            fread(&v2, sizeof(Vector3), 1, model_list_file) != 1)
        {
            LOG_ERROR("maps", "File '{}' seems to be corrupted!", VMAP::GAMEOBJECT_MODELS);
            fclose(model_list_file);
            break;
        }

        if (v1.isNaN() || v2.isNaN())
        {
            LOG_ERROR("maps", "File '{}' Model '{}' has invalid v1{} v2{} values!",
                VMAP::GAMEOBJECT_MODELS, std::string(buff, name_length), v1.toString(), v2.toString());
            continue;
        }

        model_list.emplace(std::piecewise_construct,
            std::forward_as_tuple(displayId),
            std::forward_as_tuple(&buff[0], name_length, v1, v2, isWmo != 0));
    }

    fclose(model_list_file);

    LOG_INFO("server.loading", ">> Loaded {} GameObject Models in {} ms", static_cast<uint32>(model_list.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool GameObjectModel::initialize(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, const std::string& dataPath)
{
    const ModelList::const_iterator it = model_list.find(modelOwner->GetDisplayId());
    if (it == model_list.end())
        return false;

    AABox mdl_box(it->second.bound);

    // Ignore models with no bounds
    if (mdl_box == AABox::zero())
    {
        LOG_ERROR("maps", "GameObject model {} has zero bounds, loading skipped", it->second.name);
        return false;
    }

    iModel = sWorldModelStore->AcquireModelInstance(dataPath + "vMaps/", it->second.name,
        it->second.isWmo ? VMAP::ModelFlags::MOD_WORLD_SPAWN : VMAP::ModelFlags::MOD_M2);

    if (!iModel)
        return false;

    name = it->second.name;
    iPos = modelOwner->GetPosition();
    phaseMask = modelOwner->GetPhaseMask();
    iScale = modelOwner->GetScale();
    iInvScale = 1.f / iScale;

    const G3D::Matrix3 iRotation = G3D::Matrix3::fromEulerAnglesZYX(modelOwner->GetOrientation(), 0, 0);
    iInvRot = iRotation.inverse();

    // Transform bounding box
    mdl_box = AABox(mdl_box.low() * iScale, mdl_box.high() * iScale);
    AABox rotated_bounds;
    for (int i = 0; i < 8; ++i)
        rotated_bounds.merge(iRotation * mdl_box.corner(i));

    iBound = rotated_bounds + iPos;
    owner = std::move(modelOwner);
    isWmo = it->second.isWmo;
    return true;
}

GameObjectModel* GameObjectModel::Create(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, const std::string& dataPath)
{
    const auto mdl = new GameObjectModel();
    if (!mdl->initialize(std::move(modelOwner), dataPath))
    {
        delete mdl;
        return nullptr;
    }

    return mdl;
}

bool GameObjectModel::intersectRay(const Ray& ray, float& MaxDist, const bool StopAtFirstHit, const uint32 phMask, const VMAP::ModelIgnoreFlags ignoreFlags) const
{
    if (!(phaseMask & phMask) || !owner->IsSpawned())
        return false;

    if (const float time = ray.intersectionTime(iBound); time == G3D::inf())
        return false;

    // Child bounds are defined in object space
    const Vector3 p = iInvRot * (ray.origin() - iPos) * iInvScale;
    const Ray modRay(p, iInvRot * ray.direction());
    float distance = MaxDist * iInvScale;
    const bool hit = iModel->IntersectRay(modRay, distance, StopAtFirstHit, ignoreFlags);
    if (hit)
        MaxDist = distance * iScale;
    return hit;
}

bool GameObjectModel::GetLocationInfo(const Vector3& point, VMAP::LocationInfo& info, const uint32 phMask) const
{
    if (!(phaseMask & phMask) || !owner->IsSpawned() || !IsMapObject())
        return false;

    if (!iBound.contains(point))
        return false;

    // Child bounds are defined in object space
    const Vector3 pModel = iInvRot * (point - iPos) * iInvScale;
    const Vector3 zDirModel = iInvRot * Vector3(0.f, 0.f, -1.f);
    float zDist;

    VMAP::GroupLocationInfo groupInfo;
    if (!iModel->GetLocationInfo(pModel, zDirModel, zDist, groupInfo))
        return false;

    const Vector3 modelGround = pModel + zDist * zDirModel;
    const float world_Z = (modelGround * iInvRot * iScale + iPos).z;
    if (info.ground_Z >= world_Z)
        return false;
    info.ground_Z = world_Z;
    return true;
}

bool GameObjectModel::GetLiquidLevel(const Vector3& point, const VMAP::LocationInfo& info, float& liqHeight) const
{
    // Child bounds are defined in object space
    const Vector3 pModel = iInvRot * (point - iPos) * iInvScale;
    float zDist;
    if (!info.hitModel->GetLiquidLevel(pModel, zDist))
        return false;

    // Calculate world height (zDist in model coords).
    // Assume WMO not tilted (wouldn't make much sense anyway)
    liqHeight = zDist * iScale + iPos.z;
    return true;
}

bool GameObjectModel::UpdatePosition()
{
    if (!iModel)
        return false;

    const ModelList::const_iterator it = model_list.find(owner->GetDisplayId());
    if (it == model_list.end())
        return false;

    AABox mdl_box(it->second.bound);

    // Ignore models with no bounds
    if (mdl_box == AABox::zero())
        return false;

    iPos = owner->GetPosition();
    const G3D::Matrix3 iRotation = G3D::Matrix3::fromEulerAnglesZYX(owner->GetOrientation(), 0, 0);
    iInvRot = iRotation.inverse();

    // Transform bounding box:
    mdl_box = AABox(mdl_box.low() * iScale, mdl_box.high() * iScale);
    AABox rotated_bounds;

    for (int i = 0; i < 8; ++i)
        rotated_bounds.merge(iRotation * mdl_box.corner(i));

    iBound = rotated_bounds + iPos;
    return true;
}
