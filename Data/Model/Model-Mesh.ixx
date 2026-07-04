export module GW2Viewer.Data.Model:Mesh;
import :Common;

export namespace GW2Viewer::Data::Model
{
struct FVFInfo;

struct Mesh : SceneObject
{
    explicit Mesh(Scene* scene, std::string_view name) : SceneObject(scene, name) { }

    void Render() override;
    void Debug() override;

    void LoadMesh(uint32 fvf, uint32 vertexCount, byte const* vertices, uint32 indexCount, byte const* indices, uint32 diffuseFileID, uint32 normalFileID);

    BoundingBox GetBoundingBox() const override;
    bool HitTest(HitTestContext& context) const override;

private:
    FVFInfo const* m_fvf = nullptr;
    std::vector<byte> m_vertexData;
    std::vector<Vertex> m_vertices;
    std::vector<uint16> m_indices;
    BoundingBox m_boundingBox;
    uint32 m_fileDiffuse = 0;
    uint32 m_fileNormal = 0;
    bool m_debugHighlightHovered = false;

    Graphics::Buffer m_vertexBuffer;
    Graphics::Buffer m_indexBuffer;
    Graphics::Buffer m_constantBuffer;
};

}
