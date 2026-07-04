module GW2Viewer.Data.Model;
import :Common;
import :Material;

namespace GW2Viewer::Data::Model
{

void SceneObject::Debug()
{
    if (auto value = GetPosition(); I::DragFloat3("Position", &value.x)) SetPosition(value);
    if (auto value = GetRotation(); I::DragFloat3("Rotation", &value.x, 0.01f)) SetRotation(value);
    if (auto value = GetScale(); I::DragFloat3("Scale", &value.x, 0.01f)) SetScale(value);
    if (auto value = GetScale(); I::DragFloat("Uniform", &value.x, 0.01f)) SetScale(value.x);
}

void SceneObject::CreateSelectionBuffers(BoundingBox const& localBox)
{
    Vector3 corners[8];
    localBox.GetCorners(corners);
    constexpr auto length = 0.2f;
    std::vector<Vertex> selectionVertices
    {
        { .Position = corners[0] },
        { .Position = DirectX::XMVectorLerp(corners[0], corners[1], length) },
        { .Position = DirectX::XMVectorLerp(corners[0], corners[1], 1.0f - length) },
        { .Position = corners[1] },
        { .Position = DirectX::XMVectorLerp(corners[1], corners[2], length) },
        { .Position = DirectX::XMVectorLerp(corners[1], corners[2], 1.0f - length) },
        { .Position = corners[2] },
        { .Position = DirectX::XMVectorLerp(corners[2], corners[3], length) },
        { .Position = DirectX::XMVectorLerp(corners[2], corners[3], 1.0f - length) },
        { .Position = corners[3] },
        { .Position = DirectX::XMVectorLerp(corners[3], corners[0], length) },
        { .Position = DirectX::XMVectorLerp(corners[3], corners[0], 1.0f - length) },

        { .Position = DirectX::XMVectorLerp(corners[0], corners[4], length) },
        { .Position = DirectX::XMVectorLerp(corners[1], corners[5], length) },
        { .Position = DirectX::XMVectorLerp(corners[2], corners[6], length) },
        { .Position = DirectX::XMVectorLerp(corners[3], corners[7], length) },

        { .Position = DirectX::XMVectorLerp(corners[0], corners[4], 1.0f - length) },
        { .Position = DirectX::XMVectorLerp(corners[1], corners[5], 1.0f - length) },
        { .Position = DirectX::XMVectorLerp(corners[2], corners[6], 1.0f - length) },
        { .Position = DirectX::XMVectorLerp(corners[3], corners[7], 1.0f - length) },

        { .Position = corners[4] },
        { .Position = DirectX::XMVectorLerp(corners[4], corners[5], length) },
        { .Position = DirectX::XMVectorLerp(corners[4], corners[5], 1.0f - length) },
        { .Position = corners[5] },
        { .Position = DirectX::XMVectorLerp(corners[5], corners[6], length) },
        { .Position = DirectX::XMVectorLerp(corners[5], corners[6], 1.0f - length) },
        { .Position = corners[6] },
        { .Position = DirectX::XMVectorLerp(corners[6], corners[7], length) },
        { .Position = DirectX::XMVectorLerp(corners[6], corners[7], 1.0f - length) },
        { .Position = corners[7] },
        { .Position = DirectX::XMVectorLerp(corners[7], corners[4], length) },
        { .Position = DirectX::XMVectorLerp(corners[7], corners[4], 1.0f - length) },
    };
    for (auto& vertex : selectionVertices)
        vertex.Color = Vector4 { 0.5f, 0.5f, 0.5f, 1.0f };
    std::vector<uint16> selectionIndices
    {
        0, 1,  2, 3,  3, 4,  5, 6,  6, 7,  8, 9, 9, 10,  11, 0,
        0, 12,  3, 13,  6, 14,  9, 15,
        16, 20,  17, 23,  18, 26,  19, 29,
        20, 21,  22, 23,  23, 24,  25, 26,  26, 27,  28, 29,  29, 30,  31, 20,
    };

    m_selectionVertexBuffer = G::Services::Graphics.CreateVertexBuffer(selectionVertices);
    m_selectionIndexBuffer = G::Services::Graphics.CreateIndexBuffer(selectionIndices);
}

bool SceneObject::RenderSelection()
{
    if (!GetProperties().Selected)
        return false;

    GetScene()->SetDebugShapes(true);
    uint32 stride = sizeof(Vertex);
    uint32 offset = 0;

    auto context = G::Services::Graphics.Context;
    context->IASetVertexBuffers(0, 1, m_selectionVertexBuffer.Ptr.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_selectionIndexBuffer.Ptr.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    context->DrawIndexed(m_selectionIndexBuffer.Count, 0, 0);
    GetScene()->SetDebugShapes(false);
    return true;
}

}
