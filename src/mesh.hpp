#ifndef MESH_HPP
#define MESH_HPP

#include "render.hpp"
#include "utils.hpp"
#include "mathlib.hpp"
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

struct Vertex
{
    Vertex(const float3& position, const float2& texcoord, const float3& normal)
        : position(position), texcoord(texcoord), normal(normal)
    {}

    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent_s;
    float3 tangent_t;

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
    const DirectX::XMMATRIX& GetModelToWorld() const { return m_ModelToWorld; }

    void Draw(bool drawDepth = false) const;

private:
    void InitBuffers();

    ScopedObject<ID3D11Buffer> m_VertexBuffer;
    ScopedObject<ID3D11Buffer> m_IndexBuffer;
    
    ScopedObject<ID3D11Buffer> m_DbgVertexBuffer_n;
    ScopedObject<ID3D11Buffer> m_DbgVertexBuffer_s;
    ScopedObject<ID3D11Buffer> m_DbgVertexBuffer_t;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    std::vector<MeshGroup_t> m_MeshGroups;

    std::vector<Vertex> m_DbgNormals;
    std::vector<Vertex> m_DbgTangent_s;
    std::vector<Vertex> m_DbgTangent_t;

    DirectX::XMMATRIX m_ModelToWorld;

};

#endif // MESH_HPP
