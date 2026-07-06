export module GW2Viewer.Services.Export;
import GW2Viewer.Common;
import GW2Viewer.Data.Archive;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Texture;
import GW2Viewer.User.Config;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Windows.Settings;
import std;
#include "Macros.h"

namespace GW2Viewer::Services
{

static auto& Config = G::Config.Services.Export;
inline static auto& SettingsSection = G::Windows::Settings.AddSection({
    .Name = "Export",
    .Category = UI::Windows::Settings::Category::General,
    .Draw = []
    {
        I::TextUnformatted("Export converted:");
        I::Checkbox("Textures (.png)", &Config.ConvertTexture);
        I::Checkbox("Sounds (.mp3/.ogg)", &Config.ConvertSound);
        I::AlignTextToFramePadding();
        I::TextUnformatted("Export raw files:");
        I::SameLine();
        I::SetNextItemWidth(-FLT_MIN);
        std::map<std::pair<bool, bool>, char const*> items
        {
            { { false, false }, "Never" },
            { { false, true }, "If not converted" },
            { { true, false }, "Always" },
        };
        auto const itr = items.find({ Config.ExportRawAlways, Config.ExportRawIfNotConverted });
        if (scoped::Combo("##ExportRaw", itr != items.end() ? itr->second : ""))
            for (auto const& [value, name] : items)
                if (I::Selectable(name, value == std::tie(Config.ExportRawAlways, Config.ExportRawIfNotConverted)))
                    std::tie(Config.ExportRawAlways, Config.ExportRawIfNotConverted) = value;

        I::Checkbox("Skip already existing files", &Config.SkipExisting);
    }
});

export
{

struct Export
{
    struct ExportOptions
    {
        std::span<byte const> DataSource;
        std::filesystem::path Path;
        bool Convert = true;
        bool BlockUntilExported = false; // Make sure the call comes from an async thread when using this
    };
    bool File(uint32 fileID, ExportOptions const& options = { })
    {
        auto const file = G::Game.Archive.GetFileEntry(fileID);
        return file && File(*file, options);
    }
    bool File(Data::Archive::File const& file, ExportOptions const& options = { })
    {
        std::filesystem::path const path = options.Path.empty() ? std::format(R"(Export\{})", file.ID) : options.Path;
        // ReSharper disable once CppEntityAssignedButNoRead
        std::vector<byte> data;
        std::span<byte const> dataSource;
        if (!options.DataSource.empty())
            dataSource = options.DataSource;
        else
            dataSource = data = G::Game.Archive.GetFile(file.ID);

        auto result = false;
        if (options.Convert)
        {
            if (Config.ConvertTexture)
            {
                auto texturePath = path;
                texturePath.replace_extension(".png");
                if (!Config.SkipExisting || !(result |= exists(texturePath)))
                    result |= G::Game.Texture.Load(file.ID, { .DataSource = dataSource, .ExportPath = texturePath, .BlockUntilLoaded = options.BlockUntilExported })->TextureLoadingState == Data::Texture::TextureEntry::TextureLoadingStates::Loaded;
            }
            if (Config.ConvertSound)
                result |= G::Game.Audio.PlayFile(file.ID, { .DataSource = dataSource, .Play = false, .Export = true, .ExportSkipExisting = Config.SkipExisting });
        }

        if (!options.Convert || Config.ExportRawAlways || Config.ExportRawIfNotConverted && !result)
            if (!Config.SkipExisting || !exists(path))
                result |= Data(dataSource, path);

        return result;
    }
    bool Data(std::span<byte const> data, std::filesystem::path const& path)
    {
        create_directories(path.parent_path());
        std::ofstream(path, std::ios::binary).write((char const*)data.data(), data.size());
        return true;
    }
};

}

}

export namespace GW2Viewer::G::Services { GW2Viewer::Services::Export Export; }
