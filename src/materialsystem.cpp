#include "materialsystem.hpp"
#include "render.hpp"
#include "WICTextureLoader.hpp"
#include "utils.hpp"
#include <d3dcompiler.h>
#include <stdio.h>

MaterialSystem g_Materials;
MaterialSystem* materials = &g_Materials;

void MaterialSystem::Init()
{

}

VertexShader::VertexShader(const char* filename)
{
    ID3DBlob* errorBlob;
    ID3DBlob* vertexShaderBuffer;

    wchar_t shadername[80];
    swprintf(shadername, L"shaders/%S.fx", filename);

    if (FAILED(D3DCompileFromFile(shadername, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vertexShaderBuffer, &errorBlob)))
    {
        THROW_RUNTIME((char*)errorBlob->GetBufferPointer())
    }

    render->GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &m_VertexShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    render->GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout),
        vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_InputLayout);

}

void VertexShader::Set() const
{
    render->GetDeviceContext()->IASetInputLayout(m_InputLayout);
    render->GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
}

PixelShader::PixelShader(const char* filename)
{
    ID3DBlob* errorBlob;
    ID3DBlob* pixelShaderBuffer;
    wchar_t shadername[80];
    swprintf(shadername, L"shaders/%S.fx", filename);

    if (FAILED(D3DCompileFromFile(shadername, nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &pixelShaderBuffer, &errorBlob)))
    {
        THROW_RUNTIME((char*)errorBlob->GetBufferPointer())
    }

    render->GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, &m_PixelShader);

}

void PixelShader::Set() const
{
    render->GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);
}

Texture::Texture(const char* filename) : name(filename)
{
    char fullpath[256];
    sprintf(fullpath, "textures/%s.bmp", filename);
    if (FAILED(CreateWICTextureFromFile(render->GetDevice(), render->GetDeviceContext(), fullpath, nullptr, &m_TextureView)))
    {
        THROW_RUNTIME("Failed to load texture " << fullpath)
    }


}

void Texture::Set(unsigned int slot) const
{
    render->GetDeviceContext()->PSSetShaderResources(slot, 1, &m_TextureView);
}

VertexShader* MaterialSystem::FindVertexShader(const char* filename)
{
    std::map<std::string, VertexShader>::iterator it = m_VertexShaders.find(filename);
    if (it != m_VertexShaders.end())
    {
        return &it->second;
    }

    m_VertexShaders[filename] = VertexShader(filename);
    return &m_VertexShaders[filename];

}

PixelShader* MaterialSystem::FindPixelShader(const char* filename)
{
    std::map<std::string, PixelShader>::iterator it = m_PixelShaders.find(filename);
    if (it != m_PixelShaders.end())
    {
        return &it->second;
    }

    m_PixelShaders[filename] = PixelShader(filename);
    return &m_PixelShaders[filename];

}

Texture* MaterialSystem::FindTexture(const char* filename)
{
    std::map<std::string, Texture>::iterator it = m_Textures.find(filename);
    if (it != m_Textures.end())
    {
        return &it->second;
    }

    m_Textures[filename] = Texture(filename);
    return &m_Textures[filename];

}

std::shared_ptr<Material> MaterialSystem::FindMaterial(const char* filename)
{
    std::map<std::string, std::shared_ptr<Material> >::iterator it = m_Materials.find(filename);
    if (it != m_Materials.end())
    {
        return it->second;
    }

    char fullpath[80];
    sprintf(fullpath, "materials/%s.mtl", filename);
    std::shared_ptr<WorldMaterial> material = nullptr;
    FILE* file = fopen(fullpath, "r");
    if (file == nullptr)
    {
        return FindMaterial("debug_checker");
    }

    char buffer[128];
    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (strcmp(buffer, "$mtl") == 0)
        {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "world") == 0)
            {
                material = std::make_shared<WorldMaterial>(file);
                m_Materials[filename] = material;
            }
            else if (strcmp(buffer, "depth") == 0)
            {
                // Create depth material

            }
        }

    }

    fclose(file);

    return material;
    
}

WorldMaterial::WorldMaterial(FILE* file) : m_Albedo(nullptr)
{
    m_VertexShader = materials->FindVertexShader("world");
    m_PixelShader = materials->FindPixelShader("world");
    
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    char buffer[128];
    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (strcmp(buffer, "$albedo") == 0)
        {
            fscanf(file, "%s", buffer);
            m_Albedo = materials->FindTexture(buffer);
        }
        else if (strcmp(buffer, "$nocull") == 0)
        {
            rasterizerDesc.CullMode = D3D11_CULL_NONE;
        }
        else if (strcmp(buffer, "$wireframe") == 0)
        {
            rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
        }

    }
    
    render->GetDevice()->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);
    
    D3D11_BUFFER_DESC vsConstantBufferDesc;
    ZeroMemory(&vsConstantBufferDesc, sizeof(vsConstantBufferDesc));

    vsConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vsConstantBufferDesc.ByteWidth = sizeof(VSConstantBuffer);
    vsConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    render->GetDevice()->CreateBuffer(&vsConstantBufferDesc, nullptr, &m_VSConstantBuffer);

    // Create PS Constant Buffer
    D3D11_BUFFER_DESC psConstantBufferDesc;
    ZeroMemory(&psConstantBufferDesc, sizeof(psConstantBufferDesc));

    psConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    psConstantBufferDesc.ByteWidth = sizeof(PSConstantBuffer);
    psConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    render->GetDevice()->CreateBuffer(&psConstantBufferDesc, nullptr, &m_PSConstantBuffer);

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    render->GetDevice()->CreateSamplerState(&sampDesc, &m_SamplerState);
    render->GetDeviceContext()->PSSetSamplers(0, 1, &m_SamplerState);

    //D3D11_BLEND_DESC omDesc;
    //ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
    //omDesc.RenderTarget[0].BlendEnable = true;
    //omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    //omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    //omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    //omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    //omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    //omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    //omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    //render->GetDevice()->CreateBlendState(&omDesc, &m_OpacityBlend);
    
}

void WorldMaterial::SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const
{
    m_Albedo->Set(0);

    m_VertexShader->Set();
    m_PixelShader->Set();

    render->GetDeviceContext()->RSSetState(m_RasterizerState);

    render->GetDeviceContext()->UpdateSubresource(m_VSConstantBuffer, 0, nullptr, &vsBuffer, 0, 0);
    render->GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_VSConstantBuffer);
    render->GetDeviceContext()->UpdateSubresource(m_PSConstantBuffer, 0, nullptr, &psBuffer, 0, 0);
    render->GetDeviceContext()->PSSetConstantBuffers(1, 1, &m_PSConstantBuffer);


}
