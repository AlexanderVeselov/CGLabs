#include "materialsystem.hpp"
#include "render.hpp"
#include "WICTextureLoader.hpp"
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
        MessageBox(render->GetHWND(), (char*)errorBlob->GetBufferPointer(), "Error", MB_OK);
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
        MessageBox(render->GetHWND(), (char*)errorBlob->GetBufferPointer(), "Error", MB_OK);
    }

    render->GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, &m_PixelShader);

}

void PixelShader::Set() const
{
    render->GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);
}

Texture::Texture(const char* filename)
{
    char fullpath[256];
    sprintf(fullpath, "textures/%s.bmp", filename);
    if (FAILED(CreateWICTextureFromFile(render->GetDevice(), render->GetDeviceContext(), fullpath, nullptr, &m_TextureView)))
    {
        throw std::runtime_error("Failed to open texture");
    }

}

void Texture::Set(unsigned int slot) const
{
    render->GetDeviceContext()->PSSetShaderResources(slot, 1, &m_TextureView);
}

VertexShader* MaterialSystem::FindVertexShader(const char* filename)
{
    std::map<const char*, VertexShader>::iterator it = m_VertexShaders.find(filename);
    if (it != m_VertexShaders.end())
    {
        return &it->second;
    }

    m_VertexShaders[filename] = VertexShader(filename);
    return &m_VertexShaders[filename];

}

PixelShader* MaterialSystem::FindPixelShader(const char* filename)
{
    std::map<const char*, PixelShader>::iterator it = m_PixelShaders.find(filename);
    if (it != m_PixelShaders.end())
    {
        return &it->second;
    }

    m_PixelShaders[filename] = PixelShader(filename);
    return &m_PixelShaders[filename];

}

Texture* MaterialSystem::FindTexture(const char* filename)
{
    std::map<const char*, Texture>::iterator it = m_Textures.find(filename);
    if (it != m_Textures.end())
    {
        return &it->second;
    }

    m_Textures[filename] = Texture(filename);
    return &m_Textures[filename];

}

std::shared_ptr<Material> MaterialSystem::FindMaterial(const char* filename)
{
    std::map<const char*, std::shared_ptr<Material> >::iterator it = m_Materials.find(filename);
    if (it != m_Materials.end())
    {
        return it->second;
    }

    char fullpath[80];
    sprintf(fullpath, "materials/%s.mtl", filename);
    FILE* file = fopen(fullpath, "r");
    if (file == nullptr)
    {
        return nullptr;
    }

    char buffer[128];
    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (strcmp(buffer, "$mtl") == 0)
        {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "world") == 0)
            {
                m_Materials[filename] = std::make_shared<WorldMaterial>(file);
            }
            else if (strcmp(buffer, "depth") == 0)
            {
                // Create depth material

            }
        }

    }

    fclose(file);

    return m_Materials[filename];
    
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
    
}

void WorldMaterial::SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const
{
    m_VertexShader->Set();
    m_PixelShader->Set();

    render->GetDeviceContext()->UpdateSubresource(m_VSConstantBuffer, 0, nullptr, &vsBuffer, 0, 0);
    render->GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_VSConstantBuffer);
    render->GetDeviceContext()->UpdateSubresource(m_PSConstantBuffer, 0, nullptr, &psBuffer, 0, 0);
    render->GetDeviceContext()->PSSetConstantBuffers(0, 1, &m_PSConstantBuffer);

    m_Albedo->Set(0);

}
