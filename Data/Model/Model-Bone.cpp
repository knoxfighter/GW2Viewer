module;
#include <directxtk/SimpleMath.h> // Workaround for https://developercommunity.visualstudio.com/t/11029304

module GW2Viewer.Data.Model;
import :Bone;
import :Skeleton;
#include "Macros.h"

namespace GW2Viewer::Data::Model
{

Bone::Bone(Skeleton* skeleton, std::string_view name, Bone* parent) : SceneObject(skeleton->GetScene(), name), m_skeleton(skeleton), m_parent(parent), m_token(name.substr(name.find(':') + 1))
{
    UpdateBuffers();
    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(ObjectConstantBuffer { });
}

void Bone::Update()
{
    m_hierarchicalTransform = GetTransform();
    if (auto const parent = GetParent())
        m_hierarchicalTransform *= parent->GetHierarchicalTransform();

    if (!m_inverseBindPose)
        m_inverseBindPose.emplace(GetHierarchicalTransform().Invert());

    if (!m_children.empty())
    {
        Bone const* found = nullptr;
        for (auto const& bone : m_children)
            if (std::abs(bone->GetPosition().x) >= 0.1f && std::abs(bone->GetPosition().y) < 0.1f && std::abs(bone->GetPosition().z) < 0.1f && (!found || std::abs(bone->GetPosition().x) < std::abs(found->GetPosition().x)))
                found = bone.get();
        if (!found)
            found = m_children.front().get();
        m_length = found->GetPosition().x;

        if (m_bufferedLength != m_length)
        {
            m_bufferedLength = m_length;
            UpdateBuffers();
        }
    }

    for (auto const& child : m_children)
        child->Update();
}

void Bone::Render()
{
    for (auto const& child : m_children)
        child->Render();

    if (!GetProperties().Visible)
        return;

    uint32 stride = sizeof(Vertex);
    uint32 offset = 0;

    auto const context = G::Services::Graphics.Context;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.Ptr.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ObjectConstantBuffer buffer
    {
        .World = GetWorldSpaceTransform().Transpose(),
        .HighlightObject = GetProperties().Highlighted,
    };
    m_constantBuffer.Update(buffer);
    context->VSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());

    context->Draw(m_vertexBuffer.Count, 0);

    RenderSelection();
}

void Bone::Debug()
{
    I::Checkbox(std::format("{} <c=#8>{}</c>", GetName(), m_token.GetString().data()).c_str(), &GetProperties().Selected);
    if (scoped::Indent(10))
        for (auto const& child : m_children)
            child->Debug();
}

BoundingBox Bone::GetBoundingBox() const
{
    if (m_length)
    {
        auto const start = GetWorldSpacePosition();
        auto const end = start + Vector3::Transform({ m_length, 0, 0 }, GetWorldSpaceRotationQuaternion());
        return { (end - start) / 2, Vector3(std::abs(m_length)) / 2 };
    }
    return { GetWorldSpacePosition(), Vector3(GetSkeleton().GetBoneThickness()) };
}

bool Bone::HitTest(HitTestContext& context) const
{
    for (auto const& child : m_children)
        child->HitTest(context);

    if (!GetProperties().Visible || !GetProperties().HitTestable)
        return false;

    auto const transform = GetWorldSpaceTransform();
    auto const invertedTransform = transform.Invert();
    auto const localOrigin = Vector3::Transform(context.Origin, invertedTransform);
    auto localDirection = Vector3::TransformNormal(context.Direction, invertedTransform);
    localDirection.Normalize();

    auto const thickness = GetBoneThickness();
    auto const length = std::max(thickness * 2, std::abs(m_length));
    BoundingBox box;
    if (m_length)
        box = { Vector3(m_length >= 0 ? length / 2 : -length / 2, 0, 0), { length / 2, thickness, thickness } };
    else
        box = { Vector3(thickness, 0, 0), Vector3(thickness) };

    float localHitDist;
    if (!box.Intersects(localOrigin, localDirection, localHitDist))
        return false;

    if (auto const worldHitDistSquared = Vector3::DistanceSquared(context.Origin, Vector3::Transform(localOrigin + localDirection * localHitDist, transform)); worldHitDistSquared < context.Coarse.ClosestDistanceSquared)
    {
        context.Coarse.ClosestObject = (SceneObject*)this;
        context.Coarse.ClosestDistanceSquared = worldHitDistSquared;
    }

    auto localClosestDist = std::numeric_limits<float>::max();
    for (auto itr = m_vertices.begin(); itr != m_vertices.end();)
    {
        auto const& v0 = *itr++;
        auto const& v1 = *itr++;
        auto const& v2 = *itr++;
        if (DirectX::TriangleTests::Intersects(localOrigin, localDirection, v0.Position, v1.Position, v2.Position, localHitDist) && localHitDist < localClosestDist)
            localClosestDist = localHitDist;
    }

    if (localClosestDist < std::numeric_limits<float>::max())
    {
        if (auto const worldHitDistSquared = Vector3::DistanceSquared(context.Origin, Vector3::Transform(localOrigin + localDirection * localClosestDist, transform)); worldHitDistSquared < context.Fine.ClosestDistanceSquared)
        {
            context.Fine.ClosestObject = (SceneObject*)this;
            context.Fine.ClosestDistanceSquared = worldHitDistSquared;
        }
    }

    return true;
}

float Bone::GetBoneThickness() const { return GetSkeleton().GetBoneThickness(); }

void Bone::SetHierarchicalTransform(Matrix const& transform)
{
    auto localTransform = transform;
    if (auto const parent = GetParent())
        localTransform *= parent->GetHierarchicalTransform().Invert();
    SetTransform(localTransform);
}
Matrix Bone::GetWorldSpaceTransform() const { return GetHierarchicalTransform() * GetSkeleton().GetTransform(); }
Vector3 Bone::GetWorldSpacePosition() const { return Vector3::Transform(GetHierarchicalPosition(), GetSkeleton().GetTransform()); }
Quaternion Bone::GetWorldSpaceRotationQuaternion() const { return GetHierarchicalRotationQuaternion() * GetSkeleton().GetRotationQuaternion(); }
Vector3 Bone::GetWorldSpaceScale() const { return GetHierarchicalScale() * GetSkeleton().GetScale(); }
void Bone::SetWorldSpaceTransform(Matrix const& transform) { SetHierarchicalTransform(transform * GetSkeleton().GetTransform().Invert()); }

Bone* Bone::FindBoneByName(std::string_view name) const
{
    if (GetName() == name)
        return (Bone*)this;

    for (auto const& bone : m_children)
        if (auto const found = bone->FindBoneByName(name))
            return found;

    return nullptr;
}
Bone* Bone::FindBoneByToken(Token64 token) const
{
    if (m_token == token)
        return (Bone*)this;

    for (auto const& bone : m_children)
        if (auto const found = bone->FindBoneByToken(token))
            return found;

    return nullptr;
}

void Bone::UpdateBuffers()
{
    auto const t = GetBoneThickness();
    auto const l = std::max(t * 2, std::abs(m_length));
    static constexpr Vector4 color { 0.8f, 0.85f, 0.9f, 1.0f }, colorAlt { 0.7f, 0.75f, 0.8f, 1.0f };
    m_vertices =
    {
        { .Position = { 0,  0,  0 }, .Color = color },
        { .Position = { t,  t,  0 }, .Color = color },
        { .Position = { t,  0,  t }, .Color = color },

        { .Position = { t,  0,  t }, .Color = colorAlt },
        { .Position = { t,  t,  0 }, .Color = colorAlt },
        { .Position = { l,  0,  0 }, .Color = colorAlt },

        { .Position = { 0,  0,  0 }, .Color = colorAlt },
        { .Position = { t,  0,  t }, .Color = colorAlt },
        { .Position = { t, -t,  0 }, .Color = colorAlt },

        { .Position = { t, -t,  0 }, .Color = color },
        { .Position = { t,  0,  t }, .Color = color },
        { .Position = { l,  0,  0 }, .Color = color },

        { .Position = { 0,  0,  0 }, .Color = colorAlt },
        { .Position = { t,  t,  0 }, .Color = colorAlt },
        { .Position = { t,  0, -t }, .Color = colorAlt },

        { .Position = { t,  0, -t }, .Color = color },
        { .Position = { t,  t,  0 }, .Color = color },
        { .Position = { l,  0,  0 }, .Color = color },

        { .Position = { 0,  0,  0 }, .Color = color },
        { .Position = { t,  0, -t }, .Color = color },
        { .Position = { t, -t,  0 }, .Color = color },

        { .Position = { t, -t,  0 }, .Color = colorAlt },
        { .Position = { t,  0, -t }, .Color = colorAlt },
        { .Position = { l,  0,  0 }, .Color = colorAlt },
    };
    if (m_length < 0)
        for (auto& vertex : m_vertices)
            vertex.Position.x *= -1.0f;
    m_vertexBuffer = G::Services::Graphics.CreateVertexBuffer(m_vertices);

    auto const thickness = GetBoneThickness();
    CreateSelectionBuffers({ Vector3(m_length >= 0 ? l / 2 : -l / 2, 0, 0), { l / 2, thickness, thickness } });
}

}
