export module GW2Viewer.UI.Controls:Model;
import GW2Viewer.Common;
import GW2Viewer.Common.FourCC;
import GW2Viewer.Common.Time;
import GW2Viewer.Common.Token64;
import GW2Viewer.Data.Archive;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Model;
import GW2Viewer.Data.Pack.PackFile;
import GW2Viewer.Services.Graphics;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.ImGui.ImGuizmo;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Notifications;
import GW2Viewer.User.ArchiveIndex;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Controls
{

struct ModelOptions
{
    bool Grid = false;
    bool Skeleton = false;
};
struct Model
{
    ModelOptions Options;

    std::unique_ptr<Data::Pack::PackFile> PackFile;
    std::unique_ptr<Data::Pack::PackFile> PackFileSkeleton;

    std::unique_ptr<Data::Model::Scene> Scene;
    std::unique_ptr<Data::Model::Viewport> Viewport;
    Data::Model::SceneObject* HoveredObject = nullptr;
    Data::Model::SceneObject* SelectedObject = nullptr;

    bool Panning = false;
    bool Rotating = false;

    bool Select = false;
    ImGuizmo::OPERATION Operation { };
    ImGuizmo::MODE Mode = ImGuizmo::MODE::LOCAL;
    bool Snap = false;
    ImVec4 TranslateSnap { 1.0f, 1.0f, 1.0f, 0.0f };
    Degrees RotateSnap = 15.0f;
    float ScaleSnap = 5.0f;

    Model(ModelOptions options = { }) : Options(std::move(options)) { }

    bool Load(uint32 fileID, Data::Archive::Kind kind = Data::Archive::Kind::Main)
    {
        // Check file type in archive index if available - faster than unpacking the entire PackFile to check its FourCC
        if (User::ArchiveIndex const& index = G::ArchiveIndex[kind]; index.IsLoaded() && index.GetFileMetadata(fileID).Type != User::ArchiveIndex::Type::Model)
            return false;

        if (PackFile = G::Game.Archive.GetPackFile(fileID); PackFile && PackFile->GetFourCC() == fcc::MODL)
            return Load(*PackFile);

        return false;
    }
    bool Load(Data::Pack::PackFile const& file)
    {
        if (file.GetFourCC() != fcc::MODL || !file.HasChunk(fcc::GEOM))
            return false;

        CreateScene();

        Data::Model::Skeleton* sceneSkeleton = nullptr;
        if (Options.Skeleton && file.HasChunk(fcc::SKEL))
        {
            auto loadSkeleton = [&](Data::Pack::Layout::Traversal::QueryChunk const& chunk)
            {
                if (auto const grannyModel = chunk["skeletonData"]["grannyModel"])
                {
                    auto const skeleton = grannyModel["Skeleton"];
                    if (!sceneSkeleton)
                        sceneSkeleton = &Scene->CreateSkeleton(std::format("{} <c=#8>{}</c>", (std::string_view)grannyModel["Name"], (std::string_view)skeleton["Name"]));
                    auto const bones = skeleton["Bones"];
                    for (auto const& bone : bones)
                    {
                        Data::Model::Bone* sceneBone = nullptr;
                        if (int32 const parentIndex = bone["ParentIndex"]; parentIndex >= 0)
                            if (auto const sceneBoneParent = sceneSkeleton->FindBoneByName(bones[parentIndex]["Name"]))
                                sceneBone = &sceneBoneParent->CreateBone(bone["Name"]);
                        if (!sceneBone)
                            sceneBone = &sceneSkeleton->CreateRootBone(bone["Name"]);

                        auto const localTransform = bone["LocalTransform"];
                        sceneBone->SetPosition(localTransform["Position"]);
                        sceneBone->SetRotationQuaternion(localTransform["Orientation"]);
                        // TODO: localTransform["ScaleShear"]
                        //sceneBone->SetTransform(bone["InverseWorld4x4"]);
                    }
                }
            };
            if (uint32 const skeletonFileID = file.QueryChunk(fcc::SKEL)["fileReference"])
                if (PackFileSkeleton = G::Game.Archive.GetPackFile(skeletonFileID); PackFileSkeleton && PackFileSkeleton->GetFourCC() == fcc::MODL)
                    loadSkeleton(PackFileSkeleton->QueryChunk(fcc::SKEL));
            loadSkeleton(file.QueryChunk(fcc::SKEL));
        }
        for (auto const mesh : file.QueryChunk(fcc::GEOM)["meshes"])
        {
            auto const geometry = mesh["geometry"];
            auto const verts = geometry["verts"];
            uint32 fileDiffuse = 0;
            uint32 fileNormal = 0;
            if (int32 const materialIndex = mesh["materialIndex"]; materialIndex >= 0)
                if (auto const material = file.QueryChunk(fcc::MODL)["permutations"][0u]["materials"][materialIndex])
                    if (auto const textures = material["textures"])
                        for (auto const texture : textures)
                            if ((Token64)texture["token"] == Token64("diffuse tex"))
                                fileDiffuse = texture["filename"];
                            else if ((Token64)texture["token"] == Token64("normal tex"))
                                fileNormal = texture["filename"];
            auto const meshName = ((Token64)mesh["meshName"]).GetString();
            std::string_view materialName = mesh["materialName"];

            auto& sceneMesh = Scene->CreateMesh(!materialName.empty() ? std::format("{} <c=#8>{}</c>", meshName.data(), materialName) : meshName.data());
            sceneMesh.LoadMesh(verts["mesh"]["fvf"],
                verts["vertexCount"],
                verts["mesh"]["vertices[]"].GetPointer(),
                geometry["indices"]["indices[]"].GetArraySize(),
                geometry["indices"]["indices[]"].GetPointer(),
                fileDiffuse,
                fileNormal);
            sceneMesh.GetProperties().Visible = !((uint32)mesh["flags"] & 4); // LOD

            if (sceneSkeleton)
            {
                std::vector<Data::Model::Bone const*> bones;
                bones.reserve(mesh["boneBindings[]"].GetArraySize());
                for (Token64 const token : mesh["boneBindings"])
                    if (!bones.emplace_back(sceneSkeleton->FindBoneByToken(token)))
                        G::Notifications.AddTimed(5s, { .Type = Notification::Types::Error, .Text = std::format("Bone with token \"{}\" not found in the skeleton file", token.GetString().data()) });
                sceneMesh.SetBones(std::move(bones));
            }
        }
        Viewport->GetCamera()->Focus(*Scene);
        return true;
    }

    Data::Model::OrbitCamera* GetCamera() const { return Viewport ? dynamic_cast<Data::Model::OrbitCamera*>(Viewport->GetCamera()) : nullptr; }

    struct DrawOptions
    {
        ImVec2 Size = I::GetContentRegionAvail();
        bool UI = false;
        std::function<void()> BarLeftPrependCallback = [] { };
        std::function<void()> BarLeftAppendCallback = [] { };
        std::function<void()> BarCenterCallback = [] { };
        std::function<void()> BarRightPrependCallback = [] { };
        std::function<void()> BarRightAppendCallback = [] { };
    };
    void Draw(DrawOptions const& options = { })
    {
        if (!Scene || !Viewport)
            return;

        scoped::Child("##ModelContainer", options.Size, ImGuiChildFlags_NavFlattened);

        auto const size = ImMax(options.Size, { 1, 1 });
        Scene->Update();
        Viewport->Resize({ (int32)size.x, (int32)size.y });
        Viewport->Render();
        auto const cursor = I::GetCursorScreenPos();
        if (auto const texture = G::Game.Texture.Get(G::UI.Textures.Transparency))
        {
            ImVec2 const texSize { (float)texture->Texture->Width, (float)texture->Texture->Height };
            for (ImVec2 pos; pos.x < size.x; pos.x += texture->Texture->Width)
                for (pos.y = 0; pos.y < size.y; pos.y += texture->Texture->Height)
                    I::GetWindowDrawList()->AddImage(texture->Texture->Handle, cursor + pos, ImMin(cursor + pos + texSize, cursor + size), { }, ImMin(ImVec2(size - pos) / texSize, { 1, 1 }));
        }
        I::Image((ImTextureID)Viewport->GetShaderResourceView(), size);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(cursor.x, cursor.y, size.x, size.y);

        if (I::IsItemHovered() && !IsMouseOverGizmo())
        {
            if (I::IsMouseDown(ImGuiMouseButton_Middle))
                Panning = true;
            if (I::IsMouseDown(ImGuiMouseButton_Right))
                Rotating = true;
        }
        if (!I::IsMouseDown(ImGuiMouseButton_Middle))
            Panning = false;
        if (!I::IsMouseDown(ImGuiMouseButton_Right))
            Rotating = false;

        if (I::IsItemHovered() || Panning || Rotating)
        {
            auto const pan = Panning ? I::GetIO().MouseDelta : ImVec2();
            auto const rotation = Rotating ? I::GetIO().MouseDelta : ImVec2();
            auto const zoom = I::GetIO().MouseWheel;
            if (pan.x || pan.y || rotation.x || rotation.y || zoom)
                Viewport->GetCamera()->HandleInput(pan, rotation, zoom);
        }

        if (HoveredObject)
            HoveredObject->GetProperties().Highlighted = false;
        if (SelectedObject)
            SelectedObject->GetProperties().Selected = true;

        if (options.UI)
        {
            if (I::IsItemHovered() && !IsMouseOverGizmo() && Select)
            {
                if ((HoveredObject = Viewport->HitTest(I::GetIO().MousePos - cursor)))
                {
                    HoveredObject->GetProperties().Highlighted = true;
                    if (I::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        if (SelectedObject)
                            SelectedObject->GetProperties().Selected = false;

                        SelectedObject = HoveredObject;
                    }
                }
                else if (I::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    if (SelectedObject)
                        SelectedObject->GetProperties().Selected = false;

                    SelectedObject = nullptr;
                }
            }

            if (SelectedObject)
            {
                std::optional<ImVec4> snap;
                /*
                if (Snap)
                {
                    switch (Operation)
                    {
                        case ImGuizmo::TRANSLATE: snap = TranslateSnap; break;
                        case ImGuizmo::ROTATE: snap = { RotateSnap, 0, 0, 0 }; break;
                        case ImGuizmo::SCALE: snap = { ScaleSnap, 0, 0, 0 }; break;
                    }
                }
                */

                auto transform = SelectedObject->GetTransform();
                auto const bone = dynamic_cast<Data::Model::Bone*>(SelectedObject);
                if (bone)
                    transform = bone->GetWorldSpaceTransform();
                if (ImGuizmo::Manipulate(*Viewport->GetCamera()->GetView().m, *Viewport->GetCamera()->GetProjection().m, Operation, Mode, *transform.m, snap ? &snap->x : nullptr))
                {
                    if (bone)
                        bone->SetWorldSpaceTransform(transform);
                    else
                        SelectedObject->SetTransform(transform);
                }
            }

            bool scenePanel = false;

            auto const oldFrameRounding = I::GetStyle().FrameRounding;
            if (scoped::WithCursorPos(0, 0))
            if (scoped::WithStyleVar(ImGuiStyleVar_FrameRounding, 0))
            if (scoped::Child("TopPanel", { -FLT_MIN, I::GetFrameHeight() + I::GetStyle().FramePadding.y * 2 + 2 }, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_Borders))
            if (scoped::WithStyleVar(ImGuiStyleVar_FrameRounding, oldFrameRounding))
            {
                I::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Horizontal;
                if (scoped::TableDockLeftRight("##Panel"))
                {
                    I::TableNextColumn();
                    options.BarLeftPrependCallback();
                    scenePanel = I::CollapsingHeader("Scene", ImGuiTreeNodeFlags_SpanLabelWidth);
                    I::Spacing();

                    I::Separator();
                    if (scoped::WithStyleVarX(ImGuiStyleVar_ItemSpacing, 0))
                    if (scoped::Font(G::UI.Fonts.DefaultLucide))
                    {
                        auto button = [this](char const* tooltip, bool select, ImGuizmo::OPERATION operation, char const* text)
                        {
                            bool checked = Select == select && Operation == operation;
                            if (I::CheckboxButton(text, checked, tooltip, I::GetFrameHeight()))
                            {
                                Select = select;
                                Operation = operation;
                            }
                        };
                        button("None",     false, { },                 ICON_LC_MOUSE_POINTER);
                        button("Select",    true, { },                 ICON_LC_SQUARE_DASHED_MOUSE_POINTER);
                        button("Translate", true, ImGuizmo::TRANSLATE, ICON_LC_MOVE_3D);
                        button("Rotate",    true, ImGuizmo::ROTATE,    ICON_LC_ROTATE_3D);
                        button("Scale",     true, ImGuizmo::SCALE,     ICON_LC_SCALE_3D);
                        button("Universal", true, ImGuizmo::UNIVERSAL, ICON_LC_AXIS_3D);
                    }

                    I::Separator();
                    I::RadioButton("World", (int*)&Mode, ImGuizmo::WORLD);
                    I::RadioButton("Local", (int*)&Mode, ImGuizmo::LOCAL);

                    /*
                    I::Separator();
                    if (scoped::Disabled(!(Operation == ImGuizmo::TRANSLATE || Operation == ImGuizmo::ROTATE || Operation == ImGuizmo::SCALE)))
                        I::Checkbox("Snap:", &Snap);
                    if (scoped::Disabled(!Snap))
                    {
                        switch (Operation)
                        {
                            case ImGuizmo::TRANSLATE: I::SetNextItemWidth(150); I::DragFloat3("##TranslateSnap", &TranslateSnap.x); break;
                            case ImGuizmo::ROTATE: I::SetNextItemWidth(80); I::DragFloat("##RotateSnap", (float*)&RotateSnap, 0.25f, 0, 360, "%.3f deg"); break;
                            case ImGuizmo::SCALE: I::SetNextItemWidth(80); I::DragFloat("##ScaleSnap", &ScaleSnap, 0.1f, 0, 10000, "%.3f%%"); break;
                        }
                    }
                    */
                    options.BarLeftAppendCallback();

                    I::TableNextColumn();
                    options.BarCenterCallback();

                    I::TableNextColumn();
                    options.BarRightPrependCallback();
                    if (scoped::Font(G::UI.Fonts.DefaultLucide))
                        if (I::Button(ICON_LC_FOCUS " Focus"))
                            Viewport->GetCamera()->Focus(SelectedObject ? *SelectedObject : *Scene, true);
                    options.BarRightAppendCallback();
                }
            }

            if (!scenePanel)
                return;

            if (scoped::WithCursorPos(0, I::GetFrameHeight() + I::GetStyle().FramePadding.y * 2 + 2))
            if (scoped::WithStyleVar(ImGuiStyleVar_FrameRounding, 0))
            if (scoped::Child(I::GetSharedScopeID("Controls:Model"), { 200, -FLT_MIN }, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
            if (scoped::WithStyleVar(ImGuiStyleVar_FrameRounding, oldFrameRounding))
            {
                if (scoped::WithStyleVarY(ImGuiStyleVar_ItemSpacing, 0))
                if (scoped::WithStyleVar(ImGuiStyleVar_CellPadding, ImVec2()))
                {
                    Viewport->Debug();
                    Scene->Debug();
                }
            }
        }
    }

private:
    bool IsMouseOverGizmo() const { return SelectedObject && ImGuizmo::IsOver(); }

    void CreateScene()
    {
        Scene = std::make_unique<Data::Model::Scene>(nullptr, "");
        Viewport = std::make_unique<Data::Model::Viewport>(Scene.get());
        if (Options.Grid)
            Scene->CreateGrid("").Initialize(10, 1.0f);
        Viewport->SetCamera(&Scene->CreateCameraOrbit(""));
    }
};

}
