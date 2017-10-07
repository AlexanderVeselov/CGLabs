#ifndef MATERIALSYSTEM_HPP
#define MATERIALSYSTEM_HPP

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