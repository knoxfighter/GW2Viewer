export module GW2Viewer.UI.Windows.FileExport;
import GW2Viewer.Common;
import GW2Viewer.Data.Game;
import GW2Viewer.Services.Export;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Windows.Window;
import GW2Viewer.UI.Windows.Settings;
import GW2Viewer.Utils.Async;
import GW2Viewer.Utils.Scan;
import std;
#include "Macros.h"

namespace GW2Viewer::UI::Windows
{

struct FileExport : Window
{
    Utils::Async::Scheduler Async;
    std::string InputFileIDs;
    std::vector<uint32> FileIDs;

    std::string Title() override { return "File Export"; }
    void Draw() override
    {
        if (auto context = Async.Current())
        {
            if (I::Button(ICON_FA_FLOPPY_DISK " Stop", { -I::GetFrameHeight() - I::GetStyle().ItemSpacing.x, 0 }))
                Async.Run([](Utils::Async::Context context) { context->Finish(); });
        }
        else if (I::Button(std::format(ICON_FA_FLOPPY_DISK " Export {} Files", FileIDs.size()).c_str(), { -I::GetFrameHeight() - I::GetStyle().ItemSpacing.x, 0 }) && !FileIDs.empty())
        {
            Async.Run([fileIDs = FileIDs](Utils::Async::Context context)
            {
                context->SetTotal(fileIDs.size());
                for (auto fileID : fileIDs)
                {
                    CHECK_ASYNC;
                    G::Services::Export.File(fileID, { .BlockUntilExported = true });
                    context->Increment();
                }

                context->Finish();
            });
        }
        I::SameLine();
        G::Windows::Settings.GetSection("Export").DrawPopupButton();
        if (auto context = Async.Current())
        {
            I::SetNextItemWidth(-FLT_MIN);
            if (scoped::Disabled(true))
                I::InputText("##Description", (char*)std::format("{} / {}", context.Current, context.Total).c_str(), 9999);
            Controls::AsyncProgressBar(Async);
        }
        else
        {
            I::SetNextItemWidth(-FLT_MIN);
            if (scoped::Disabled(true))
                I::InputText("##Description", (char*)"", 9999);
        }
        I::TextUnformatted("File IDs (one per row):");
        if (I::InputTextEx("#FileIDs", FileIDs.empty() ? "" : "<all filtered files from file list viewer>", &InputFileIDs, { -FLT_MIN, -FLT_MIN }, ImGuiInputTextFlags_Multiline))
            FileIDs.assign_range(InputFileIDs | std::views::split("\n"sv) | std::views::transform([](auto id) { return *Utils::Scan::Single<uint32>(std::string(std::from_range, id)); }) | std::views::filter(std::identity()));
    }
};

}

export namespace GW2Viewer::G::Windows { UI::Windows::FileExport FileExport; }
