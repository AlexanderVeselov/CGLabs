#ifndef MESH_HPP
#define MESH_HPP

#include "render.hpp"
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

struct Vertex
{
    Vertex(float x, float y, float z) : position(x, y, z), texcoord(0, 0), normal(0, 0, 0) {}
    Vertex(float x, float y, float z, float u, float v) : position(x, y, z), texcoord(u, v), normal(0, 0, 0) {}
    Vertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) : position(x, y, z), texcoord(u, v), normal(nx, ny, nz) {}
    Vertex(const DirectX::XMFLOAT3& position) : position(position), texcoord(0, 0), normal(0, 0, 0) {}
    Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& texcoord, const DirectX::XMFLOAT3& normal) : position(position), texcoord(texcoord), normal(normal) {}

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texcoord;
    DirectX::XMFLOAT3 normal;

};

class Mesh
{
public:
    static Mesh CreateCylinder(int sides = 16, float height = 2.0f);
    static Mesh CreateTorus(int segments = 16, int sides = 16, float radius1 = 2.0f, float radius2 = 0.5f);
    static Mesh CreateSphere(int segments = 16, float radius = 1.0f);
    static Mesh CreateCone(int sides = 16, float radius = 1.0f, float height = 2.0f);
    static Mesh CreateTetrahedron(float size = 4.0f);

    Mesh(const char* filename);
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
    const std::vector<unsigned int>& GetIndices() const { return m_Indices; }

    void Draw() const;

private:
    void InitBuffers();

    ID3D11Buffer* m_VertexBuffer;
    ID3D11Buffer* m_IndexBuffer;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;

};

class MorphedMesh
{
public:
    MorphedMesh() : m_VertexBuffer(nullptr), m_IndexBuffer(nullptr)
    {
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.ByteWidth = sizeof(Vertex) * 32 * 32;
        vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        render->GetDevice()->CreateBuffer(&vertexBufferDesc, nullptr, &m_VertexBuffer);

        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
        indexBufferDesc.ByteWidth = sizeof(unsigned int) * 32 * 32 * 6;
        indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        render->GetDevice()->CreateBuffer(&indexBufferDesc, nullptr, &m_IndexBuffer);
    }

    void Draw(float morphFactor);

private:

    ID3D11Buffer* m_VertexBuffer;
    ID3D11Buffer* m_IndexBuffer;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;

};

#endif // MESH_HPP
