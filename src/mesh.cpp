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

#ifdef DEBUG_TANGENTS
    D3D11_BUFFER_DESC dbgVertexBufferDesc;
    ZeroMemory(&dbgVertexBufferDesc, sizeof(dbgVertexBufferDesc));

    dbgVertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    dbgVertexBufferDesc.ByteWidth = sizeof(Vertex) * m_DbgNormals.size();
    dbgVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA dbgVertexBufferData;
    ZeroMemory(&dbgVertexBufferData, sizeof(dbgVertexBufferData));
    dbgVertexBufferData.pSysMem = m_DbgNormals.data();
    render->GetDevice()->CreateBuffer(&dbgVertexBufferDesc, &dbgVertexBufferData, &m_DbgVertexBuffer_n);
    dbgVertexBufferData.pSysMem = m_DbgTangent_s.data();
    render->GetDevice()->CreateBuffer(&dbgVertexBufferDesc, &dbgVertexBufferData, &m_DbgVertexBuffer_s);
    dbgVertexBufferData.pSysMem = m_DbgTangent_t.data();
    render->GetDevice()->CreateBuffer(&dbgVertexBufferDesc, &dbgVertexBufferData, &m_DbgVertexBuffer_t);
#endif

}

#include <unordered_map>

std::string HashIntTriple(unsigned int a, unsigned int b, unsigned int c)
{
    std::stringstream ss;
    ss << a << " " << b << " " << c;
    return ss.str();
}

void ComputeTangentSpace(Vertex& v1, Vertex& v2, Vertex& v3)
{
    const float3& v1p = v1.position;
    const float3& v2p = v2.position;
    const float3& v3p = v3.position;

    const float2& v1t = v1.texcoord;
    const float2& v2t = v2.texcoord;
    const float2& v3t = v3.texcoord;

    double x1 = v2p.x - v1p.x;
    double x2 = v3p.x - v1p.x;
    double y1 = v2p.y - v1p.y;
    double y2 = v3p.y - v1p.y;
    double z1 = v2p.z - v1p.z;
    double z2 = v3p.z - v1p.z;

    double s1 = v2t.x - v1t.x;
    double s2 = v3t.x - v1t.x;
    double t1 = v2t.y - v1t.y;
    double t2 = v3t.y - v1t.y;

    double r = 1.0 / (s1 * t2 - s2 * t1);
    float3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
    float3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

    v1.tangent_s += sdir;
    v2.tangent_s += sdir;
    v3.tangent_s += sdir;

    v1.tangent_t += tdir;
    v2.tangent_t += tdir;
    v3.tangent_t += tdir;

}

// Load from .obj
Mesh::Mesh(const char* filename) : m_ModelToWorld(DirectX::XMMatrixIdentity())
{
    ObjReader objReader(filename);

    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float2> texcoords;
    unsigned int newIndex = 0;
    unsigned int existingIndex = 0;

    std::string materialName;
    ObjReader::ObjToken_t token;
    
    std::unordered_map<std::string, unsigned int> indexDictionary;
    std::unordered_map<std::string, unsigned int>::iterator iter;

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
            positions.push_back(float3(objReader.GetVectorValue().x, objReader.GetVectorValue().y, objReader.GetVectorValue().z));
            //m_Vertices.push_back(Vertex(positions.back()));
            break;
        case ObjReader::OBJ_NORMAL:
            normals.push_back(float3(objReader.GetVectorValue().x, objReader.GetVectorValue().y, objReader.GetVectorValue().z));
            break;
        case ObjReader::OBJ_TEXCOORD:
            texcoords.push_back(float2(1.0f - objReader.GetVectorValue().x, 1.0f - objReader.GetVectorValue().y));
            break;
        case ObjReader::OBJ_FACE:
            const unsigned int *iv, *it, *in;
            objReader.GetFaceIndices(&iv, &it, &in);

            // Re-index Mesh
            for (size_t i = 0; i < 3; ++i)
            {
                std::string triple = HashIntTriple(iv[i], it[i], in[i]);
                if ((iter = indexDictionary.find(triple)) != indexDictionary.end())
                {
                    m_Indices.push_back(iter->second);
                }
                else
                {
                    m_Vertices.push_back(Vertex(positions[iv[i]], texcoords[it[i]], normals[in[i]]));
                    indexDictionary[triple] = newIndex;
                    m_Indices.push_back(newIndex++);
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
    
    for (unsigned int i = 0; i < m_Indices.size(); i += 3)
    {
        unsigned int i1 = m_Indices[i];
        unsigned int i2 = m_Indices[i + 1];
        unsigned int i3 = m_Indices[i + 2];

        ComputeTangentSpace(m_Vertices[i1], m_Vertices[i2], m_Vertices[i3]);

    }

    for (unsigned int i = 0; i < m_Vertices.size(); ++i)
    {
        float3& n = m_Vertices[i].normal;
        const float3& t = m_Vertices[i].tangent_t;

        m_Vertices[i].tangent_t = (t - n * dot(n, t)).normalize();
        m_Vertices[i].tangent_s = cross(m_Vertices[i].tangent_t, m_Vertices[i].normal);

    }
#ifdef DEBUG_TANGENTS    for (unsigned int i = 0; i < m_Vertices.size(); ++i)    {
        m_DbgNormals.push_back(Vertex(m_Vertices[i].position, m_Vertices[i].texcoord, m_Vertices[i].normal));
        m_DbgNormals.push_back(Vertex(m_Vertices[i].position + m_Vertices[i].normal, m_Vertices[i].texcoord, m_Vertices[i].normal));        
        m_DbgTangent_s.push_back(Vertex(m_Vertices[i].position, m_Vertices[i].texcoord, m_Vertices[i].normal));
        m_DbgTangent_s.push_back(Vertex(m_Vertices[i].position + m_Vertices[i].tangent_s, m_Vertices[i].texcoord, m_Vertices[i].normal));

        m_DbgTangent_t.push_back(Vertex(m_Vertices[i].position, m_Vertices[i].texcoord, m_Vertices[i].normal));
        m_DbgTangent_t.push_back(Vertex(m_Vertices[i].position + m_Vertices[i].tangent_t, m_Vertices[i].texcoord, m_Vertices[i].normal));    }
#endif

    InitBuffers();

}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : m_Vertices(vertices), m_Indices(indices)
{
    InitBuffers();
}

void Mesh::Draw(bool drawDepth) const
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    const ViewSetup* view = render->GetCurrentView();
    Material::VSConstantBuffer vscb;
    vscb.matWorldToCamera = DirectX::XMMatrixTranspose(view->matWorldToCamera);
    vscb.matModelToWorld = m_ModelToWorld;
    Material::PSConstantBuffer pscb;
    pscb.viewPosition = view->origin;

    render->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    render->GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    render->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (drawDepth)
    {
        materials->FindMaterial("depth")->SetMaterial(vscb, pscb);
        render->GetDeviceContext()->DrawIndexed(m_Indices.size(), 0, 0);
    }
    else
    {
        const ViewSetup* shadowView = render->GetPreviousView();
        vscb.matShadowToWorld = DirectX::XMMatrixTranspose(shadowView->matWorldToCamera);
        pscb.lightPositions[0] = shadowView->origin;
        for (unsigned int i = 0; i < m_MeshGroups.size(); ++i)
        {
            const MeshGroup_t& meshGroup = m_MeshGroups[i];
            meshGroup.material->SetMaterial(vscb, pscb);
            render->GetDeviceContext()->DrawIndexed(meshGroup.indexCount, meshGroup.startIndex, 0);
        }

#ifdef DEBUG_TANGENTS
        materials->FindMaterial("debug_normal")->SetMaterial(vscb, pscb);
        render->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_DbgVertexBuffer_n, &stride, &offset);
        render->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        render->GetDeviceContext()->Draw(m_DbgNormals.size(), 0);

        materials->FindMaterial("debug_tangent_s")->SetMaterial(vscb, pscb);
        render->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        render->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_DbgVertexBuffer_s, &stride, &offset);
        render->GetDeviceContext()->Draw(m_DbgTangent_s.size(), 0);

        materials->FindMaterial("debug_tangent_t")->SetMaterial(vscb, pscb);
        render->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        render->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_DbgVertexBuffer_t, &stride, &offset);
        render->GetDeviceContext()->Draw(m_DbgTangent_t.size(), 0);
#endif
    }


}
//
//Mesh Mesh::CreateCylinder(int sides, float height)
//{
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//
//    // index 0
//    vertices.push_back(Vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
//    // index 1
//    vertices.push_back(Vertex(0.0f, 0.0f, height, 1.0f, 1.0f));
//    for (int i = 0; i < sides; ++i)
//    {
//        // index i + 2
//        vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * i / sides), std::sinf(DirectX::XM_2PI * i / sides), 0.0f, 1.0f - (float)i / sides, 1));
//        // index i + 3
//        vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * i / sides), std::sinf(DirectX::XM_2PI * i / sides), height, 1.0f - (float)i / sides, 0));
//
//        unsigned int imul2plus2 = (i == sides - 1) ? 0 : (i * 2 + 2);
//
//        indices.push_back(i * 2 + 2);
//        indices.push_back(0);
//        indices.push_back(imul2plus2 + 2);
//
//        indices.push_back(i * 2 + 3);
//        indices.push_back(i * 2 + 2);
//        indices.push_back(imul2plus2 + 2);
//
//        indices.push_back(i * 2 + 3);
//        indices.push_back(imul2plus2 + 2);
//        indices.push_back(imul2plus2 + 3);
//
//        indices.push_back(1);
//        indices.push_back(i * 2 + 3);
//        indices.push_back(imul2plus2 + 3);
//    }
//
//
//    return Mesh(vertices, indices);
//}
//
//Mesh Mesh::CreateTorus(int segments, int sides, float radius1, float radius2)
//{
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//
//    for (int i = 0; i < segments; ++i)
//    {
//        //DirectX::XMMATRIX worldToSegment1 = DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);
//        //worldToSegment1 *= DirectX::XMMatrixRotationZ(DirectX::XM_2PI * i / segments);
//        //worldToSegment1 *= DirectX::XMMatrixTranslation(std::cosf(DirectX::XM_2PI * i / segments) * radius1, std::sinf(DirectX::XM_2PI * i / segments) * radius1, 0.0f);
//
//        for (int j = 0; j < sides; ++j)
//        {
//            float theta = DirectX::XM_2PI * i / segments;
//            float phi = DirectX::XM_2PI * j / sides;
//
//            //vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * j / sides) * radius2, std::sinf(DirectX::XM_2PI * j / sides) * radius2, 0.0f, worldToSegment1));
//            vertices.push_back(Vertex((radius1 + radius2 * std::cosf(theta)) * std::cosf(phi), (radius1 + radius2 * std::cosf(theta)) * std::sinf(phi), radius2 * std::sinf(theta), (float)i / segments, (float)j / sides));
//
//            unsigned int iplus1 = (i == segments - 1) ? 0 : (i + 1);
//            unsigned int jplus1 = (j == sides - 1) ? 0 : (j + 1);
//
//            indices.push_back(iplus1 * sides + j);
//            indices.push_back(i * sides + j);
//            indices.push_back(i * sides + jplus1);
//
//            indices.push_back(iplus1 * sides + j);
//            indices.push_back(i * sides + jplus1);
//            indices.push_back(iplus1 * sides + jplus1);
//
//        }
//    }
//
//    return Mesh(vertices, indices);
//}
//
//Mesh Mesh::CreateSphere(int segments, float radius)
//{
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//
//    vertices.push_back(Vertex(0.0f, 0.0f, radius));
//    vertices.push_back(Vertex(0.0f, 0.0f, -radius));
//    for (int i = 0; i < segments - 1; ++i)
//    {
//        for (int j = 0; j < segments; ++j)
//        {
//            float theta = DirectX::XM_PI * (i + 1) / segments;
//            float phi = DirectX::XM_2PI * j / segments;
//
//            vertices.push_back(Vertex(std::cosf(phi) * std::sinf(theta) * radius, std::sinf(phi) * std::sinf(theta) * radius, std::cosf(theta) * radius));
//
//            unsigned int jplus1 = (j == segments - 1) ? 0 : (j + 1);
//            if (i == 0)
//            {
//                indices.push_back(0);
//                indices.push_back(j + 2);
//                indices.push_back(jplus1 + 2);
//            }
//            if (i < segments - 2)
//            {
//                indices.push_back(i * segments + j + 2);
//                indices.push_back((i + 1) * segments + j + 2);
//                indices.push_back(i * segments + jplus1 + 2);
//
//                indices.push_back((i + 1) * segments + j + 2);
//                indices.push_back((i + 1) * segments + jplus1 + 2);
//                indices.push_back(i * segments + jplus1 + 2);
//            }
//            if (i == segments - 2)
//            {
//                indices.push_back(1);
//                indices.push_back(i * segments + j + 2);
//                indices.push_back(i * segments + jplus1 + 2);
//            }
//
//        }
//    }
//
//    return Mesh(vertices, indices);
//}
//
//Mesh Mesh::CreateCone(int sides, float radius, float height)
//{
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//
//    // index 0
//    vertices.push_back(Vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
//    // index 1
//    vertices.push_back(Vertex(0.0f, 0.0f, height, 1.0f, 1.0f));
//    for (int i = 0; i < sides; ++i)
//    {
//        // index i + 2
//        vertices.push_back(Vertex(std::cosf(DirectX::XM_2PI * i / sides), std::sinf(DirectX::XM_2PI * i / sides), 0.0f, 1.0f - (float)i / sides, 1));
//        
//        unsigned int iplus2 = (i == sides - 1) ? 0 : (i + 1);
//
//        indices.push_back(i + 2);
//        indices.push_back(0);
//        indices.push_back(iplus2 + 2);
//
//        indices.push_back(1);
//        indices.push_back(i + 2);
//        indices.push_back(iplus2 + 2);
//    }
//
//
//    return Mesh(vertices, indices);
//}
//
//Mesh Mesh::CreateTetrahedron(float size)
//{
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//
//    float sqrt3 = 1.732f;
//    float sqrt6 = 2.4494f;
//    
//    vertices.push_back(Vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
//    vertices.push_back(Vertex(size, 0.0f, 0.0f, size, 0.0f));
//    vertices.push_back(Vertex(0.5f * size, sqrt3 / 2.0f * size, 0.0f, 1.0f, 1.0f));
//    vertices.push_back(Vertex(0.5f * size, sqrt3 / 6.0f * size, sqrt6 / 3.0f * size, 0.5f, 1.0f));
//
//    indices.push_back(0);
//    indices.push_back(2);
//    indices.push_back(1);
//    
//    indices.push_back(1);
//    indices.push_back(2);
//    indices.push_back(3);
//    
//    indices.push_back(0);
//    indices.push_back(1);
//    indices.push_back(3);
//
//    indices.push_back(2);
//    indices.push_back(0);
//    indices.push_back(3);
//
//    return Mesh(vertices, indices);
//}
//
//Vertex VertexLerp(const Vertex& v1, const Vertex& v2, float factor)
//{
//    return Vertex(v1.position.x * (1 - factor) + v2.position.x * factor,
//        v1.position.y * (1 - factor) + v2.position.y * factor,
//        v1.position.z * (1 - factor) + v2.position.z * factor,
//        v1.texcoord.x * (1 - factor) + v2.texcoord.x * factor,
//        v1.texcoord.y * (1 - factor) + v2.texcoord.y * factor);
//
//}
