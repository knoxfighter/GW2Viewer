export module GW2Viewer.Data.Model:Skeleton;
import :Common;

export namespace GW2Viewer::Data::Model
{

struct Skeleton final : SceneObject
{
    Skeleton(Scene* scene, std::string_view name);
    ~Skeleton() override;

    void Update() override;
    void Render() override;
    void Debug() override;

    BoundingBox GetBoundingBox() const override;
    bool HitTest(HitTestContext& context) const override;

    float GetBoneThickness() const { return m_boneThickness; }

    Bone& CreateRootBone(std::string_view name);

    Bone* FindBoneByName(std::string_view name) const;
    Bone* FindBoneByToken(Token64 token) const;

private:
    float m_boneThickness = 1.0f;

    std::list<std::unique_ptr<Bone>> m_rootBones;
};

}
