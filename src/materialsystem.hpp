#ifndef MATERIALSYSTEM_HPP
#define MATERIALSYSTEM_HPP

#include <d3d11.h>

class MaterialSystem
{
public:
    void Init();
private:
    ID3D11InputLayout* m_InputLayout;

};

extern MaterialSystem* materials;

class Shader
{
    Shader(const char* filename);
};

class VertexShader : public Shader
{
    VertexShader(const char* filename);
};

class PixelShader : public Shader
{
    PixelShader(const char* filename);
};

#endif // MATERIALSYSTEM_HPP