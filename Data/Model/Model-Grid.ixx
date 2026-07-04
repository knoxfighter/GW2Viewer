export module GW2Viewer.Data.Model:Grid;
import :Common;

export namespace GW2Viewer::Data::Model
{

struct Grid : SceneObject
{
    explicit Grid(Scene* scene, std::string_view name) : SceneObject(scene, name) { }

    void Initialize(uint32 size, float spacing);

    void Render() override;
    void Debug() override;

private:
    uint32 m_size = 0;
    float m_spacing = 0;
    bool m_visible = true;

    Graphics::Buffer m_vertexBuffer;
    Graphics::Buffer m_constantBuffer;
};

}
