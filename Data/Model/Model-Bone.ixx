module;
#include <directxtk/SimpleMath.h> // Workaround for https://developercommunity.visualstudio.com/t/11029304

export module GW2Viewer.Data.Model:Bone;
import :Common;

export namespace GW2Viewer::Data::Model
{

struct Bone final : SceneObject
{
    Bone(Skeleton* skeleton, std::string_view name, Bone* parent);

    void Update() override;
    void Render() override;
    void Debug() override;

    BoundingBox GetBoundingBox() const override;
    bool HitTest(HitTestContext& context) const override;

    Skeleton& GetSkeleton() const { return *m_skeleton; }
    Bone* GetParent() const { return m_parent; }
    float GetBoneThickness() const;

    Matrix const& GetHierarchicalTransform() const { return m_hierarchicalTransform; }
    Vector3 GetHierarchicalPosition() const { return GetHierarchicalTransform().Translation(); }
    Quaternion GetHierarchicalRotationQuaternion() const
    {
        DirectX::XMVECTOR position, rotation, scale;
        DirectX::XMMatrixDecompose(&position, &rotation, &scale, GetHierarchicalTransform());
        return rotation;
    }
    Vector3 GetHierarchicalScale() const
    {
        DirectX::XMVECTOR position, rotation, scale;
        DirectX::XMMatrixDecompose(&position, &rotation, &scale, GetHierarchicalTransform());
        return scale;
    }
    void SetHierarchicalTransform(Matrix const& transform);

    Matrix const& GetInverseBindPose() const { return *m_inverseBindPose; }

    Matrix GetWorldSpaceTransform() const;
    Vector3 GetWorldSpacePosition() const;
    Quaternion GetWorldSpaceRotationQuaternion() const;
    Vector3 GetWorldSpaceScale() const;
    void SetWorldSpaceTransform(Matrix const& transform);

    Bone& CreateBone(std::string_view name) { return *m_children.emplace_back(std::make_unique<Bone>(m_skeleton, name, this)); }

    Bone* FindBoneByName(std::string_view name) const;
    Bone* FindBoneByToken(Token64 token) const;

private:
    Skeleton* m_skeleton;
    Bone* m_parent;
    Token64 m_token;
    float m_length = 0;
    std::list<std::unique_ptr<Bone>> m_children;

    Matrix m_hierarchicalTransform;
    std::optional<Matrix> m_inverseBindPose;

    float m_bufferedLength = 0;
    std::vector<Vertex> m_vertices;
    Graphics::Buffer m_vertexBuffer;
    Graphics::Buffer m_constantBuffer;
    void UpdateBuffers();
};

}
