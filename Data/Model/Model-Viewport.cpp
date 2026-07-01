module GW2Viewer.Data.Model;
import :Camera;
import :Scene;
import :Viewport;

namespace GW2Viewer::Data::Model
{

struct ViewportConstantBuffer
{
    Matrix ViewProjection;
    Vector3 EyePosition;
    float Padding0;
    Vector3 LightDir;
    float Padding1;
};
static_assert(!(sizeof(ViewportConstantBuffer) % 16));

Viewport::Viewport(Scene* scene): m_scene(scene)
{
    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(ViewportConstantBuffer { });
}

void Viewport::Resize(ImVec2i size)
{
    m_renderTexture.Initialize(size);
    UpdateCamera();
}

void Viewport::UpdateCamera() const
{
    if (m_camera)
        m_camera->SetAspectRatio(m_renderTexture.GetAspectRatio());
}

void Viewport::Render()
{
    if (!m_scene || !m_camera)
        return;

    m_renderTexture.Bind();
    m_renderTexture.Clear({ 0.1f, 0.1f, 0.15f, 1.0f });

    ViewportConstantBuffer buffer
    {
        .ViewProjection = (m_camera->GetView() * m_camera->GetProjection()).Transpose(),
        .EyePosition = m_camera->GetPosition(),
        .LightDir = m_scene->GetLightDirection(),
    };
    buffer.LightDir.Normalize();
    m_constantBuffer.Update(buffer);
    auto context = G::Services::Graphics.Context;
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.Ptr.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.Ptr.GetAddressOf());

    m_scene->Render();
}

void Viewport::Debug()
{
}

void Viewport::HitTest(HitTestContext& context) const
{
    if (m_scene)
        m_scene->HitTest(context);
}

SceneObject* Viewport::HitTest(ImVec2 mouse, HitTestContext* outContext) const
{
    if (!m_camera)
        return nullptr;

    Vector3 const ndc { mouse.x / m_renderTexture.GetSize().x * 2.0f - 1.0f, 1.0f - mouse.y / m_renderTexture.GetSize().y * 2.0f, 0.0f };
    auto const invertedViewProjection = (m_camera->GetView() * m_camera->GetProjection()).Invert();

    HitTestContext context
    {
        .Origin = Vector3::Transform(ndc, invertedViewProjection),
    };
    context.Direction = Vector3::Transform(ndc + Vector3::UnitZ, invertedViewProjection) - context.Origin;
    context.Direction.Normalize();

    HitTest(context);

    auto const result = context.GetClosestObject();
    if (outContext)
        *outContext = std::move(context);
    return result;
}

}
