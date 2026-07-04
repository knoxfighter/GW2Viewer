export module GW2Viewer.Data.Model:Scene;
import :Common;

export namespace GW2Viewer::Data::Model
{

struct Scene : SceneObject
{
    explicit Scene(Scene* scene, std::string_view name);
    ~Scene() override;

    void Update() override;
    void Render() override;
    void Debug() override;

    Camera& CreateCameraOrbit(std::string_view name);
    Grid& CreateGrid(std::string_view name);
    Mesh& CreateMesh(std::string_view name);
    Scene& CreateScene(std::string_view name);

    BoundingBox GetBoundingBox() const override;
    bool HitTest(HitTestContext& context) const override;

    auto GetLightDirection() const { return m_lightDirection; }
    void SetLightDirection(Vector3 value) { m_lightDirection = value; }

    void SetCurrentMaterial(Material* material);
    void SetDebugShapes(bool enabled);

    auto GetDummyTextureDiffuse() const { return m_dummyTextureDiffuse.Get(); }
    auto GetDummyTextureNormal() const { return m_dummyTextureNormal.Get(); }

private:
    Vector3 m_lightDirection = { std::cos(DirectX::XM_PIDIV4), std::sin(DirectX::XM_PIDIV4), 1 };
    bool m_lightAutoRotate = false;

    std::unique_ptr<Material> m_basicMaterial;
    std::unique_ptr<Material> m_debugShapesMaterial;
    Material* m_currentMaterial = nullptr;
    Material* m_debugMaterialBackup = nullptr;
    uint32 m_debugShapesDepth = 0;

    std::list<std::unique_ptr<SceneObject>> m_objects;
    template<typename T, typename... TArgs> requires std::is_assignable_v<SceneObject, T>
    T& Create(std::string_view name, TArgs&&... args)
    {
        return *(T*)m_objects.emplace_back(std::make_unique<T>(this, name, std::forward<TArgs>(args)...)).get();
    }

    ComPtr<ID3D11ShaderResourceView> m_dummyTextureDiffuse;
    ComPtr<ID3D11ShaderResourceView> m_dummyTextureNormal;
};

}
