module;
#include <directxtk/SimpleMath.h> // Workaround for https://developercommunity.visualstudio.com/t/11029304

module GW2Viewer.Data.Model;
import :Camera;

namespace GW2Viewer::Data::Model
{

void Camera::Update()
{
    UpdateView();
}

void Camera::Debug()
{
    SceneObject::Debug();
    if (auto value = GetForward(); I::InputFloat3("Forward", &value.x, "%.3f", ImGuiInputTextFlags_ReadOnly)) { }
    if (auto value = GetRight(); I::InputFloat3("Right", &value.x, "%.3f", ImGuiInputTextFlags_ReadOnly)) { }
    if (auto value = GetUp(); I::InputFloat3("Up", &value.x, "%.3f", ImGuiInputTextFlags_ReadOnly)) { }
}

void Camera::Focus(SceneObject const& object, bool smooth)
{
    auto const box = object.GetBoundingBox();
    SetPosition(box.Center - GetForward() * Vector3(box.Extents).Length() * 2.5f);
}

void Camera::UpdateView()
{
    m_view = DirectX::XMMatrixLookToLH(GetPosition(), GetForward(), GetUp());
}

void Camera::UpdateProjection()
{
    m_projection = DirectX::XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_nearClip, m_farClip);
}

void OrbitCamera::Update()
{
    Utils::Math::ExpDecayChase(m_target.x, m_targetTarget.x, 15, 0.001f);
    Utils::Math::ExpDecayChase(m_target.y, m_targetTarget.y, 15, 0.001f);
    Utils::Math::ExpDecayChase(m_target.z, m_targetTarget.z, 15, 0.001f);
    Utils::Math::ExpDecayChase(m_radius, m_radiusTarget, 15, 0.001f);
    Utils::Math::ExpDecayChase(m_yaw, m_yawTarget, 15, 0.0001f);
    Utils::Math::ExpDecayChase(m_pitch, m_pitchTarget, 15, 0.0001f);
    Utils::Math::ExpDecayChase(m_roll, m_rollTarget, 15, 0.0001f);

    SetRotation(m_roll, m_pitch, m_yaw);
    SetPosition(m_target - m_radius * GetForward());
    Camera::Update();
}

void OrbitCamera::Debug()
{
    Camera::Debug();
    I::DragFloat3("Target", &m_targetTarget.x);
    I::DragFloat("Radius", &m_radiusTarget);
    I::DragFloat("Yaw", (float*)&m_yawTarget, 0.01f);
    I::DragFloat("Pitch", (float*)&m_pitchTarget, 0.01f);
    I::DragFloat("Roll", (float*)&m_rollTarget, 0.01f);
}

void OrbitCamera::HandleInput(Vector2 pan, Vector2 rotation, float zoom)
{
    m_targetTarget = m_targetTarget + GetRight() * pan.x * m_radius * -0.001f + GetUp() * pan.y * m_radius * 0.001f;
    m_radiusTarget = std::clamp(m_radiusTarget * std::exp(-zoom * 0.15f), 0.1f, m_farClip);
    m_yawTarget = Degrees(m_yawTarget.GetDegrees() + rotation.x * 0.5f);
    m_pitchTarget = Degrees(std::clamp(m_pitchTarget.GetDegrees() + rotation.y * 0.5f, -89.9f, 89.9f));
}

void OrbitCamera::Focus(SceneObject const& object, bool smooth)
{
    auto const box = object.GetBoundingBox();
    m_targetTarget = box.Center;
    m_radiusTarget = Vector3(box.Extents).Length() * 2.5f;
    if (!smooth)
    {
        m_target = m_targetTarget;
        m_radius = m_radiusTarget;
    }
}

}
