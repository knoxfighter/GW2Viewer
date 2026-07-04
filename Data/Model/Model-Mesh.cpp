module GW2Viewer.Data.Model;
import :Material;
import :Mesh;
import :Scene;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Texture;
import magic_enum;
#include "Macros.h"

namespace GW2Viewer::Data::Model
{

struct FVFInfo
{
    uint32 Size = 0;
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout;
    struct
    {
        int32 Position = -1;
        int32 PositionCompressed = -1;
        int32 BlendWeights = -1;
        int32 BlendIndices = -1;
        int32 Normal = -1;
        int32 NormalCompressed = -1;
        int32 Tangent = -1;
        int32 TangentCompressed = -1;
        int32 Bitangent = -1;
        int32 BitangentCompressed = -1;
        int32 TangentFrame = -1;
        int32 Color = -1;
        std::array<int32, 8> TexCoord16 { -1, -1, -1, -1, -1, -1, -1, -1 };
        std::array<int32, 8> TexCoord32 { -1, -1, -1, -1, -1, -1, -1, -1 };
    } Offset;
};

void Mesh::Render()
{
    if (!GetProperties().Visible)
        return;

    ID3D11ShaderResourceView* textures[] { GetScene()->GetDummyTextureDiffuse(), GetScene()->GetDummyTextureNormal() };
    if (auto const texture = G::Game.Texture.Get(m_fileDiffuse))
        textures[0] = (ID3D11ShaderResourceView*)texture->Texture->Handle.GetTexID();
    if (auto const texture = G::Game.Texture.Get(m_fileNormal))
        textures[1] = (ID3D11ShaderResourceView*)texture->Texture->Handle.GetTexID();

    uint32 stride = sizeof(Vertex);
    uint32 offset = 0;

    auto context = G::Services::Graphics.Context;
    //context->IASetInputLayout(m_inputLayout.Get());
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.Ptr.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Ptr.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->PSSetShaderResources(0, 2, textures);

    ObjectConstantBuffer buffer
    {
        .World = GetTransform().Transpose(),
        .HighlightObject = GetProperties().Highlighted || m_debugHighlightHovered,
    };
    for (auto const [index, bone] : m_bones | std::views::enumerate)
        if (bone)
            buffer.BoneTransforms[index] = (bone->GetInverseBindPose() * bone->GetHierarchicalTransform()).Transpose();
    m_constantBuffer.Update(buffer);
    context->VSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());

    context->DrawIndexed(m_indexBuffer.Count, 0, 0);

    RenderSelection();
}

void Mesh::Debug()
{
    scoped::WithID(this);
    I::TableNextRow();

    I::TableNextColumn();
    bool const open = I::CollapsingHeader("##Fold", ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAllColumns);
    m_debugHighlightHovered = I::IsItemHovered();

    I::TableNextColumn();
    I::Checkbox("##Visible", &GetProperties().Visible);
    I::SameLine(0, 0);
    I::TextUnformatted(GetName().c_str());

    if (open)
    {
        I::TableNextRow();
        I::TableNextColumn();
        I::TableNextColumn();

        if (scoped::TabBar("Tabs"))
        {
            if (scoped::TabItem("Transform"))
                SceneObject::Debug();
            if (scoped::TabItem("Properties"))
            {
                auto& properties = GetProperties();
                I::Checkbox("Visible", &properties.Visible);
                I::Checkbox("Highlighted", &properties.Highlighted);
                I::Checkbox("Selected", &properties.Selected);
                I::Checkbox("Hit Testable", &properties.HitTestable);
            }
            if (scoped::TabItem("Vertex Data"))
            {
                if (scoped::Table("Table", 1 + m_fvf->Layout.size(), ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_HighlightHoveredColumn))
                {
                    I::TableSetupColumn("#");
                    for (auto const& element : m_fvf->Layout)
                        I::TableSetupColumn(element.SemanticName == "TEXCOORD"sv || element.SemanticName == "UNKONE"sv ? std::format("{}{}", element.SemanticName, element.SemanticIndex).c_str() : element.SemanticName);
                    I::TableSetupScrollFreeze(1, 1);
                    I::TableHeadersRow();

                    auto inputs = [](auto const& value, uint32 components, char const* format = nullptr)
                    {
                        auto data = (byte*)&value;
                        scoped::Group();
                        for (auto i = 0; i < components; ++i)
                        {
                            scoped::WithID(i);
                            if (i)
                                I::SameLine(0, 0);
                            I::SetNextItemWidth(50);
                            static constexpr uint32 colors[] { 0xFFCCCCFF, 0xFFCCFFCC, 0xFFFFCCCC, 0xFFFFCCFF };
                            if (scoped::WithColorVar(ImGuiCol_Text, components > 1 ? colors[i] : I::GetColorU32(ImGuiCol_Text)))
                                I::InputScalar("", ImGuiDataType_Float, data, nullptr, nullptr, format, ImGuiInputTextFlags_ReadOnly);
                            data += sizeof(float);
                        }
                    };

                    auto p = m_vertexData.data();
                    for (auto i = 0; i < m_vertices.size(); ++i)
                    {
                        I::TableNextRow();
                        scoped::WithID(i);
                        I::TableNextColumn();
                        I::Text("<c=#8>%u</c>", i);
                        for (auto const& element : m_fvf->Layout)
                        {
                            I::TableNextColumn();
                            scoped::WithID(p);
                            switch (element.Format)
                            {
                                case DXGI_FORMAT_R32G32B32A32_FLOAT: inputs(*(Vector4*)p, 4); p += sizeof(float) * 4; break;
                                case DXGI_FORMAT_R32G32B32_FLOAT: inputs(*(Vector3*)p, 3); p += sizeof(float) * 3; break;
                                case DXGI_FORMAT_R32G32_FLOAT: inputs(*(Vector2*)p, 2); p += sizeof(float) * 2; break;
                                case DXGI_FORMAT_R32_FLOAT: inputs(*(float*)p, 1); p += sizeof(float) * 1; break;
                                case DXGI_FORMAT_R16G16_FLOAT: inputs(DirectX::PackedVector::XMLoadHalf2((DirectX::PackedVector::XMHALF2*)p), 2); p += sizeof(uint16) * 2; break;
                                case DXGI_FORMAT_R11G11B10_FLOAT: inputs(DirectX::PackedVector::XMLoadFloat3PK((DirectX::PackedVector::XMFLOAT3PK*)p), 3); p += 4; break;
                                case DXGI_FORMAT_R8G8B8A8_UINT: inputs(DirectX::PackedVector::XMLoadUByte4((DirectX::PackedVector::XMUBYTE4*)p), 4, "%.0f"); p += sizeof(byte) * 4; break;
                                case DXGI_FORMAT_R8G8B8A8_UNORM: inputs(DirectX::PackedVector::XMLoadUByteN4((DirectX::PackedVector::XMUBYTEN4*)p), 4); p += sizeof(byte) * 4; break;
                                default: I::TextUnformatted(std::format("<c=#F00>Unhandled format {}</c>", element.Format).c_str()); break;
                            }
                        }
                    }
                }
            }
        }
    }
}

bool Mesh::HitTest(HitTestContext& context) const
{
    if (!GetProperties().Visible || !GetProperties().HitTestable)
        return false;

    auto const transform = GetTransform();
    auto const invertedTransform = transform.Invert();
    auto const localOrigin = Vector3::Transform(context.Origin, invertedTransform);
    auto localDirection = Vector3::TransformNormal(context.Direction, invertedTransform);
    localDirection.Normalize();

    float localHitDist;
    if (!m_boundingBox.Intersects(localOrigin, localDirection, localHitDist))
        return false;

    if (auto const worldHitDistSquared = Vector3::DistanceSquared(context.Origin, Vector3::Transform(localOrigin + localDirection * localHitDist, transform)); worldHitDistSquared < context.Coarse.ClosestDistanceSquared)
    {
        context.Coarse.ClosestObject = (SceneObject*)this;
        context.Coarse.ClosestDistanceSquared = worldHitDistSquared;
    }

    auto localClosestDist = std::numeric_limits<float>::max();
    for (auto itr = m_indices.begin(); itr != m_indices.end();)
    {
        auto const i0 = *itr++;
        auto const i1 = *itr++;
        auto const i2 = *itr++;
        if (DirectX::TriangleTests::Intersects(localOrigin, localDirection, m_vertices[i0].Position, m_vertices[i1].Position, m_vertices[i2].Position, localHitDist) && localHitDist < localClosestDist)
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

enum class FVFFlags : uint32
{
    None               = 0,
    Position           = 0x00000001,
    BlendWeight        = 0x00000002,
    BlendIndices       = 0x00000004,
    Normal             = 0x00000008,
    Color              = 0x00000010,
    Tangent            = 0x00000020,
    Binormal           = 0x00000040,
    TangentFrame       = 0x00000080,
    UV32Mask           = 0x0000ff00,
    UV16Mask           = 0x00ff0000,
    Unknown1           = 0x01000000,
    Unknown2           = 0x02000000,
    Unknown3           = 0x04000000,
    Unknown4           = 0x08000000,
    PositionCompressed = 0x10000000,
    Unknown5           = 0x20000000
};
std::unordered_map<FVFFlags, FVFInfo> cachedInputLayouts;
FVFInfo const* BuildLayout(FVFFlags fvfMask)
{
    auto& cached = cachedInputLayouts[fvfMask];
    if (cached.Size)
        return &cached;

    FVFInfo info;
    auto add = [&info](char const* name, uint32 index, DXGI_FORMAT format, int32* outOffset = nullptr)
    {
        info.Layout.push_back(
        {
            .SemanticName         = name,
            .SemanticIndex        = index,
            .Format               = format,
            .AlignedByteOffset    = info.Size,
            .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
        });
        if (outOffset)
            outOffset[index] = info.Size;
        info.Size += DirectX::BitsPerPixel(format) / 8;
    };

    if (fvfMask & FVFFlags::Position)       add("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, &info.Offset.Position);
    if (fvfMask & FVFFlags::BlendWeight)    add("BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, &info.Offset.BlendWeights);
    if (fvfMask & FVFFlags::BlendIndices)   add("BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, &info.Offset.BlendIndices);
    if (fvfMask & FVFFlags::Normal)         add("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, &info.Offset.Normal);
    if (fvfMask & FVFFlags::Color)          add("COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, &info.Offset.Color);
    if (fvfMask & FVFFlags::Tangent)        add("TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, &info.Offset.Tangent);
    if (fvfMask & FVFFlags::Binormal)       add("BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, &info.Offset.Bitangent);
    if (fvfMask & FVFFlags::TangentFrame)
    {
        add("NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, &info.Offset.NormalCompressed);
        add("TANGENT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, &info.Offset.TangentCompressed);
        add("BINORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, &info.Offset.BitangentCompressed);
    }

    uint32 texCoordIndex = 0;
    for (auto i = 8; i < 24; ++i)
        if (fvfMask & FVFFlags(1 << i))
            add("TEXCOORD", texCoordIndex++, i < 16 ? DXGI_FORMAT_R32G32_FLOAT : DXGI_FORMAT_R16G16_FLOAT, i < 16 ? info.Offset.TexCoord32.data() : info.Offset.TexCoord16.data());

    uint32 padIndex = 0;
    if (fvfMask & FVFFlags::Unknown1)
{
        add("UNKONE", padIndex++, DXGI_FORMAT_R32G32B32A32_FLOAT);
        add("UNKONE", padIndex++, DXGI_FORMAT_R32G32B32A32_FLOAT);
        add("UNKONE", padIndex++, DXGI_FORMAT_R32G32B32A32_FLOAT);
    }
    if (fvfMask & FVFFlags::Unknown2) add("UNKTWO", 0, DXGI_FORMAT_R32_FLOAT);
    if (fvfMask & FVFFlags::Unknown3) add("UNKTHREE", 0, DXGI_FORMAT_R32_FLOAT);
    if (fvfMask & FVFFlags::Unknown4) add("UNKFOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
    if (fvfMask & FVFFlags::PositionCompressed) add("POSITION", 0, DXGI_FORMAT_R11G11B10_FLOAT, &info.Offset.PositionCompressed);
    if (fvfMask & FVFFlags::Unknown5) add("UNKFIVE", 0, DXGI_FORMAT_R32G32B32_FLOAT);

    cached = std::move(info);
    return &cached;
}

void Mesh::LoadMesh(uint32 fvf, uint32 vertexCount, byte const* vertices, uint32 indexCount, byte const* indices, uint32 diffuseFileID, uint32 normalFileID)
{
    m_fvf = BuildLayout((FVFFlags)fvf);
    //device->CreateInputLayout(info->Layout.data(), info->Layout.size(), shaderCode, shaderSize, &m_inputLayout);

    m_vertexData.assign_range(std::span { vertices, vertexCount * m_fvf->Size });
    m_vertices.reserve(vertexCount);
    for (auto i = 0; i < vertexCount; ++i)
    {
        auto& vertex = m_vertices.emplace_back();
        if (m_fvf->Offset.Position >= 0)
            vertex.Position = *(Vector3*)&vertices[m_fvf->Offset.Position];
        else if (m_fvf->Offset.PositionCompressed >= 0)
            vertex.Position = DirectX::PackedVector::XMLoadFloat3PK((DirectX::PackedVector::XMFLOAT3PK*)&vertices[m_fvf->Offset.PositionCompressed]);
        if (m_fvf->Offset.BlendWeights >= 0)
            vertex.BlendWeights = *(std::array<byte, 4>*)&vertices[m_fvf->Offset.BlendWeights];
        if (m_fvf->Offset.BlendIndices >= 0)
            vertex.BlendIndices = *(std::array<byte, 4>*)&vertices[m_fvf->Offset.BlendIndices];
        if (m_fvf->Offset.Normal >= 0)
            vertex.Normal = *(Vector3*)&vertices[m_fvf->Offset.Normal];
        else if (m_fvf->Offset.NormalCompressed >= 0)
            vertex.Normal = DirectX::PackedVector::XMLoadUByteN4((DirectX::PackedVector::XMUBYTEN4*)&vertices[m_fvf->Offset.NormalCompressed]);
        if (m_fvf->Offset.Tangent >= 0)
            vertex.Tangent = *(Vector3*)&vertices[m_fvf->Offset.Tangent];
        else if (m_fvf->Offset.TangentCompressed >= 0)
            vertex.Tangent = DirectX::PackedVector::XMLoadUByteN4((DirectX::PackedVector::XMUBYTEN4*)&vertices[m_fvf->Offset.TangentCompressed]);
        if (m_fvf->Offset.Bitangent >= 0)
            vertex.Bitangent = *(Vector3*)&vertices[m_fvf->Offset.Bitangent];
        else if (m_fvf->Offset.BitangentCompressed >= 0)
            vertex.Bitangent = DirectX::PackedVector::XMLoadUByteN4((DirectX::PackedVector::XMUBYTEN4*)&vertices[m_fvf->Offset.BitangentCompressed]);
        if (m_fvf->Offset.Color >= 0)
            vertex.Color = DirectX::PackedVector::XMLoadUByte4((DirectX::PackedVector::XMUBYTE4*)&vertices[m_fvf->Offset.Color]);
        else
            vertex.Color = Vector4 { 1, 1, 1, 1 };
        if (m_fvf->Offset.TexCoord32[0] >= 0)
            vertex.UV = *(Vector2*)&vertices[m_fvf->Offset.TexCoord32[0]];
        else if (m_fvf->Offset.TexCoord16[0] >= 0)
            vertex.UV = DirectX::PackedVector::XMLoadHalf2((DirectX::PackedVector::XMHALF2*)&vertices[m_fvf->Offset.TexCoord16[0]]);
        vertices += m_fvf->Size;
    }
    m_indices.assign_range(std::span { (uint16*)indices, indexCount });
    BoundingBox::CreateFromPoints(m_boundingBox, m_vertices.size(), &m_vertices.data()->Position, sizeof(Vertex));
    if (auto const file = G::Game.Archive.GetFileEntry(diffuseFileID))
        diffuseFileID = file->GetBestVersion().ID;
    m_fileDiffuse = diffuseFileID;
    if (auto const file = G::Game.Archive.GetFileEntry(normalFileID))
        normalFileID = file->GetBestVersion().ID;
    m_fileNormal = normalFileID;

    CreateSelectionBuffers(m_boundingBox);

    m_vertexBuffer = G::Services::Graphics.CreateVertexBuffer(m_vertices);
    m_indexBuffer = G::Services::Graphics.CreateIndexBuffer(m_indices);
    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(ObjectConstantBuffer { });
}

void Mesh::SetBones(std::vector<Bone const*>&& bones)
{
    m_bones = std::move(bones);
}

BoundingBox Mesh::GetBoundingBox() const
{
    BoundingBox result;
    m_boundingBox.Transform(result, GetTransform());
    return result;
}

}
