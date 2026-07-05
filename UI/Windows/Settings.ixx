export module GW2Viewer.UI.Windows.Settings;
import GW2Viewer.Common;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Windows.Window;
import GW2Viewer.User.Config;
import GW2Viewer.Utils.ConstString;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Windows
{

struct Settings : Window
{
    bool Accepted = false;

    Settings()
    {
        AddSection({
            .Name = "Paths",
            .Category = Category::General,
            .Order = std::numeric_limits<int32>::min(),
            .Draw = [this]
            {
                auto path = [](char const* label, std::string& value)
                {
                    if (scoped::WithStyleVarY(ImGuiStyleVar_ItemSpacing, 0))
                        I::TextUnformatted(label);
                    I::SetNextItemWidth(-FLT_MIN);
                    I::InputText(std::format("##{}", label).c_str(), &value);
                };
                path("<c=#8>Gw2-64.exe</c> <c=#F00>(required)</c>", G::Config.GameExePath);
                path("<c=#8>Gw2.dat</c> <c=#F00>(required)</c>", G::Config.GameDatPath);
                path("<c=#8>Local.dat</c> <c=#F00>(required)</c>", G::Config.LocalDatPath);
                path("<c=#8>External Database (.sqlite)</c>", G::Config.DecryptionKeysPath);
                if (Accepted)
                {
                    if (scoped::WithTextWrapPos(I::GetContentRegionAvail().x))
                        I::TextUnformatted("<c=#4>Application needs to be restarted after changing any of these paths for the changes to take effect</c>");
                }
                else if (scoped::Disabled(G::Config.GameExePath.empty() || G::Config.GameDatPath.empty() || G::Config.LocalDatPath.empty()))
                {
                    if (I::Button("OK"))
                    {
                        Accepted = true;
                        Hide();
                    }
                }
            }
        });
    }

    std::string Title() override { return "Settings"; }
    void Draw() override
    {
        if (!SelectedSection)
            SelectedSection = &*GetSectionRegistry().Sections.begin();

        if (scoped::Child("Sections", { 0, -FLT_MIN }, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX | ImGuiChildFlags_AutoResizeX, ImGuiWindowFlags_NoSavedSettings))
        {
            auto category = Category::General;
            for (auto const& section : GetSectionRegistry().Sections)
            {
                if (category != section.Category)
                {
                    category = section.Category;
                    I::Separator();
                }
                if (I::Selectable(section.Name.c_str(), SelectedSection == &section))
                    SelectedSection = &section;
            }
        }
        I::SameLine(0, 0);
        if (scoped::Child("Section", { -FLT_MIN, -FLT_MIN }, ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar))
            SelectedSection->Draw();
    }

    enum class Category
    {
        General,
        Controls,
        Viewers,
    };
    struct Section
    {
        std::string Name;
        Category Category;
        std::optional<int32> Order;
        std::function<void()> Draw;

        void DrawPopupButton() const
        {
            if (I::Button(ICON_FA_GEAR))
                I::OpenPopup("SettingsPopup");

            I::SetNextWindowPos(I::LastRect().GetBR(), ImGuiCond_Always, { 1, 0 });
            if (scoped::Popup("SettingsPopup"))
                Draw();
        }

        auto operator<=>(Section const& other) const
        {
            if (auto const result = Category <=> other.Category; result != std::strong_ordering::equal) return result;
            if (auto const result = Order.value_or(0) <=> other.Order.value_or(0); result != std::strong_ordering::equal) return result;
            if (auto const result = Name <=> other.Name; result != std::strong_ordering::equal) return result;
            return std::strong_ordering::equal;
        }
    };
    auto const& AddSection(Section info) { return *GetSectionRegistry().Sections.emplace(std::move(info)).first; }

private:
    Section const* SelectedSection = nullptr;
    struct SectionRegistry
    {
        std::set<Section> Sections;
        Section const& Add(Section info) { return *Sections.emplace(std::move(info)).first; }
    };
    SectionRegistry& GetSectionRegistry()
    {
        static SectionRegistry instance;
        return instance;
    }
};

}

export namespace GW2Viewer::G::Windows { UI::Windows::Settings Settings; }
