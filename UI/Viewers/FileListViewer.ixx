export module GW2Viewer.UI.Viewers.FileListViewer;
import GW2Viewer.Common;
import GW2Viewer.Common.Time;
import GW2Viewer.Data.Archive;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Manifest.Asset;
import GW2Viewer.Data.Pack;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Viewers.FileViewer;
import GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.UI.Viewers.ViewerRegistry;
import GW2Viewer.UI.Windows.ContentSearch;
import GW2Viewer.UI.Windows.FileExport;
import GW2Viewer.User.ArchiveIndex;
import GW2Viewer.Utils.Async;
import GW2Viewer.Utils.Encoding;
import GW2Viewer.Utils.Format;
import GW2Viewer.Utils.Scan;
import GW2Viewer.Utils.Sort;
import GW2Viewer.Utils.Thread;
import std;
import magic_enum;
#include "Macros.h"

using GW2Viewer::Data::Archive::File;

export namespace GW2Viewer::UI::Viewers
{

struct FileListViewer : ListViewer<FileListViewer, { ICON_FA_FILE " Files", "Files", Category::ListViewer }>
{
    std::shared_mutex Lock;
    std::vector<File> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::optional<File> ScrollTo;

    std::string FilterString;
    std::optional<std::pair<int32, int32>> FilterID;
    uint32 FilterRange { };
    std::optional<User::ArchiveIndex::Type> FilterType;

    struct DrawContext
    {
        File const& File;
        Data::Archive::Archive::MftEntry const& Entry = File.GetMftEntry();
        User::ArchiveIndex const& Index = G::ArchiveIndex[File.GetSourceKind()];
        User::ArchiveIndex::CacheFile const& Cache = Index.GetFile(File.ID);
        User::ArchiveIndex::CacheMetadata const& Metadata = Index.GetMetadata(Cache.MetadataIndex);
        User::ArchiveIndex::CacheTimestamp const& AddedTimestamp = Index.GetTimestamp(Cache.AddedTimestampIndex);
        User::ArchiveIndex::CacheTimestamp const& ChangedTimestamp = Index.GetTimestamp(Cache.ChangedTimestampIndex);
        Data::Manifest::Asset const& Asset = File.GetManifestAsset();

        std::optional<Data::Archive::File> ScrollTo;
    };
    Controls::Table<File, decltype(FilteredList)&, DrawContext const&> Table { *this };

    FileListViewer(uint32 id, bool newTab) : Base(id, newTab)
    {
        static auto timestamp = [](User::ArchiveIndex::CacheTimestamp const& timestamp, std::string_view description)
        {
            I::Text("<c=#%s>%u</c>", G::Game.Build ? (timestamp.Build >= G::Game.Build ? "F00" : timestamp.Build + 100 >= G::Game.Build ? "F80" : timestamp.Build + 1000 >= G::Game.Build ? "FF0": timestamp.Build + 5000 >= G::Game.Build ? "FF8" : "F") : "F", timestamp.Build);
            if (scoped::ItemTooltip())
                I::Text(std::format("<c=#4>{}:</c>\nBuild: {}\nDate: {}", description, timestamp.Build, Utils::Format::DateTimeFullLocal(Time::FromTimestamp(timestamp.Timestamp))).c_str());
        };

        Table.SetShowFilterRow();
        Table.AddColumn("File ID",
        {
            .Width = 60,
            .NoHeaderWidth = true,
            .Filter = [](File const& file) { return file.ID; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { std::ranges::sort(data, [invert](auto a, auto b) { return a.ID < b.ID ^ invert; }); },
            .Draw = [this](DrawContext const& context)
            {
                I::SetNextItemAllowOverlap();
                I::Selectable(std::format("<c=#{}>{}</c>", context.Asset.BaseID && context.Asset.FileID && context.Asset.BaseID != context.Asset.FileID && context.File.ID == context.Asset.FileID ? "4" : context.Asset.StreamBaseID ? "8" : "F", context.File.ID).c_str(), FileViewer::Is(G::UI.GetCurrentViewer(), context.File), ImGuiSelectableFlags_SpanAllColumns);
                if (context.ScrollTo == context.File)
                    I::ScrollToItem();
                if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                    FileViewer::Open(context.File, {.MouseButton = button });
                if (scoped::PopupContextItem())
                {
                    uint64 const fileRef = (0x100 + context.File.ID / 0xFF00) << 16 | 0xFF + context.File.ID % 0xFF00;
                    Controls::CopyButton("ID", context.File.ID); I::SameLine();
                    Controls::CopyButton("FileReference", fileRef); I::SameLine();
                    Controls::CopyButton("FileReference", std::format("{:s}", std::span { (byte const*)&fileRef, 8 } | std::views::transform([](byte b) { return std::format("{:02X}", b); }) | std::views::join_with(" "sv)));

                    Controls::CopyButton("Archive Name", context.File.GetSourcePath().filename().wstring()); I::SameLine();
                    Controls::CopyButton("Archive Path", context.File.GetSourcePath().wstring());

                    Controls::CopyButton("Type", magic_enum::enum_name(context.Metadata.Type)); I::SameLine();
                    Controls::CopyButton("Metadata", context.Metadata.DataToString()); I::SameLine();
                    Controls::CopyButton("Manifest", std::wstring { std::from_range, context.Asset.ManifestNames | std::views::transform([](wchar_t const* manifestName) -> std::wstring_view { return manifestName; }) | std::views::join_with(L", "sv) }); I::SameLine();
                    Controls::CopyButton("FourCC", context.Metadata.FourCCToString());

                    Controls::CopyButton("Added Build", context.AddedTimestamp.Build); I::SameLine();
                    Controls::CopyButton("Added Timestamp", Utils::Format::DateTimeFullLocal(Time::FromTimestamp(context.AddedTimestamp.Timestamp)));

                    Controls::CopyButton("Changed Build", context.ChangedTimestamp.Build); I::SameLine();
                    Controls::CopyButton("Changed Timestamp", Utils::Format::DateTimeFullLocal(Time::FromTimestamp(context.ChangedTimestamp.Timestamp)));

                    Controls::CopyButton("Size", context.Cache.FileSize); I::SameLine();
                    Controls::CopyButton("Compressed Size", context.Entry.alloc.size); I::SameLine();
                    Controls::CopyButton("Extra Bytes", context.Entry.alloc.extraBytes); I::SameLine();
                    Controls::CopyButton("Offset", context.Entry.alloc.offset);

                    Controls::CopyButton("Flags", context.Entry.alloc.flags); I::SameLine();
                    Controls::CopyButton("Stream", context.Entry.alloc.stream); I::SameLine();
                    Controls::CopyButton("Next Stream", context.Entry.alloc.nextStream); I::SameLine();
                    Controls::CopyButton("CRC", std::format("{:08X}", context.Entry.alloc.crc));

                    I::Dummy({ 1, 10 });

                    if (auto const version = G::Game.Archive.GetFileEntry(context.Asset.BaseID); scoped::Disabled(!(version && context.Asset.BaseID && context.Asset.FileID && context.Asset.BaseID != context.Asset.FileID && context.File.ID == context.Asset.FileID)))
                        if (Controls::FileButton(context.Asset.BaseID, version, {.Icon = ICON_FA_CHEVRONS_LEFT, .Text = "Base Version", .InlinePreview = false, .TooltipPreview = false }))
                            ScrollTo = *version;
                    I::SameLine();
                    if (auto const version = G::Game.Archive.GetFileEntry(context.Asset.FileID); scoped::Disabled(!(version && context.Asset.BaseID && context.Asset.FileID && context.Asset.BaseID != context.Asset.FileID && context.File.ID == context.Asset.BaseID)))
                        if (Controls::FileButton(context.Asset.FileID, version, {.Icon = ICON_FA_CHEVRONS_RIGHT, .Text = "Latest Version", .InlinePreview = false, .TooltipPreview = false }))
                            ScrollTo = *version;

                    if (auto const version = G::Game.Archive.GetFileEntry(context.Asset.ParentBaseID); scoped::Disabled(!version))
                        if (Controls::FileButton(context.Asset.ParentBaseID, version, {.Icon = ICON_FA_ARROW_DOWN_BIG_SMALL, .Text = "Lower Quality", .TextMissingFile = "Lower Quality", .InlinePreview = false, .TooltipPreviewBestVersion = false }))
                            ScrollTo = *version;
                    I::SameLine();
                    if (auto const version = G::Game.Archive.GetFileEntry(context.Asset.StreamBaseID); scoped::Disabled(!version))
                        if (Controls::FileButton(context.Asset.StreamBaseID, version, {.Icon = ICON_FA_ARROW_UP_BIG_SMALL, .Text = "Higher Quality", .TextMissingFile = "Higher Quality", .InlinePreview = false, .TooltipPreviewBestVersion = false }))
                            ScrollTo = *version;
                    if (ScrollTo)
                        I::CloseCurrentPopup();

                    I::Dummy({ 1, 10 });

                    // TODO:: Export

                    if (I::Button("Search for Content References"))
                        G::Windows::ContentSearch.SearchForSymbolValue("FileID", context.File.ID);
                }
            },
        });
        Table.AddColumn("##Stream",
        {
            .Width = I::GetTextLineHeight(),
            .NoHeaderWidth = true,
            .NoResize = true,
            .Draw = [this](DrawContext const& context)
            {
                assert(!(context.Asset.ParentBaseID && context.Asset.StreamBaseID));
                if (auto const parent = context.File.GetSource().GetFile(context.Asset.ParentBaseID))
                {
                    I::Selectable("<c=#4>" ICON_FA_ARROW_DOWN_BIG_SMALL "</c>");
                    if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                        FileViewer::Open(ScrollTo.emplace(*parent), { .MouseButton = button });
                    if (scoped::ItemTooltip())
                        I::Text("Lower quality: %u", parent->ID);
                }
                else if (auto const stream = context.File.GetSource().GetFile(context.Asset.StreamBaseID))
                {
                    I::Selectable(ICON_FA_ARROW_UP_BIG_SMALL);
                    if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                        FileViewer::Open(ScrollTo.emplace(*stream), { .MouseButton = button });
                    if (scoped::ItemTooltip())
                        I::Text("Higher quality: %u", stream->ID);
                }
            }
        });
        Table.AddColumn("Archive",
        {
            .Width = 60,
            .NoHeaderWidth = true,
            .Filter = [](File const& file) { return file.GetSourcePath().filename().string(); },
            .Sort = [](decltype(FilteredList)& data, bool invert) { Utils::Sort::ComplexSort(data, invert, [](File const& file) { return file.GetSourceLoadOrder(); }); },
            .Draw = [this](DrawContext const& context)
            {
                I::Text("<c=#4>%s</c>", context.File.GetSourcePath().filename().string().c_str());
            },
        });
        Table.AddColumn("FourCC",
        {
            .Width = 60,
            .NoHeaderWidth = true,
            .Hide = true,
            .Filter = [](File const& file) { return G::ArchiveIndex[file.GetSourceKind()].GetFileMetadata(file.ID).FourCCToString(); },
            .Draw = [this](DrawContext const& context) { I::Text("<c=#8>%s</c>", context.Metadata.FourCCToString().c_str()); },
        });
        Table.AddColumn("Type",
        {
            .Width = -2,
            .NoHeaderWidth = true,
            .Filter = [](File const& file) { return magic_enum::enum_name(G::ArchiveIndex[file.GetSourceKind()].GetFileMetadata(file.ID).Type); },
            .Draw = [this](DrawContext const& context) { I::Text(std::format("{}", magic_enum::enum_name(context.Metadata.Type)).c_str()); },
        });
        Table.AddColumn("Metadata",
        {
            .Width = -3,
            .NoHeaderWidth = true,
            .Filter = [](File const& file) { return G::ArchiveIndex[file.GetSourceKind()].GetFileMetadata(file.ID).DataToString(); },
            .Draw = [this](DrawContext const& context) { I::TextUnformatted(context.Metadata.DataToString().c_str()); },
        });
        Table.AddColumn("Manifest",
        {
            .Width = -3,
            .NoHeaderWidth = true,
            .Filter = [](File const& file) { return file.GetManifestAsset().ManifestNames; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { Utils::Sort::ComplexSort(data, invert, [](File const& file) { return std::vector { std::from_range, file.GetManifestAsset().ManifestNames }; }); },
            .Draw = [this](DrawContext const& context)
            {
                if (!context.Asset.ManifestNames.empty())
                {
                    I::Text(context.Asset.ManifestNames.size() == 1 ? "%s" : "%s +%u", Utils::Encoding::ToUTF8(*context.Asset.ManifestNames.begin()).c_str(), (uint32)(context.Asset.ManifestNames.size() - 1));
                    if (scoped::ItemTooltip(context.Asset.ManifestNames.size() > 1 ? ImGuiHoveredFlags_DelayNone : ImGuiHoveredFlags_None))
                    {
                        I::TextUnformatted(context.Asset.ManifestNames.size() > 1 ? "<c=#4>Included in manifests:</c>" : "<c=#4>Included in manifest:</c>");
                        for (auto const& manifestName : context.Asset.ManifestNames)
                            I::TextUnformatted(Utils::Encoding::ToUTF8(manifestName).c_str());
                    }
                }
            },
        });
        Table.AddColumn("Added",
        {
            .Width = 50,
            .NoHeaderWidth = true,
            .PreferSortDescending = true,
            .Filter = [](File const& file) { return G::ArchiveIndex[file.GetSourceKind()].GetFileAddedTimestamp(file.ID).Build; },
            .Draw = [this](DrawContext const& context) { timestamp(context.AddedTimestamp, "File first scanned"); },
        });
        Table.AddColumn("##Version",
        {
            .Width = I::GetTextLineHeight(),
            .NoHeaderWidth = true,
            .NoResize = true,
            .Draw = [this](DrawContext const& context)
            {
                if (context.Asset.BaseID && context.Asset.FileID && context.Asset.BaseID != context.Asset.FileID)
                {
                    if (context.File.ID == context.Asset.FileID)
                    {
                        if (auto const base = context.File.GetSource().GetFile(context.Asset.BaseID))
                        {
                            I::Selectable(ICON_FA_CHEVRONS_LEFT);
                            if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                                FileViewer::Open(ScrollTo.emplace(*base), {.MouseButton = button });
                            if (scoped::ItemTooltip())
                                I::Text("Base version: %u", base->ID);
                        }
                    }
                    else if (context.File.ID == context.Asset.BaseID)
                    {
                        if (auto const latest = context.File.GetSource().GetFile(context.Asset.FileID))
                        {
                            I::Selectable("<c=#4>" ICON_FA_CHEVRONS_RIGHT "</c>");
                            if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                                FileViewer::Open(ScrollTo.emplace(*latest), {.MouseButton = button });
                            if (scoped::ItemTooltip())
                                I::Text("Latest version: %u", latest->ID);
                        }
                    }
                }
            },
        });
        Table.AddColumn("Changed",
        {
            .Width = 50,
            .NoHeaderWidth = true,
            .PreferSortDescending = true,
            .Filter = [](File const& file) { return G::ArchiveIndex[file.GetSourceKind()].GetFileChangedTimestamp(file.ID).Build; },
            .Draw = [this](DrawContext const& context) { timestamp(context.ChangedTimestamp, "Last time file change was scanned"); },
        });
        Table.AddColumn("Size",
        {
            .Width = 70,
            .NoHeaderWidth = true,
            .Hide = true,
            .Filter = [](File const& file) { return G::ArchiveIndex[file.GetSourceKind()].GetFile(file.ID).FileSize; },
            .Draw = [this](DrawContext const& context) { I::Text("%u", context.Cache.FileSize); },
        });
        Table.AddColumn("Compressed Size",
        {
            .Width = 70,
            .NoHeaderWidth = true,
            .Hide = true,
            .Filter = [](File const& file) { return file.GetMftEntry().alloc.size; },
            .Draw = [this](DrawContext const& context) { I::Text("%u", context.Entry.alloc.size); },
        });
        Table.AddColumn("Extra Bytes",
        {
            .Width = 20,
            .NoHeaderWidth = true,
            .Hide = true,
            .PreferSortDescending = true,
            .Filter = [](File const& file) { return file.GetMftEntry().alloc.extraBytes; },
            .Draw = [this](DrawContext const& context) { I::Text(context.Entry.alloc.extraBytes ? "%u" : "<c=#4>%u</c>", (uint32)context.Entry.alloc.extraBytes); },
        });
        Table.AddColumn("Flags",
        {
            .Width = 20,
            .NoHeaderWidth = true,
            .Hide = true,
            .PreferSortDescending = true,
            .Filter = [](File const& file) { return file.GetMftEntry().alloc.flags; },
            .Draw = [this](DrawContext const& context) { I::Text(context.Entry.alloc.flags ? "%u" : "<c=#4>%u</c>", context.Entry.alloc.flags); },
        });
        Table.AddColumn("Stream",
        {
            .Width = 20,
            .NoHeaderWidth = true,
            .Hide = true,
            .PreferSortDescending = true,
            .Filter = [](File const& file) { return file.GetMftEntry().alloc.stream; },
            .Draw = [this](DrawContext const& context) { I::Text(context.Entry.alloc.stream ? "%u" : "<c=#4>%u</c>", context.Entry.alloc.stream); },
        });
        Table.AddColumn("Next Stream",
        {
            .Width = 30,
            .NoHeaderWidth = true,
            .Hide = true,
            .PreferSortDescending = true,
            .Filter = [](File const& file) { return file.GetMftEntry().alloc.nextStream; },
            .Draw = [this](DrawContext const& context) { I::Text(context.Entry.alloc.nextStream ? "%u" : "<c=#4>%u</c>", context.Entry.alloc.nextStream); },
        });
        Table.AddColumn("CRC",
        {
            .Width = 70,
            .NoHeaderWidth = true,
            .Hide = true,
            .Filter = [](File const& file) { return file.GetMftEntry().alloc.crc; },
            .Draw = [this](DrawContext const& context) { I::Text("%08X", context.Entry.alloc.crc); },
        });
        Table.AddColumn("Refs",
        {
            .Width = 40,
            .NoHeaderWidth = true,
            .PreferSortDescending = true,
            .Filter = [](File const& file) { return G::Game.ReferencedFiles.contains(file.ID) ? 1u : 0u; },
            .Draw = [this](DrawContext const& context)
            {
                if (G::Game.ReferencedFiles.contains(context.File.ID))
                    I::TextColored({ 0, 0.5f, 1, 1 }, ICON_FA_ARROW_LEFT "EXE");
            },
        });

        UpdateSearch();
    }

    void SetResult(Utils::Async::Context context, std::vector<File>&& data)
    {
        if (context->Cancelled) return;
        std::unique_lock _(Lock);
        FilteredList = std::move(data);
        context->Finish();
    }
    void UpdateSort() override
    {
        if (std::shared_lock _(Lock); FilteredList.empty())
            return UpdateSearch();

        AsyncFilter.Run([this, table = Table.Prepare()](Utils::Async::Context context)
        {
            context->SetIndeterminate();
            std::vector<File> data;
            {
                std::shared_lock _(Lock);
                data = FilteredList;
            }
            CHECK_ASYNC;
            table.Sort(data);
            SetResult(context, std::move(data));
        });
    }
    void UpdateSearch() override
    {
        FilterID.reset();
        if (FilterString.empty())
            ;
        else if (uint32 id, range; Utils::Scan::Into(FilterString, "{}+{}", id, range))
            FilterID.emplace(id - range, id + range);
        else if (Utils::Scan::Into(FilterString, "{}-{}", id, range))
            FilterID.emplace(id, range);
        else if (Utils::Scan::NumberLiteral(FilterString, id))
            FilterID.emplace(id - FilterRange, id + FilterRange);
        else if (Utils::Scan::NumberLiteral(FilterString, "0x{:x}", id))
            FilterID.emplace(id - FilterRange, id + FilterRange);

        if (FilterID && (FilterID->first >= 0x10000FF || FilterID->second >= 0x10000FF))
        {
            Data::Pack::FileReference ref;
            *(uint64*)&ref = FilterID->first;
            FilterID->first = ref.GetFileID();
            *(uint64*)&ref = FilterID->second;
            FilterID->second = ref.GetFileID();
        }

        AsyncFilter.Run([this, filter = FilterID, type = FilterType, table = Table.Prepare()](Utils::Async::Context context) mutable
        {
            context->SetIndeterminate();
            for (auto const& index : G::ArchiveIndex)
                Utils::Thread::SleepUntil(100ms, [&] { return index.IsLoaded(); });
            auto limits = filter.value_or(std::pair { std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max() });
            limits.first = std::max(0, limits.first);
            std::vector data { std::from_range, G::Game.Archive.GetFiles() | std::views::filter([limits, &table, type](File const& file)
            {
                if (file.ID < limits.first || file.ID > limits.second)
                    return false;

                if (!table.Filter(file))
                    return false;

                if (!type)
                    return true;

                return G::ArchiveIndex[file.GetSourceKind()].GetFileMetadata(file.ID).Type == *type;
            }) };
            CHECK_ASYNC;
            table.Sort(data);
            SetResult(context, std::move(data));
        });
    }

    void Draw() override
    {
        if (scoped::TableDockRight("Search"))
        {
            I::TableNextColumn();
            if (Controls::SearchInput(FilterString, FilteredList, Lock, &AsyncFilter))
                UpdateSearch();

            I::TableNextColumn();
            if (Controls::SearchFilterRange(FilterID, FilterRange))
                UpdateSearch();
        }

        if (scoped::TableDockRight("Filter"))
        {
            I::TableNextColumn();
            I::SetNextItemWidth(-FLT_MIN);
            std::vector<std::optional<User::ArchiveIndex::Type>> values(1, std::nullopt);
            values.append_range(magic_enum::enum_values<User::ArchiveIndex::Type>());
            if (Controls::FilteredComboBox("##Type", FilterType, values,
            {
                .MaxHeight = 500,
                .Formatter = [](auto const& type) -> std::string
                {
                    if (!type)
                        return std::format("<c=#8>{} {}</c>", ICON_FA_FILTER, "Any Type");

                    return std::format("<c=#8>{}</c> {}", ICON_FA_FILTER, magic_enum::enum_name(*type));
                },
                .Filter = [](auto const& type, auto const& filter, auto const& options)
                {
                    if (!type)
                        return filter.Filters.empty();

                    return filter.PassFilter(magic_enum::enum_name(*type).data());
                },
            }))
                UpdateFilter();

            I::TableNextColumn();
            if (I::Button(ICON_FA_FLOPPY_DISK))
            {
                G::Windows::FileExport.FileIDs.assign_range(FilteredList | std::views::transform([](File const& file) { return file.ID; }));
                G::Windows::FileExport.InputFileIDs.clear();
                G::Windows::FileExport.GetShown() ^= true;
            }
            I::SetItemTooltip("File List Export");
            I::SameLine(0, 0);
            auto const viewer = dynamic_cast<FileViewer*>(G::UI.GetCurrentViewer());
            if (scoped::Disabled(!viewer))
                if (I::Button(ICON_FA_FOLDER_MAGNIFYING_GLASS))
                    ScrollTo = viewer->GetCurrent();
            I::SetItemTooltip("Locate:\n%s", viewer ? viewer->Title().c_str() : "<no file selected>");
        }

        if (scoped::TableList("Table", Table.GetColumnCount()))
        {
            Table.DrawColumnHeaders();

            std::shared_lock __(Lock);
            ImGuiListClipper clipper;
            clipper.Begin(FilteredList.size());
            auto scrollTo = std::exchange(ScrollTo, std::nullopt);
            if (scrollTo)
                if (auto const itr = std::ranges::find(FilteredList, *scrollTo); itr != FilteredList.end())
                    clipper.IncludeItemByIndex(std::distance(FilteredList.begin(), itr));
            while (clipper.Step())
                for (auto const& file : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                    if (scoped::WithID(I::GetIDWithSeed(file.ID, file.GetSourceLoadOrder())))
                        Table.DrawRow({ .File = file, .ScrollTo = scrollTo });
        }
    }
};

}
