#include "materialsystem.hpp"
#include "render.hpp"
#include "utils.hpp"
#include "dds.hpp"
#include <d3dcompiler.h>
#include <stdio.h>

static MaterialSystem g_Materials;
MaterialSystem* materials = &g_Materials;

void MaterialSystem::Init()
{
    m_Materials["depth"] = std::make_shared<DepthMaterial>();
    std::shared_ptr<VertexShader> debug_vs = materials->FindVertexShader("debug/base_vs");
    m_Materials["debug_normal"]    = std::make_shared<DebugMaterial>(debug_vs, materials->FindPixelShader("debug/normal_ps"));
    m_Materials["debug_tangent_s"] = std::make_shared<DebugMaterial>(debug_vs, materials->FindPixelShader("debug/tangent_s_ps"));
    m_Materials["debug_tangent_t"] = std::make_shared<DebugMaterial>(debug_vs, materials->FindPixelShader("debug/tangent_t_ps"));

}

VertexShader::VertexShader(const char* filename) : m_Name(filename)
{
    ID3DBlob* errorBlob;
    ID3DBlob* vertexShaderBuffer;

    wchar_t shadername[80];
    swprintf(shadername, L"shaders/%S.fx", filename);

    HRESULT hr = D3DCompileFromFile(shadername, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vertexShaderBuffer, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            MessageBox(render->GetHWND(), (char*)errorBlob->GetBufferPointer(), "Error", 0);
            THROW_RUNTIME((char*)errorBlob->GetBufferPointer())
        }
        else
        {
            char info[80];
            sprintf(info, "Error compiling Vertex Shader HRESULT: %d", hr);
            THROW_RUNTIME(info);
        }
    }

    render->GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &m_VertexShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT_S", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT_T", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    render->GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout),
        vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_InputLayout);

}

void VertexShader::Set() const
{
    render->GetDeviceContext()->IASetInputLayout(m_InputLayout.Get());
    render->GetDeviceContext()->VSSetShader(m_VertexShader.Get(), nullptr, 0);
}

PixelShader::PixelShader(const char* filename) : m_Name(filename)
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
    render->GetDeviceContext()->PSSetShader(m_PixelShader.Get(), nullptr, 0);
}

Texture::Texture(const char* filename) : m_Name(filename)
{
    wchar_t fullpath[256];
    swprintf(fullpath, L"textures/%S.dds", filename);
    if (FAILED(DirectX::CreateDDSTextureFromFile(render->GetDevice(), render->GetDeviceContext(), fullpath, nullptr, &m_TextureView)))
    {
        THROW_RUNTIME("Failed to load texture " << fullpath)
    }

}

Texture::Texture(unsigned int width, unsigned int height, const char* name) : m_Name(name)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        
    // Create the texture
    ID3D11Texture2D* texture;
    render->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    render->GetDevice()->CreateShaderResourceView(texture, &shaderResourceViewDesc, &m_TextureView);
    
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;
    render->GetDevice()->CreateRenderTargetView(texture, &renderTargetViewDesc, &m_RenderTargetView);

    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ScopedObject<ID3D11Texture2D> depthStencilTexture;
    render->GetDevice()->CreateTexture2D(&descDepth, nullptr, &depthStencilTexture);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    render->GetDevice()->CreateDepthStencilView(depthStencilTexture.Get(), &descDSV, &m_DepthStencilView);

}

void Texture::Set(unsigned int slot) const
{
    render->GetDeviceContext()->PSSetShaderResources(slot, 1, &m_TextureView);
}

std::shared_ptr<VertexShader> MaterialSystem::FindVertexShader(const char* filename)
{
    std::map<std::string, std::shared_ptr<VertexShader> >::iterator it = m_VertexShaders.find(filename);
    if (it != m_VertexShaders.end())
    {
        return it->second;
    }

    m_VertexShaders[filename] = std::make_shared<VertexShader>(filename);
    return m_VertexShaders[filename];

}

std::shared_ptr<PixelShader> MaterialSystem::FindPixelShader(const char* filename)
{
    std::map<std::string, std::shared_ptr<PixelShader> >::iterator it = m_PixelShaders.find(filename);
    if (it != m_PixelShaders.end())
    {
        return it->second;
    }

    m_PixelShaders[filename] = std::make_shared<PixelShader>(filename);
    return m_PixelShaders[filename];

}

std::shared_ptr<Texture> MaterialSystem::FindTexture(const char* filename, TextureGroup_t textureGroup)
{
    std::map<std::string, std::shared_ptr<Texture> >::iterator it = m_Textures.find(filename);
    if (it != m_Textures.end())
    {
        return it->second;
    }

    std::shared_ptr<Texture> texture;
    try
    {
        texture = std::make_shared<Texture>(filename);
        m_Textures[filename] = texture;
        return texture;
    }
    catch (...)
    {
        // Failed to load texture
        switch (textureGroup)
        {
        case TEXTURE_GROUP_DIFFUSE:
            return FindTexture("checker");
        case TEXTURE_GROUP_NORMAL:
            return FindTexture("normal_flat");
        default:
            return FindTexture("black");
        }
    }

}

std::shared_ptr<Texture> MaterialSystem::CreateRenderableTexture(unsigned int width, unsigned int height, const char* filename)
{
    std::map<std::string, std::shared_ptr<Texture> >::iterator it = m_Textures.find(filename);
    if (it != m_Textures.end())
    {
        return it->second;
    }

    m_Textures[filename] = std::make_shared<Texture>(width, height, filename);
    return m_Textures[filename];
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
    std::shared_ptr<Material> material = nullptr;
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
            else if (strcmp(buffer, "sky") == 0)
            {
                material = std::make_shared<SkyMaterial>(file);
                m_Materials[filename] = material;
            }
        }

    }

    fclose(file);

    return material;
    
}

void Material::SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const
{
    m_VertexShader->Set();
    m_PixelShader->Set();

    render->GetDeviceContext()->RSSetState(m_RasterizerState.Get());

    render->GetDeviceContext()->UpdateSubresource(m_VSConstantBuffer.Get(), 0, nullptr, &vsBuffer, 0, 0);
    render->GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_VSConstantBuffer);

    render->GetDeviceContext()->UpdateSubresource(m_PSConstantBuffer.Get(), 0, nullptr, &psBuffer, 0, 0);
    render->GetDeviceContext()->PSSetConstantBuffers(1, 1, &m_PSConstantBuffer);
}

void Material::InitConstantBuffers()
{
    D3D11_BUFFER_DESC vsConstantBufferDesc;
    ZeroMemory(&vsConstantBufferDesc, sizeof(vsConstantBufferDesc));
    vsConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vsConstantBufferDesc.ByteWidth = sizeof(VSConstantBuffer);
    vsConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    render->GetDevice()->CreateBuffer(&vsConstantBufferDesc, nullptr, &m_VSConstantBuffer);

    D3D11_BUFFER_DESC psConstantBufferDesc;
    ZeroMemory(&psConstantBufferDesc, sizeof(psConstantBufferDesc));
    psConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    psConstantBufferDesc.ByteWidth = sizeof(PSConstantBuffer);
    psConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    render->GetDevice()->CreateBuffer(&psConstantBufferDesc, nullptr, &m_PSConstantBuffer);

}

void Material::InitSampler(ScopedObject<ID3D11SamplerState>& samplerState, D3D11_FILTER filter,
    D3D11_TEXTURE_ADDRESS_MODE addressMode, D3D11_COMPARISON_FUNC comparisonFunc) const
{
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = filter;
    sampDesc.AddressU = addressMode;
    sampDesc.AddressV = addressMode;
    sampDesc.AddressW = addressMode;
    sampDesc.ComparisonFunc = comparisonFunc;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    render->GetDevice()->CreateSamplerState(&sampDesc, &samplerState);
}

WorldMaterial::WorldMaterial(FILE* file)
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
        else if (strcmp(buffer, "$normal") == 0)
        {
            fscanf(file, "%s", buffer);
            m_Normal = materials->FindTexture(buffer, TEXTURE_GROUP_NORMAL);
        }
        else if (strcmp(buffer, "$spec") == 0)
        {
            fscanf(file, "%s", buffer);
            m_Specular = materials->FindTexture(buffer, TEXTURE_GROUP_OTHER);
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

    m_ShadowDepth = materials->FindTexture("_rt_ShadowDepth", TEXTURE_GROUP_SHADOW_DEPTH);

    if (!m_Normal)
    {
        m_Normal = materials->FindTexture("normal_flat", TEXTURE_GROUP_NORMAL);
    }
    if (!m_Specular)
    {
        m_Specular = materials->FindTexture("white", TEXTURE_GROUP_OTHER);
    }
    
    render->GetDevice()->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);
    InitConstantBuffers();
    
    InitSampler(m_SamplerLinear, D3D11_FILTER_MIN_MAG_MIP_LINEAR);
    render->GetDeviceContext()->PSSetSamplers(0, 1, &m_SamplerLinear);
    InitSampler(m_SamplerShadowDepth, D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_COMPARISON_LESS);
    render->GetDeviceContext()->PSSetSamplers(1, 1, &m_SamplerShadowDepth);

    /*
    D3D11_BLEND_DESC omDesc;
    ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
    omDesc.RenderTarget[0].BlendEnable = true;
    omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    render->GetDevice()->CreateBlendState(&omDesc, &m_OpacityBlend);
    */
    
}

void WorldMaterial::SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const
{
    m_Albedo->Set(0);
    m_ShadowDepth->Set(1);
    m_Normal->Set(2);
    m_Specular->Set(3);

    Material::SetMaterial(vsBuffer, psBuffer);
    
}

SkyMaterial::SkyMaterial(FILE* file)
{
    m_VertexShader = materials->FindVertexShader("sky");
    m_PixelShader = materials->FindPixelShader("sky");

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
    InitConstantBuffers();

    InitSampler(m_SamplerLinear, D3D11_FILTER_MIN_MAG_MIP_LINEAR);
    render->GetDeviceContext()->PSSetSamplers(0, 1, &m_SamplerLinear);

    /*
    D3D11_BLEND_DESC omDesc;
    ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
    omDesc.RenderTarget[0].BlendEnable = true;
    omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    render->GetDevice()->CreateBlendState(&omDesc, &m_OpacityBlend);
    */

}

void SkyMaterial::SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const
{
    m_Albedo->Set(0);

    Material::SetMaterial(vsBuffer, psBuffer);

}

DepthMaterial::DepthMaterial()
{
    m_VertexShader = materials->FindVertexShader("depth");
    m_PixelShader = materials->FindPixelShader("depth");

    InitConstantBuffers();
    
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    render->GetDevice()->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);


}

DebugMaterial::DebugMaterial(std::shared_ptr<VertexShader> vs, std::shared_ptr<PixelShader> ps)
{
    m_VertexShader = vs;
    m_PixelShader = ps;

    InitConstantBuffers();

    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    render->GetDevice()->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);

}
