#ifndef MATERIALSYSTEM_HPP
#define MATERIALSYSTEM_HPP

#include "mathlib.hpp"
#include "utils.hpp"
#include <map>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>

class VertexShader
{
public:
    VertexShader(const char* filename);
    void Set() const;

private:
    ScopedObject<ID3D11VertexShader> m_VertexShader;
    ScopedObject<ID3D11InputLayout> m_InputLayout;
    std::string m_Name;

};

class PixelShader
{
public:
    PixelShader(const char* filename);
    void Set() const;

private:
    ScopedObject<ID3D11PixelShader> m_PixelShader;
    std::string m_Name;

};

enum TextureGroup_t
{
    TEXTURE_GROUP_DIFFUSE = 0,
    TEXTURE_GROUP_NORMAL,
    TEXTURE_GROUP_SHADOW_DEPTH,
    TEXTURE_GROUP_OTHER
};

class Texture
{
public:
    // Load from file
    Texture(const char* filename);
    // Create procedural texture
    Texture(unsigned int width, unsigned int height, const char* name);
    void Set(unsigned int slot = 0) const;
    ID3D11RenderTargetView* GetRenderTargetView() { return m_RenderTargetView.Get(); }
    ID3D11DepthStencilView* GetDepthStencilView() { return m_DepthStencilView.Get(); }

private:
    ScopedObject<ID3D11ShaderResourceView> m_TextureView;
    ScopedObject<ID3D11RenderTargetView>   m_RenderTargetView;
    ScopedObject<ID3D11DepthStencilView>   m_DepthStencilView;
    std::string m_Name;

};

class Material
{
public:
    // TODO: overload this
    struct VSConstantBuffer
    {
        DirectX::XMMATRIX matModelToWorld;
        DirectX::XMMATRIX matWorldToCamera;
        DirectX::XMMATRIX matShadowToWorld;
    };

    
    struct PSConstantBuffer
    {
        float3_aligned lightPos;
        float3_aligned lightColor;
        float3_aligned viewPosition;
        float3_aligned phongScale;
    };

    virtual void SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const;

protected:
    void InitConstantBuffers();
    void InitSampler(ScopedObject<ID3D11SamplerState>& samplerState, D3D11_FILTER filter,
        D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_COMPARISON_FUNC comparisonFunc = D3D11_COMPARISON_NEVER) const;

    ScopedObject<ID3D11Buffer> m_VSConstantBuffer;
    ScopedObject<ID3D11Buffer> m_PSConstantBuffer;

    ScopedObject<ID3D11RasterizerState> m_RasterizerState;

    std::shared_ptr<VertexShader> m_VertexShader;
    std::shared_ptr<PixelShader>  m_PixelShader;


};

class WorldMaterial : public Material
{
public:
    WorldMaterial(FILE* file);
    virtual void SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const;
   
private:
    std::shared_ptr<Texture> m_Albedo;
    std::shared_ptr<Texture> m_Normal;
    std::shared_ptr<Texture> m_Specular;
    std::shared_ptr<Texture> m_ShadowDepth;
    ScopedObject<ID3D11SamplerState> m_SamplerLinear;
    ScopedObject<ID3D11SamplerState> m_SamplerShadowDepth;

};

class SkyMaterial : public Material
{
public:
    SkyMaterial(FILE* file);
    virtual void SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const;

private:
    std::shared_ptr<Texture> m_Albedo;
    ScopedObject<ID3D11SamplerState> m_SamplerLinear;
};

class DepthMaterial : public Material
{
public:
    DepthMaterial();
    
};

class DebugMaterial : public Material
{
public:
    DebugMaterial(std::shared_ptr<VertexShader> vs, std::shared_ptr<PixelShader> ps);
        
};

class MaterialSystem
{
public:
    void Init();
    std::shared_ptr<Material>       FindMaterial(const char* filename);
    std::shared_ptr<Texture>        FindTexture(const char* filename, TextureGroup_t textureGroup = TEXTURE_GROUP_DIFFUSE);
    std::shared_ptr<VertexShader>   FindVertexShader(const char* filename);
    std::shared_ptr<PixelShader>    FindPixelShader(const char* filename);
    std::shared_ptr<Texture>        CreateRenderableTexture(unsigned int width, unsigned int height, const char* filename);
        
private:
    std::map<std::string, std::shared_ptr<Material> >     m_Materials;
    std::map<std::string, std::shared_ptr<Texture> >      m_Textures;
    std::map<std::string, std::shared_ptr<VertexShader> > m_VertexShaders;
    std::map<std::string, std::shared_ptr<PixelShader> >  m_PixelShaders;
    
};

extern MaterialSystem* materials;

#endif // MATERIALSYSTEM_HPP
