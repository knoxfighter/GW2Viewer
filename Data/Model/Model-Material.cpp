module;
#include "Macros.h"
#include <d3d11.h>

module GW2Viewer.Data.Model;
import :Material;
import :Scene;
import GW2Viewer.UI.Manager;
import <d3dcompiler.h>;
#pragma comment(lib, "d3dcompiler")

std::string shaderBasic = R"(
Texture2D texDiffuse : register(t0);
Texture2D texNormal : register(t1);
SamplerState texSampler : register(s0);

cbuffer CB : register(b0)
{
    matrix ViewProj;
    float3 EyePosition;
    float Padding0;
    float3 LightDir;
    float Padding1;
};

cbuffer MaterialCB : register(b1)
{
    uint CustomMaterialFlags;
};

cbuffer MeshCB : register(b2)
{
    matrix World;
    matrix WorldInvertedTransposed;
    uint CustomMeshFlags;
};

struct VS_IN
{
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float3 Tangent   : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 UV        : TEXCOORD;
    float4 Color     : COLOR;
};

struct PS_IN
{
    float4 Position      : SV_POSITION;
    float3 WorldPosition : POSITION;
    float4 Normal        : NORMAL;
    float4 Tangent       : TANGENT;
    float4 Bitangent     : BITANGENT;
    float2 UV            : TEXCOORD;
    float4 Color         : COLOR;
};

PS_IN VS(VS_IN input)
{
    PS_IN output;
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    output.WorldPosition = worldPos.xyz;
    output.Position = mul(worldPos, ViewProj);

    float3x3 world3x3 = (float3x3)World;
    float3 viewDir = normalize(EyePosition - output.WorldPosition);
    output.Normal = float4(normalize(mul(input.Normal.zyx * 2.0f - 1.0f, (float3x3)WorldInvertedTransposed)), viewDir.z);
    output.Tangent = float4(normalize(mul(input.Tangent.zyx * 2.0f - 1.0f, world3x3)), viewDir.x);
    output.Bitangent = float4(-normalize(mul(input.Bitangent.zyx * 2.0f - 1.0f, world3x3)), viewDir.y);

    output.UV = input.UV;
    output.Color = input.Color;
    return output;
}

float4 PS(PS_IN input) : SV_Target
{
    if (CustomMaterialFlags & 0x2)
        return input.Color;

    float4 texColor = texDiffuse.Sample(texSampler, input.UV);
    if (CustomMaterialFlags & 0x1 && texColor.a * 2 < 0.5f)
        discard;

    float3x3 tbn = float3x3(input.Tangent.xyz, input.Bitangent.xyz, input.Normal.xyz);
    float3 viewDir = normalize(float3(input.Tangent.w, input.Bitangent.w, input.Normal.w));

    // Sample normal from normal map and extrapolate Z despite it being available in the texture /shrug
    float3 normalColor;
    normalColor.xy = texNormal.Sample(texSampler, input.UV).rg * 2.0f - 1.0f;
    normalColor.z = sqrt(max(1.0 - dot(normalColor.xy, normalColor.xy), 0.0));
    normalColor = normalize(normalColor);

    float3 worldNormal = normalize(mul(normalColor, tbn));

    float3 ambient = 0.2f * texColor.rgb;

    float3 toLight = normalize(-LightDir);
    //float3 viewDir = normalize(EyePosition - input.WorldPosition);
    float3 halfDir = normalize(toLight + viewDir);

    float diffuseAmount = max(dot(worldNormal, toLight), 0.0f);
    float3 diffuse = diffuseAmount * texColor.rgb;

    //float glossiness = min(saturate(texColor.a - 0.5f) * 2.0f, 1.0f) * 128.0f;
    //float specPower = pow(max(dot(worldNormal, halfDir), 0.0001f), glossiness);
    //float shadowMask = 1.0f;
    //float3 specular = specPower * shadowMask;
    float specPower = pow(max(dot(worldNormal, halfDir), 0.0f), 32.0f);
    float specIntensity = saturate(texColor.a - 0.5f) * 2.0f;
    float3 specular = specPower * specIntensity;

    float fresnel = 1.0 - saturate(dot(worldNormal, viewDir));
    float halfLambert = dot(worldNormal, viewDir) * 0.5 + 0.5;
    float rimMask = (fresnel * fresnel) * (halfLambert * halfLambert);
    float3 rimLight = float3(0.149, 0.129, 0.117) * rimMask;

    float3 finalColor = input.Color.rgb * (ambient + rimLight + diffuse + specular);
    if (CustomMeshFlags & 0x1)
        finalColor = lerp(finalColor, float3(0.75f, 0.75f, 1.0f), 0.25f);
    return float4(finalColor, CustomMaterialFlags & 0x1 ? 1 : saturate(texColor.a * 2.0f));
}
)";

namespace GW2Viewer::Data::Model
{

struct MaterialConstantBuffer
{
    uint32 AlphaTestTransparency : 1 = false;
    uint32 DebugShapes : 1 = false;
    float Padding0;
    float Padding1;
    float Padding2;
};
static_assert(!(sizeof(MaterialConstantBuffer) % 16));

void Material::Bind()
{
    if (!m_vertexShader.Get() || !m_pixelShader.Get())
        return;

    ID3D11ShaderResourceView* textures[] { m_scene->GetDummyTextureDiffuse(), m_scene->GetDummyTextureNormal() };

    auto context = G::Services::Graphics.Context;
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());
    context->RSSetState(m_rasterizerState.Get());
    context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);
    context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
    context->PSSetShaderResources(0, 2, textures);

    MaterialConstantBuffer buffer
    {
        .AlphaTestTransparency = m_alphaTestTransparency,
        .DebugShapes = m_debugShapesMaterial,
    };
    m_constantBuffer.Update(buffer);
    context->VSSetConstantBuffers(1, 1, m_constantBuffer.Ptr.GetAddressOf());
    context->PSSetConstantBuffers(1, 1, m_constantBuffer.Ptr.GetAddressOf());
}

void Material::Debug()
{
    I::Checkbox("Alpha Test Transparency", &m_alphaTestTransparency);
    if (scoped::Child("Shader", { -FLT_MIN, 50 }, ImGuiChildFlags_ResizeY))
        if (scoped::Font(G::UI.Fonts.Monospace, 12))
            I::InputTextMultiline("##Shader", &shaderBasic, { -FLT_MIN, -FLT_MIN });
    if (I::Button("Compile", { -FLT_MIN, 0 }))
        CreateBasic();
}

void Material::CreateBasic()
{
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    D3DCompile(shaderBasic.data(), shaderBasic.size(), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    D3DCompile(shaderBasic.data(), shaderBasic.size(), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (errorBlob.Get())
    {
        MessageBoxA(nullptr, (char const*)errorBlob->GetBufferPointer(), "Shader Compilation Error", 0);
        return;
    }

    auto device = G::Services::Graphics.Device;
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

    D3D11_INPUT_ELEMENT_DESC const layout[]
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, Position),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, Normal),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, Tangent),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, Bitangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(Vertex, UV),        D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Color),     D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    device->CreateInputLayout(layout, std::size(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

    D3D11_BLEND_DESC const blendDesc
    {
        .RenderTarget =
        {
            {
                .BlendEnable           = true,
                .SrcBlend              = D3D11_BLEND_SRC_ALPHA,
                .DestBlend             = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOp               = D3D11_BLEND_OP_ADD,
                .SrcBlendAlpha         = D3D11_BLEND_ONE,
                .DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha          = D3D11_BLEND_OP_ADD,
                .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
            },
        },
    };
    device->CreateBlendState(&blendDesc, &m_blendState);

    D3D11_RASTERIZER_DESC const rasterDesc
    {
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
        .DepthClipEnable = true,
    };
    device->CreateRasterizerState(&rasterDesc, &m_rasterizerState);

    D3D11_SAMPLER_DESC const sampDesc
    {
        .Filter = D3D11_FILTER_ANISOTROPIC,
        .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
        .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
        .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D11_COMPARISON_NEVER,
        .BorderColor = { 1, 0, 0, 1 },
        .MinLOD = 0.0f,
        .MaxLOD = D3D11_FLOAT32_MAX,
    };
    device->CreateSamplerState(&sampDesc, &m_samplerState);

    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(MaterialConstantBuffer { });
}

}
