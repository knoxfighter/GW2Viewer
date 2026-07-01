export module GW2Viewer.Data.Model:Camera;
import :Common;

export namespace GW2Viewer::Data::Model
{

struct Camera : SceneObject
{
    explicit Camera(Scene* scene, std::string_view name) : SceneObject(scene, name) { }

    void Update() override;
    void Debug() override;

    virtual void HandleInput(Vector2 pan, Vector2 rotation, float zoom) = 0;
    virtual void Focus(SceneObject const& object, bool smooth = false);

    auto GetForward() const { return Vector3::Transform(Vector3::UnitX, GetRotationQuaternion()); }
    auto GetRight() const { return Vector3::Transform(-Vector3::UnitY, GetRotationQuaternion()); }
    auto GetUp() const { return Vector3::Transform(-Vector3::UnitZ, GetRotationQuaternion()); }

    auto const& GetView() const { return m_view; }

    auto GetFoV() const { return m_fov; }
    void SetFoV(float value) { m_fov = value; UpdateProjection(); }
    auto GetAspectRatio() const { return m_aspectRatio; }
    void SetAspectRatio(float value) { m_aspectRatio = value; UpdateProjection(); }
    auto GetNearClip() const { return m_nearClip; }
    void SetNearClip(float value) { m_nearClip = value; UpdateProjection(); }
    auto GetFarClip() const { return m_farClip; }
    void SetFarClip(float value) { m_farClip = value; UpdateProjection(); }
    auto const& GetProjection() const { return m_projection; }

protected:
    Matrix m_view = Matrix::Identity;
    void UpdateView();

    float m_fov = DirectX::XM_PIDIV4;
    float m_aspectRatio = 1;
    float m_nearClip = 1;
    float m_farClip = 100000;
    Matrix m_projection = Matrix::Identity;
    void UpdateProjection();
};

struct OrbitCamera : Camera
{
    explicit OrbitCamera(Scene* scene, std::string_view name) : Camera(scene, name) { }

    void Update() override;
    void Debug() override;

    void HandleInput(Vector2 pan, Vector2 rotation, float zoom) override;
    void Focus(SceneObject const& object, bool smooth) override;

    auto GetTarget() const { return m_targetTarget; }
    void SetTarget(Vector3 value) { m_target = m_targetTarget = value; }
    auto GetRadius() const { return m_radiusTarget; }
    void SetRadius(float value) { m_radius = m_radiusTarget = value; }
    auto GetYaw() const { return m_yawTarget; }
    void SetYaw(Radians value) { m_yaw = m_yawTarget = value; }
    auto GetPitch() const { return m_pitchTarget; }
    void SetPitch(Radians value) { m_pitch = m_pitchTarget = value; }
    auto GetRoll() const { return m_rollTarget; }
    void SetRoll(Radians value) { m_roll = m_rollTarget = value; }

private:
    Vector3 m_target = Vector3::Zero;
    Vector3 m_targetTarget = m_target;
    float m_radius = 15.0f;
    float m_radiusTarget = m_radius;
    Radians m_yaw = -DirectX::XM_PIDIV4;
    Radians m_yawTarget = m_yaw;
    Radians m_pitch = DirectX::XM_PIDIV4 / 2;
    Radians m_pitchTarget = m_pitch;
    Radians m_roll = 0;
    Radians m_rollTarget = m_roll;
};

}
