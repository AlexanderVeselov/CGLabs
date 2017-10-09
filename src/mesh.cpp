#include "mesh.hpp"
#include "mathlib.hpp"
#include "render.hpp"
#include "materialsystem.hpp"
#include "utils.hpp"
#include <cctype>
#include <fstream>
#include <string>


class FileReader
{
public:
    FileReader(const char* filename) : m_InputFile(filename), m_VectorValue(0.0f)
    {
        if (!m_InputFile)
        {
            THROW_RUNTIME("Failed to load object file " << filename)

        }
    }

    std::string GetStringValue() const
    {
        return m_StringValue;
    }

    float3 GetVectorValue() const
    {
        return m_VectorValue;
    }

protected:
    float ReadFloatValue()
    {
        SkipSpaces();
        float value = 0.0f;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;
        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10.0f + (float)((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        if (m_CurrentLine[m_CurrentChar++] == '.')
        {
            size_t frac = 1;
            while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
            {
                value += (float)((int)m_CurrentLine[m_CurrentChar++] - 48) / (pow(10.0f, frac++));
            }
        }

        return minus ? -value : value;

    }

    int ReadIntValue()
    {
        SkipSpaces();
        int value = 0;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;

        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10 + ((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        return minus ? -value : value;
    }

    unsigned int ReadUintValue()
    {
        SkipSpaces();
        unsigned int value = 0;

        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10 + ((unsigned int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        return value;
    }

    void ReadVectorValue()
    {
        m_VectorValue.x = ReadFloatValue();
        m_VectorValue.y = ReadFloatValue();
        m_VectorValue.z = ReadFloatValue();

    }

    void ReadStringValue()
    {
        SkipSpaces();
        m_StringValue.clear();

        if (m_CurrentChar < m_CurrentLine.size() && IsIdentifierStart(m_CurrentLine[m_CurrentChar]))
        {
            m_StringValue += m_CurrentLine[m_CurrentChar++];
            while (m_CurrentChar < m_CurrentLine.size() && IsIdentifierBody(m_CurrentLine[m_CurrentChar]))
            {
                m_StringValue += m_CurrentLine[m_CurrentChar++];
            }
        }

    }

    bool ReadLine()
    {
        do
        {
            if (!std::getline(m_InputFile, m_CurrentLine))
            {
                return false;
            }
        } while (m_CurrentLine.size() == 0);

        m_CurrentChar = 0;
        return true;
    }

    void SkipSpaces()
    {
        while (std::isspace(m_CurrentLine[m_CurrentChar])) { m_CurrentChar++; }
    }

    void SkipSymbol(char symbol)
    {
        SkipSpaces();
        while (m_CurrentLine[m_CurrentChar] == symbol) { m_CurrentChar++; }
    }

    bool IsIdentifierStart(char symbol)
    {
        return symbol >= 'a' && symbol <= 'z' ||
            symbol >= 'A' && symbol <= 'Z' || symbol == '_';
    }

    bool IsIdentifierBody(char symbol)
    {
        return IsIdentifierStart(symbol) || symbol >= '0' && symbol <= '9';
    }

private:
    std::ifstream m_InputFile;
    std::string m_CurrentLine;
    std::string m_StringValue;
    float3 m_VectorValue;
    size_t m_CurrentChar;

};

class ObjReader : public FileReader
{
public:
    enum ObjToken_t
    {
        OBJ_MTLLIB,
        OBJ_USEMTL,
        OBJ_POSITION,
        OBJ_TEXCOORD,
        OBJ_NORMAL,
        OBJ_FACE,
        OBJ_SMOOTHINGGROUP,
        OBJ_INVALID,
        OBJ_EOF
    };

    ObjReader(const char* filename) : FileReader(filename)
    {
    }

    ObjToken_t NextToken()
    {
        if (!ReadLine())
        {
            return OBJ_EOF;
        }

        ReadStringValue();
        if (GetStringValue() == "mtllib")
        {
            ReadStringValue();
            return OBJ_MTLLIB;
        }
        else if (GetStringValue() == "usemtl")
        {
            ReadStringValue();
            return OBJ_USEMTL;
        }
        else if (GetStringValue() == "v")
        {
            ReadVectorValue();
            return OBJ_POSITION;
        }
        else if (GetStringValue() == "vn")
        {
            ReadVectorValue();
            return OBJ_NORMAL;
        }
        else if (GetStringValue() == "vt")
        {
            ReadVectorValue();
            return OBJ_TEXCOORD;
        }
        else if (GetStringValue() == "f")
        {
            for (size_t i = 0; i < 3; ++i)
            {
                m_VertexIndices[i] = (unsigned int)ReadIntValue() - 1;
                SkipSymbol('/');
                m_TexcoordIndices[i] = (unsigned int)ReadIntValue() - 1;
                SkipSymbol('/');
                m_NormalIndices[i] = (unsigned int)ReadIntValue() - 1;

            }
            return OBJ_FACE;
        }
        else if (GetStringValue() == "s")
        {
            m_UintValue = ReadUintValue();
            return OBJ_SMOOTHINGGROUP;
        }
        else
        {
            return OBJ_INVALID;
        }

    }

    void GetFaceIndices(const unsigned int** iv, const unsigned int** it, const unsigned int** in) const
    {
        *iv = m_VertexIndices;
        *it = m_TexcoordIndices;
        *in = m_NormalIndices;

    }

    int GetUintValue() const
    {
        return m_UintValue;
    }

private:
    // Face indices
    unsigned int m_VertexIndices[3];
    unsigned int m_TexcoordIndices[3];
    unsigned int m_NormalIndices[3];
    unsigned int m_UintValue;

};

void Mesh::InitBuffers()
{
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * m_Vertices.size();
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexBufferData;
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = m_Vertices.data();

    render->GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertexBuffer);

    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned int) * m_Indices.size();
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexBufferData;
    ZeroMemory(&indexBufferData, sizeof(indexBufferData));
    indexBufferData.pSysMem = m_Indices.data();
    render->GetDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexBuffer);

}

#include <map>

// CAREFULLY...
unsigned int HashIntPair(int a, int b)
{
    assert(a >= 0 && b >= 0);
    return (a + b) * (a + b + 1) / 2 + a;
}

unsigned int HashIntTriple(int a, int b, int c)
{
    return HashIntPair(a, HashIntPair(b, c));
}

// Load from .obj
Mesh::Mesh(const char* filename) : m_ModelToWorld(DirectX::XMMatrixIdentity())
{
    ObjReader objReader(filename);

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texcoords;
    unsigned int newIndex = 0;
    unsigned int existingIndex = 0;

    std::string materialName;
    ObjReader::ObjToken_t token;
    
    std::map<unsigned int, unsigned int> indexDictionary;
    std::map<unsigned int, unsigned int>::iterator iter;

    MeshGroup_t meshGroup;
    meshGroup.startIndex = 0;
    meshGroup.indexCount = 0;
    meshGroup.material = nullptr;

    while ((token = objReader.NextToken()) != ObjReader::OBJ_EOF)
    {
        switch (token)
        {
        case ObjReader::OBJ_USEMTL:
            if (meshGroup.indexCount > 0)
            {
                m_MeshGroups.push_back(meshGroup);
            }
            meshGroup.material = materials->FindMaterial(objReader.GetStringValue().c_str());
            meshGroup.startIndex = m_Indices.size();
            meshGroup.indexCount = 0;
            break;
        case ObjReader::OBJ_POSITION:
            positions.push_back(DirectX::XMFLOAT3(objReader.GetVectorValue().x, objReader.GetVectorValue().y, objReader.GetVectorValue().z));
            //m_Vertices.push_back(Vertex(positions.back()));
            break;
        case ObjReader::OBJ_NORMAL:
            normals.push_back(DirectX::XMFLOAT3(objReader.GetVectorValue().x, objReader.GetVectorValue().y, objReader.GetVectorValue().z));
            break;
        case ObjReader::OBJ_TEXCOORD:
            texcoords.push_back(DirectX::XMFLOAT2(1.0f - objReader.GetVectorValue().x, 1.0f - objReader.GetVectorValue().y));
            break;
        case ObjReader::OBJ_FACE:
            const unsigned int *iv, *it, *in;
            objReader.GetFaceIndices(&iv, &it, &in);

            // Re-index Mesh
            for (size_t i = 0; i < 3; ++i)
            {
                unsigned int triple = HashIntTriple(iv[i], it[i], in[i]);
                if ((iter = indexDictionary.find(triple)) != indexDictionary.end())
                {
                    m_Indices.push_back(iter->second);
                }
                else
                {
                    indexDictionary[triple] = newIndex;
                    m_Vertices.push_back(Vertex(positions[iv[i]], texcoords[it[i]], normals[in[i]]));
                    m_Indices.push_back(newIndex);
                    ++newIndex;
                }
                ++meshGroup.indexCount;
            }
            break;
        case ObjReader::OBJ_SMOOTHINGGROUP:
            break;
        }
    }

    // No material information in .obj file
    if (meshGroup.material == nullptr)
    {
        meshGroup.material = materials->FindMaterial("debug_checker");
    }
    m_MeshGroups.push_back(meshGroup);
    
    InitBuffers();

}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : m_Vertices(vertices), m_Indices(indices)
{
    InitBuffers();
}

void Mesh::Draw() const
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    render->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    render->GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    const ViewSetup* view = render->GetCurrentView();
    Material::VSConstantBuffer vscb;
    vscb.matViewProjection = DirectX::XMMatrixTranspose(view->matView * view->matProjection);
    vscb.matWorld = m_ModelToWorld;
    Material::PSConstantBuffer pscb;
    pscb.viewPosition = view->origin;

    for (unsigned int i = 0; i < m_MeshGroups.size(); ++i)
    {
        const MeshGroup_t& meshGroup = m_MeshGroups[i];
        meshGroup.material->SetMaterial(vscb, pscb);
        render->GetDeviceContext()->DrawIndexed(meshGroup.indexCount, meshGroup.startIndex, 0);

    }

}

Mesh Mesh::CreateCylinder(int sides, float height)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // index 0
    vertices.push_back(Vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
    // index 1
    vertices.push_back(Vertex(0.0f, 0.0f, height, 1.0f, 1.0f));
    for (int i = 0; i < sides; ++i)
    {
        // index i + 2
        vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * i / sides), std::sinf(DirectX::XM_2PI * i / sides), 0.0f, 1.0f - (float)i / sides, 1));
        // index i + 3
        vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * i / sides), std::sinf(DirectX::XM_2PI * i / sides), height, 1.0f - (float)i / sides, 0));

        unsigned int imul2plus2 = (i == sides - 1) ? 0 : (i * 2 + 2);

        indices.push_back(i * 2 + 2);
        indices.push_back(0);
        indices.push_back(imul2plus2 + 2);

        indices.push_back(i * 2 + 3);
        indices.push_back(i * 2 + 2);
        indices.push_back(imul2plus2 + 2);

        indices.push_back(i * 2 + 3);
        indices.push_back(imul2plus2 + 2);
        indices.push_back(imul2plus2 + 3);

        indices.push_back(1);
        indices.push_back(i * 2 + 3);
        indices.push_back(imul2plus2 + 3);
    }


    return Mesh(vertices, indices);
}

Mesh Mesh::CreateTorus(int segments, int sides, float radius1, float radius2)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i < segments; ++i)
    {
        //DirectX::XMMATRIX worldToSegment1 = DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);
        //worldToSegment1 *= DirectX::XMMatrixRotationZ(DirectX::XM_2PI * i / segments);
        //worldToSegment1 *= DirectX::XMMatrixTranslation(std::cosf(DirectX::XM_2PI * i / segments) * radius1, std::sinf(DirectX::XM_2PI * i / segments) * radius1, 0.0f);

        for (int j = 0; j < sides; ++j)
        {
            float theta = DirectX::XM_2PI * i / segments;
            float phi = DirectX::XM_2PI * j / sides;

            //vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * j / sides) * radius2, std::sinf(DirectX::XM_2PI * j / sides) * radius2, 0.0f, worldToSegment1));
            vertices.push_back(Vertex((radius1 + radius2 * std::cosf(theta)) * std::cosf(phi), (radius1 + radius2 * std::cosf(theta)) * std::sinf(phi), radius2 * std::sinf(theta), (float)i / segments, (float)j / sides));

            unsigned int iplus1 = (i == segments - 1) ? 0 : (i + 1);
            unsigned int jplus1 = (j == sides - 1) ? 0 : (j + 1);

            indices.push_back(iplus1 * sides + j);
            indices.push_back(i * sides + j);
            indices.push_back(i * sides + jplus1);

            indices.push_back(iplus1 * sides + j);
            indices.push_back(i * sides + jplus1);
            indices.push_back(iplus1 * sides + jplus1);

        }
    }

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateSphere(int segments, float radius)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    vertices.push_back(Vertex(0.0f, 0.0f, radius));
    vertices.push_back(Vertex(0.0f, 0.0f, -radius));
    for (int i = 0; i < segments - 1; ++i)
    {
        for (int j = 0; j < segments; ++j)
        {
            float theta = DirectX::XM_PI * (i + 1) / segments;
            float phi = DirectX::XM_2PI * j / segments;

            vertices.push_back(Vertex(std::cosf(phi) * std::sinf(theta) * radius, std::sinf(phi) * std::sinf(theta) * radius, std::cosf(theta) * radius));

            unsigned int jplus1 = (j == segments - 1) ? 0 : (j + 1);
            if (i == 0)
            {
                indices.push_back(0);
                indices.push_back(j + 2);
                indices.push_back(jplus1 + 2);
            }
            if (i < segments - 2)
            {
                indices.push_back(i * segments + j + 2);
                indices.push_back((i + 1) * segments + j + 2);
                indices.push_back(i * segments + jplus1 + 2);

                indices.push_back((i + 1) * segments + j + 2);
                indices.push_back((i + 1) * segments + jplus1 + 2);
                indices.push_back(i * segments + jplus1 + 2);
            }
            if (i == segments - 2)
            {
                indices.push_back(1);
                indices.push_back(i * segments + j + 2);
                indices.push_back(i * segments + jplus1 + 2);
            }

        }
    }

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateCone(int sides, float radius, float height)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // index 0
    vertices.push_back(Vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
    // index 1
    vertices.push_back(Vertex(0.0f, 0.0f, height, 1.0f, 1.0f));
    for (int i = 0; i < sides; ++i)
    {
        // index i + 2
        vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * i / sides), std::sinf(DirectX::XM_2PI * i / sides), 0.0f, 1.0f - (float)i / sides, 1));
        
        unsigned int iplus2 = (i == sides - 1) ? 0 : (i + 1);

        indices.push_back(i + 2);
        indices.push_back(0);
        indices.push_back(iplus2 + 2);

        indices.push_back(1);
        indices.push_back(i + 2);
        indices.push_back(iplus2 + 2);
    }


    return Mesh(vertices, indices);
}

Mesh Mesh::CreateTetrahedron(float size)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float sqrt3 = 1.732f;
    float sqrt6 = 2.4494f;
    
    vertices.push_back(Vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
    vertices.push_back(Vertex(size, 0.0f, 0.0f, size, 0.0f));
    vertices.push_back(Vertex(0.5f * size, sqrt3 / 2.0f * size, 0.0f, 1.0f, 1.0f));
    vertices.push_back(Vertex(0.5f * size, sqrt3 / 6.0f * size, sqrt6 / 3.0f * size, 0.5f, 1.0f));

    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(1);
    
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(3);
    
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(3);

    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(3);

    return Mesh(vertices, indices);
}

Vertex VertexLerp(const Vertex& v1, const Vertex& v2, float factor)
{
    return Vertex(v1.position.x * (1 - factor) + v2.position.x * factor,
        v1.position.y * (1 - factor) + v2.position.y * factor,
        v1.position.z * (1 - factor) + v2.position.z * factor,
        v1.texcoord.x * (1 - factor) + v2.texcoord.x * factor,
        v1.texcoord.y * (1 - factor) + v2.texcoord.y * factor);

}

/*
void MorphedMesh::Draw(float morphFactor)
{
    assert(morphFactor >= 0.0f && morphFactor <= 1.0f);

    m_Vertices.clear();
    m_Indices.clear();

    int segments = 32;
    int sides = 32;
    float radius1 = 2.0f;
    float radius2 = 0.5f;

    for (int i = 0; i < segments; ++i)
    {
        for (int j = 0; j < sides; ++j)
        {
            float theta1 = -DirectX::XM_2PI * i / (segments - 1) + 2 * DirectX::XM_PIDIV2;
            float phi1 = -DirectX::XM_2PI * j / sides;

            float theta2 = DirectX::XM_PI * i / (segments - 1);
            float phi2 = -DirectX::XM_2PI * j / sides;

            Vertex v1((radius1 + radius2 * std::cosf(theta1)) * std::cosf(phi1), (radius1 + radius2 * std::cosf(theta1)) * std::sinf(phi1), radius2 * std::sinf(theta1), (float)i / segments, (float)j / sides);
            Vertex v2(std::cosf(phi2) * std::sinf(theta2) * radius1, std::sinf(phi2) * std::sinf(theta2) * radius1, std::cosf(theta2) * radius1, (float)i / segments, (float)j / sides);
            m_Vertices.push_back(VertexLerp(v1, v2, morphFactor));

            if (i < sides - 1)
            {
                unsigned int jplus1 = (j == sides - 1) ? 0 : (j + 1);
                m_Indices.push_back((i + 1) * sides + j);
                m_Indices.push_back(i * sides + j);
                m_Indices.push_back(i * sides + jplus1);

                m_Indices.push_back((i + 1) * sides + j);
                m_Indices.push_back(i * sides + jplus1);
                m_Indices.push_back((i + 1) * sides + jplus1);
            }
        }
    }

    D3D11_MAPPED_SUBRESOURCE mappedVertices;
    render->GetDeviceContext()->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertices);
    memcpy(mappedVertices.pData, m_Vertices.data(), sizeof(Vertex) * m_Vertices.size());
    render->GetDeviceContext()->Unmap(m_VertexBuffer, 0);

    D3D11_MAPPED_SUBRESOURCE mappedIndices;
    render->GetDeviceContext()->Map(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedIndices);
    memcpy(mappedIndices.pData, m_Indices.data(), sizeof(unsigned int) * m_Indices.size());
    render->GetDeviceContext()->Unmap(m_IndexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    render->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    render->GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    render->GetDeviceContext()->DrawIndexed(m_Indices.size(), 0, 0);

}
*/