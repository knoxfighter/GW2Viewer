module;
#include <directxtk/SimpleMath.h> // Workaround for https://developercommunity.visualstudio.com/t/11029304

export module GW2Viewer.Data.Model:Common;
import GW2Viewer.Common;
import GW2Viewer.Services.Graphics;
import GW2Viewer.UI.ImGui;
import GW2Viewer.Utils.Enum;
import GW2Viewer.Utils.Math;
import std;
import <d3d11.h>;
import <DirectXTex.h>;
import <directxtk/SimpleMath.h>;
import <wrl/client.h>;

export
{
using DirectX::BoundingBox;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using Microsoft::WRL::ComPtr;
using GW2Viewer::Services::Graphics;
}

export namespace GW2Viewer::Data::Model
{
struct Camera;
struct Grid;
struct Material;
struct Mesh;
struct Scene;
struct Viewport;

Quaternion FromRollPitchYaw(float roll, float pitch, float yaw) { return Quaternion::CreateFromAxisAngle(Vector3::UnitX, roll) * Quaternion::CreateFromAxisAngle(-Vector3::UnitY, pitch) * Quaternion::CreateFromAxisAngle(-Vector3::UnitZ, yaw); }
Quaternion FromRollPitchYaw(Vector3 const& rollPitchYaw) { return FromRollPitchYaw(rollPitchYaw.x, rollPitchYaw.y, rollPitchYaw.z); }
Vector3 ToRollPitchYaw(Quaternion const& quaternion);

struct HitTestContext;
struct SceneObject
{
    struct Properties
    {
        bool Visible = true;
        bool Highlighted = false;
        bool Selected = false;
        bool HitTestable = true;
    };

    explicit SceneObject(Scene* scene, std::string_view name) : m_scene(scene), m_name(name) { }
    virtual ~SceneObject() = default;

    virtual void Update() { }
    virtual void Render() { }
    virtual void Debug();

    Scene* GetScene() const { return m_scene; }
    std::string const& GetName() const { return m_name; }
    void SetName(std::string_view name) { m_name = name; }

    Vector3 GetPosition() const { return m_position; }
    void SetPosition(Vector3 const& position) { m_position = position; UpdateTransform(); }
    void SetPosition(float x, float y, float z) { SetPosition({ x, y, z }); }
    Vector3 GetRotation() const { return m_rotation; }
    void SetRotation(Vector3 const& rollPitchYaw) { m_rotation = rollPitchYaw; UpdateTransform(); }
    void SetRotation(float roll, float pitch, float yaw) { SetRotation({ roll, pitch, yaw }); }
    Vector3 GetScale() const { return m_scale; }
    void SetScale(Vector3 const& scale) { m_scale = scale; UpdateTransform(); }
    void SetScale(float x, float y, float z) { SetScale({ x, y, z }); }
    void SetScale(float scale) { SetScale(scale, scale, scale); }
    Quaternion GetRotationQuaternion() const { return FromRollPitchYaw(m_rotation); }
    void SetRotationQuaternion(Quaternion const& rotation) { m_rotation = ToRollPitchYaw(rotation); UpdateTransform(); }
    Matrix const& GetTransform() const { return m_transform; }
    void SetTransform(Matrix const& transform) { m_transform = transform; Quaternion rotation; m_transform.Decompose(m_scale, rotation, m_position); m_rotation = ToRollPitchYaw(rotation); }

    virtual BoundingBox GetBoundingBox() const { return { }; }
    virtual bool HitTest(HitTestContext& context) const { return false; }

    Properties& GetProperties() { return m_properties; }
    Properties const& GetProperties() const { return m_properties; }

protected:
    void CreateSelectionBuffers(BoundingBox const& localBox);
    bool RenderSelection();

private:
    Scene* m_scene;
    std::string m_name;

    Vector3 m_position;
    Vector3 m_rotation;
    Vector3 m_scale = Vector3::One;
    Matrix m_transform;
    void UpdateTransform() { m_transform = Matrix::CreateScale(m_scale) * Matrix::CreateFromQuaternion(FromRollPitchYaw(m_rotation)) * Matrix::CreateTranslation(m_position); }

    Properties m_properties;

    Graphics::Buffer m_selectionVertexBuffer;
    Graphics::Buffer m_selectionIndexBuffer;
};

struct HitTestContext
{
    struct Hit
    {
        SceneObject* ClosestObject = nullptr;
        float ClosestDistanceSquared = std::numeric_limits<float>::max();
    };

    Vector3 Origin;
    Vector3 Direction;
    Hit Fine; // triangle intersection
    Hit Coarse; // bounding box intersection

    bool HasHit() const { return Coarse.ClosestObject; }
    SceneObject* GetClosestObject() const { return Fine.ClosestObject ? Fine.ClosestObject : Coarse.ClosestObject; }
    float GetContactDistance() const { return std::sqrt(Fine.ClosestObject ? Fine.ClosestDistanceSquared : Coarse.ClosestDistanceSquared); }
    Vector3 GetContactPoint() const { return Origin + Direction * GetContactDistance(); }

    operator bool() const { return HasHit(); }
};

struct Vertex
{
    Vector3 Position;
    Vector3 Normal;
    Vector3 Tangent;
    Vector3 Bitangent;
    Vector2 UV;
    Vector4 Color;
};

struct ObjectConstantBuffer
{
    Matrix World;
    Matrix WorldInvertedTransposed = World.Invert();
    uint32 HighlightObject : 1 = false;
    float Padding0;
    float Padding1;
    float Padding2;
};
static_assert(!(sizeof(ObjectConstantBuffer) % 16));

}
