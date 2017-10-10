#ifndef MESH_HPP
#define MESH_HPP

#include "render.hpp"
#include "utils.hpp"
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

class Material;

struct MeshGroup_t
{
    unsigned int startIndex;
    unsigned int indexCount;
    std::shared_ptr<Material> material;

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
    const DirectX::XMMATRIX& GetModelToWorld() const { return m_ModelToWorld; }

    void Draw(bool depth = false) const;

private:
    void InitBuffers();

    ScopedObject<ID3D11Buffer> m_VertexBuffer;
    ScopedObject<ID3D11Buffer> m_IndexBuffer;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    std::vector<MeshGroup_t> m_MeshGroups;

    DirectX::XMMATRIX m_ModelToWorld;

};

#endif // MESH_HPP
