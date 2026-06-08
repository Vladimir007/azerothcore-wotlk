#include <cstdio>
#include <filesystem>

#include "adtfile.h"
#include "dbcfile.h"
#include "DBCStorage.h"
#include "model.h"
#include "vMapExport.h"

bool ExtractSingleModel(const std::string& filename, std::string& outName)
{
    if (filename.empty())
        return false;

    fs::path filepath(filename);
    filepath.replace_extension(".m2");

    outName = GetPlainName(filepath.string());
    const std::string output = szWorkDirWmo / outName;
    if (fs::exists(output))
        return true;

    std::string filepathStr(filepath);
    Model mdl(filepathStr);
    if (!mdl.open())
        return false;

    return mdl.ConvertToVMAPModel(output);
}

void ExtractGameObjectModels()
{
    std::cout << "Extracting GameObject models..." << std::endl;

    DBCStorage<GameObjectDisplayInfoEntry> modelStorage;
    if (!modelStorage.Load("dbc_game_object_display_info", "id, model_name", "id"))
        Abort("Fatal error: Can't load GameObjectDisplayInfo.dbc entries!");

    const std::string modelListPath = szWorkDirWmo / TMP_GAME_OBJECT_MODELS;
    FILE* file = fopen(modelListPath.c_str(), "wb");
    if (!file)
        Abort(std::format("Fatal error: Could not open file for write '{}'", modelListPath));

    fwrite(VMAP::RAW_VMAP_MAGIC, 1, 8, file);

    for (const auto entry : modelStorage)
    {
        fs::path modelPath = entry->Name;
        std::string extension = modelPath.extension();
        std::ranges::transform(extension, extension.begin(), [](const unsigned char c) { return std::tolower(c); });
        if (extension == ".mdl")
            // TODO: extract .mdl files, if needed
            continue;

        uint8 isWmo = extension == ".wmo" ? 1 : 0;
        std::string name;

        bool result = false;
        if (isWmo)
            result = ExtractSingleWmo(modelPath, name);
        else
            result = ExtractSingleModel(modelPath, name);

        if (result)
        {
            const uint32 displayId = entry->ID;
            uint32 nameSize = name.size();

            fwrite(&displayId, sizeof(uint32), 1, file);
            fwrite(&isWmo, sizeof(uint8), 1, file);
            fwrite(&nameSize, sizeof(uint32), 1, file);
            fwrite(name.c_str(), sizeof(char), nameSize, file);
        }
    }

    fclose(file);

    std::cout << "Done!" << std::endl;
}
