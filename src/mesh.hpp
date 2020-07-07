#ifndef MESH_HPP
#define MESH_HPP

#include "render.hpp"
#include "utils.hpp"
#include "mathlib.hpp"
#include <d3d11.h>
#include <vector>

class Material;

struct MeshGroup_t
{
    unsigned int startIndex;
    unsigned int indexCount;
    char materialName[56]; // Align to 64 bytes

};

class Mesh
{
public:
    Mesh() {}
    Mesh(const char* filename, const char* mtldir = nullptr, const Matrix& modelToWorld = Matrix::Identity(), bool castShadow = true);
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    const void SetTransform(const Matrix& transform) { m_ModelToWorld = transform; }
    const Matrix& GetModelToWorld() const { return m_ModelToWorld; }

    virtual void Draw(bool drawDepth = false);

protected:
    void InitBuffers();
    void LoadFromObj(const char* filename);
    void LoadFromDat(const char* filename);
    void SaveToDat(const char* filename) const;

    ScopedObject<ID3D11Buffer> m_VertexBuffer;
    ScopedObject<ID3D11Buffer> m_IndexBuffer;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    std::vector<MeshGroup_t> m_MeshGroups;

    Matrix m_ModelToWorld;

    bool m_CastShadow;
    
};

struct Plane_t
{
    float3 normal;
    float  dist;
};

struct Neighbor_t
{
    unsigned int sideIndex;
    unsigned int edgeIndex;
};

struct Side_t
{
    Plane_t plane;
    float3 center;
    std::vector<float3> positions;
    std::vector<Neighbor_t> neighbors;

};


class AnimatedPolyhedron : public Mesh
{
public:
    AnimatedPolyhedron(float3 origin);
    virtual void Draw(bool drawDepth = false);

private:
    std::vector<unsigned int> m_Edges;
    std::vector<Side_t> m_Sides;
    float3 m_Velocity;
    double m_PlaneAngle;
    double m_CurrentAngle;
    unsigned int m_CurrentSide;
    unsigned int m_CurrentEdge;
    float3 v1, v2;

};

#endif // MESH_HPP
