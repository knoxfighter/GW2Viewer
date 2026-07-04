module GW2Viewer.Data.Model;
import :Grid;

namespace GW2Viewer::Data::Model
{

void Grid::Initialize(uint32 size, float spacing)
{
    m_size = size;
    m_spacing = spacing;

    static constexpr Vector4 color { 0.3f, 0.3f, 0.3f, 1.0f };
    static constexpr Vector4 colorX { 1.0f, 0.0f, 0.0f, 1.0f };
    static constexpr Vector4 colorY { 0.0f, 1.0f, 0.0f, 1.0f };
    static constexpr Vector4 negative { 0.25f, 0.25f, 0.25f, 1.0f };
    static constexpr Vector4 positive { 1.0f, 1.0f, 1.0f, 1.0f };

    float const halfSize = size * spacing;
    std::vector<Vertex> vertices;
    vertices.reserve(4 * (size + 1 + size));
    for (int32 i = -(int32)size; i <= (int32)size; ++i)
    {
        float const pos = i * spacing;
        vertices.push_back({ .Position = { -halfSize, pos, 0.0f }, .Color = i ? color : colorX * negative });
        vertices.push_back({ .Position = {  halfSize, pos, 0.0f }, .Color = i ? color : colorX * positive });
        vertices.push_back({ .Position = { pos, -halfSize, 0.0f }, .Color = i ? color : colorY * negative });
        vertices.push_back({ .Position = { pos,  halfSize, 0.0f }, .Color = i ? color : colorY * positive });
    }
    m_vertexBuffer = G::Services::Graphics.CreateVertexBuffer(vertices);
    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(ObjectConstantBuffer { });
}

void Grid::Render()
{
    if (!m_visible)
        return;

    GetScene()->SetDebugShapes(true);
    uint32 stride = sizeof(Vertex);
    uint32 offset = 0;

    auto const context = G::Services::Graphics.Context;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.Ptr.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    context->VSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());

    context->Draw(m_vertexBuffer.Count, 0);
    GetScene()->SetDebugShapes(false);
}

void Grid::Debug()
{
    I::Checkbox("Visible", &m_visible);
    if (I::DragInt("Size", (int*)&m_size, 0.1f, 1, 10000))
        Initialize(m_size, m_spacing);
    if (I::DragFloat("Spacing", &m_spacing, 0.1f, 0.1f, 10000))
        Initialize(m_size, m_spacing);
}

}
