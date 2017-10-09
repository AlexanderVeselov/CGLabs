#ifndef MATERIALSYSTEM_HPP
#define MATERIALSYSTEM_HPP

#include "mathlib.hpp"
#include <map>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>

class VertexShader
{
public:
    VertexShader() {}
    VertexShader(const char* filename);
    void Set() const;

private:
    ID3D11VertexShader* m_VertexShader;
    ID3D11InputLayout* m_InputLayout;

};

class PixelShader
{
public:
    PixelShader() {}
    PixelShader(const char* filename);
    void Set() const;

private:
    ID3D11PixelShader* m_PixelShader;

};

class Texture
{
public:
    Texture() {}
    Texture(const char* filename);
    void Set(unsigned int slot = 0) const;

private:
    ID3D11ShaderResourceView* m_TextureView;
    std::string name;

};

class Material
{
public:

    struct VSConstantBuffer
    {
        DirectX::XMMATRIX matWorld;
        DirectX::XMMATRIX matViewProjection;
    };

    struct PSConstantBuffer
    {
        float3 lightPositions[3];
        float3 lightColors[3];
        float3 viewPosition;
        float3 phongScale;
    };

    virtual void SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const = 0;

protected:
    VertexShader* m_VertexShader;
    PixelShader*  m_PixelShader;

    ID3D11Buffer* m_VSConstantBuffer;
    ID3D11Buffer* m_PSConstantBuffer;

    ID3D11RasterizerState* m_RasterizerState;

};

class WorldMaterial : public Material
{
public:
    WorldMaterial(FILE* file);
    virtual void SetMaterial(const VSConstantBuffer& vsBuffer, const PSConstantBuffer& psBuffer) const;
   
private:
    Texture* m_Albedo;
    ID3D11SamplerState* m_SamplerState;

};

class MaterialSystem
{
public:
    void Init();
    std::shared_ptr<Material> FindMaterial(const char* filename);
    Texture* FindTexture(const char* filename);
    VertexShader* FindVertexShader(const char* filename);
    PixelShader* FindPixelShader(const char* filename);
        
private:
    std::map<std::string, std::shared_ptr<Material> > m_Materials;
    std::map<std::string, Texture>  m_Textures;
    std::map<std::string, VertexShader> m_VertexShaders;
    std::map<std::string, PixelShader>  m_PixelShaders;
    
};

extern MaterialSystem* materials;

#endif // MATERIALSYSTEM_HPP
