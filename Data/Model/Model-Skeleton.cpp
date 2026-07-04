module GW2Viewer.Data.Model;
import :Bone;
import :Material;
import :Skeleton;

namespace GW2Viewer::Data::Model
{

Skeleton::Skeleton(Scene* scene, std::string_view name): SceneObject(scene, name) { }
Skeleton::~Skeleton() = default;

void Skeleton::Update()
{
    for (auto const& bone : m_rootBones)
        bone->Update();
}

void Skeleton::Render()
{
    GetScene()->SetDebugShapes(true);
    for (auto const& bone : m_rootBones)
        bone->Render();
    GetScene()->SetDebugShapes(false);
}

void Skeleton::Debug()
{
    SceneObject::Debug();
    for (auto const& bone : m_rootBones)
        bone->Debug();
}

BoundingBox Skeleton::GetBoundingBox() const
{
    std::optional<BoundingBox> box;
    for (auto const& bone : m_rootBones)
    {
        if (box)
            BoundingBox::CreateMerged(*box, *box, bone->GetBoundingBox());
        else
            box = bone->GetBoundingBox();
    }

    return box.value_or({ Vector3::Zero, Vector3::One });
}

bool Skeleton::HitTest(HitTestContext& context) const
{
    for (auto const& bone : m_rootBones)
        bone->HitTest(context);

    return context.Coarse.ClosestObject;
}

Bone& Skeleton::CreateRootBone(std::string_view name)
{
    return *m_rootBones.emplace_back(std::make_unique<Bone>(this, name, nullptr));
}

Bone* Skeleton::FindBoneByName(std::string_view name) const
{
    for (auto const& bone : m_rootBones)
        if (auto const found = bone->FindBoneByName(name))
            return found;

    return nullptr;
}
Bone* Skeleton::FindBoneByToken(Token64 token) const
{
    for (auto const& bone : m_rootBones)
        if (auto const found = bone->FindBoneByToken(token))
            return found;

    return nullptr;
}

}
