export module GW2Viewer.Data.Model:Viewport;
import :Common;
import GW2Viewer.UI.ImGui;

export namespace GW2Viewer::Data::Model
{

struct Viewport
{
    explicit Viewport(Scene* scene);

    void Resize(ImVec2i size);
    void Render();
    void Debug();

    void HitTest(HitTestContext& context) const;
    SceneObject* HitTest(ImVec2 mouse, HitTestContext* outContext = nullptr) const;

    void SetCamera(Camera* camera) { m_camera = camera; UpdateCamera(); }
    auto GetCamera() const { return m_camera; }
    auto GetShaderResourceView() const { return m_renderTexture.GetShaderResourceView(); }

private:
    Scene* m_scene = nullptr;
    Camera* m_camera = nullptr;
    void UpdateCamera() const;

    Graphics::RenderTexture m_renderTexture;
    Graphics::Buffer m_constantBuffer;
};

}
