#include "mesh.hpp"
#include "materialsystem.hpp"
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

void Mesh::LoadFromObj(const char* filename)
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
    strcpy(meshGroup.materialName, "debug_checker");
    
    while ((token = objReader.NextToken()) != ObjReader::OBJ_EOF)
    {
        switch (token)
        {
        case ObjReader::OBJ_USEMTL:
            if (meshGroup.indexCount > 0)
            {
                m_MeshGroups.push_back(meshGroup);
            }
            strcpy(meshGroup.materialName, objReader.GetStringValue().c_str());
            meshGroup.startIndex = m_Indices.size();
            meshGroup.indexCount = 0;
            break;
        case ObjReader::OBJ_POSITION:
            positions.push_back(float3(objReader.GetVectorValue().x, -objReader.GetVectorValue().y, objReader.GetVectorValue().z));
            //m_Vertices.push_back(Vertex(positions.back()));
            break;
        case ObjReader::OBJ_NORMAL:
            normals.push_back(float3(objReader.GetVectorValue().x, -objReader.GetVectorValue().y, objReader.GetVectorValue().z));
            break;
        case ObjReader::OBJ_TEXCOORD:
            texcoords.push_back(float2(objReader.GetVectorValue().x, 1.0f - objReader.GetVectorValue().y));
            break;
        case ObjReader::OBJ_FACE:
            unsigned int *iv, *it, *in;
            objReader.GetFaceIndices((const unsigned int**)&iv, (const unsigned int**)&it, (const unsigned int**)&in);
            std::swap(iv[0], iv[1]);
            std::swap(it[0], it[1]);
            std::swap(in[0], in[1]);

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
                ++meshGroup.indexCount;            }
            break;
        case ObjReader::OBJ_SMOOTHINGGROUP:
            break;
        }
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
}

// Maybe not good, but much faster than loading obj...
void Mesh::SaveToDat(const char* filename) const
{
    std::ofstream outfile(filename, std::ios::out | std::ofstream::binary);
    unsigned int size = m_Vertices.size() * sizeof(Vertex);
    outfile.write((char*)&size, sizeof(size));
    outfile.write((char*)m_Vertices.data(), size);

    size = m_Indices.size() * sizeof(unsigned int);
    outfile.write((char*)&size, sizeof(size));
    outfile.write((char*)m_Indices.data(), size);

    size = m_MeshGroups.size() * sizeof(MeshGroup_t);
    outfile.write((char*)&size, sizeof(size));
    outfile.write((char*)m_MeshGroups.data(), size);

}

void Mesh::LoadFromDat(const char* filename)
{
    std::ifstream infile(filename, std::ios::in | std::ifstream::binary);

    unsigned int size;
    infile.read((char*)&size, sizeof(size));
    m_Vertices.resize(size / sizeof(Vertex));
    infile.read((char*)m_Vertices.data(), size);

    infile.read((char*)&size, sizeof(size));
    m_Indices.resize(size / sizeof(unsigned int));
    infile.read((char*)m_Indices.data(), size);

    infile.read((char*)&size, sizeof(size));
    m_MeshGroups.resize(size / sizeof(MeshGroup_t));
    infile.read((char*)m_MeshGroups.data(), size);


}

// Load from .obj
Mesh::Mesh(const char* filename, const char* mtldir, const Matrix& modelToWorld, bool castShadow) : m_ModelToWorld(modelToWorld), m_CastShadow(castShadow)
{
    const char* ext;
    ext = strrchr(filename, '.');

    if (strcmp(ext, ".obj") == 0)
    {
        LoadFromObj(filename);
        char outfile[80];
        ZeroMemory(outfile, 80);
        strncpy(outfile, filename, ext - filename);
        strcat(outfile, ".dat");
        SaveToDat(outfile);
    }
    else if (strcmp(ext, ".dat") == 0)
    {
        LoadFromDat(filename);
    }

    if (mtldir)
    {
        for (unsigned int i = 0; i < m_MeshGroups.size(); ++i)
        {
            char name[56];
            ZeroMemory(name, 56);
            sprintf(name, "%s/%s", mtldir, m_MeshGroups[i].materialName);

            memcpy(m_MeshGroups[i].materialName, name, 56);
        }
    }

    InitBuffers();

}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : m_Vertices(vertices), m_Indices(indices), m_ModelToWorld(Matrix::Identity())
{
    InitBuffers();
}

void Mesh::Draw(bool drawDepth)
{
    if (drawDepth && !m_CastShadow)
    {
        return;
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    const ViewSetup* view = render->GetCurrentView();
    Material::VSConstantBuffer vscb;
    vscb.matWorldToCamera = view->matWorldToCamera.Transpose();
    vscb.matModelToWorld = m_ModelToWorld;
    vscb.viewPosition = view->origin;
    
    Material::PSConstantBuffer pscb;
    pscb.viewPosition = view->origin;
    pscb.lightColor = float3(1.0f, 0.9f, 0.8f) * 2.0f;

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
        vscb.matShadowToWorld = shadowView->matWorldToCamera.Transpose();
        pscb.lightPos = shadowView->origin;
        for (unsigned int i = 0; i < m_MeshGroups.size(); ++i)
        {
            const MeshGroup_t& meshGroup = m_MeshGroups[i];
            materials->FindMaterial(meshGroup.materialName)->SetMaterial(vscb, pscb);
            render->GetDeviceContext()->DrawIndexed(meshGroup.indexCount, meshGroup.startIndex, 0);
        }

    }


}

#define MAX_COORD_INTEGER 16384

std::vector<Vertex> BaseWindingForPlane(float3 normal, float dist)
{
    int		i, x;
    float	max, v;
    float3	org, vright, vup;
    std::vector<Vertex>	w;

    // find the major axis
    max = -1;
    x = -1;
    for (i = 0; i<3; i++)
    {
        v = fabs(normal[i]);
        if (v > max)
        {
            x = i;
            max = v;
        }
    }
    if (x == -1)
    {
        throw std::exception("BaseWindingForPlane: no axis found");
    }

    switch (x)
    {
    case 0:
    case 1:
        vup.z = 1;
        break;
    case 2:
        vup.x = 1;
        break;
    }

    v = dot(vup, normal);
    vup -= normal * v;
    vup = vup.normalize();
    org = normal * dist;

    vright = cross(vup, normal);
    vup *= 1024.0f;
    vright *= 1024.0f;
            
    w.push_back(Vertex(org - vright + vup, float2(0.0f, 0.0f) * 64.0f, normal));
    w.push_back(Vertex(org + vright + vup, float2(0.0f, 1.0f) * 64.0f, normal));
    w.push_back(Vertex(org + vright - vup, float2(1.0f, 1.0f) * 64.0f, normal));
    w.push_back(Vertex(org - vright - vup, float2(1.0f, 0.0f) * 64.0f, normal));
    
    return w;
}

#define MAX_POINTS_ON_WINDING 64
#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2
#define SIDE_CROSS  -2

std::vector<Vertex> ChopWindingInPlace(const std::vector<Vertex>& in, const float3 &normal, float dist, float epsilon)
{
    float	dists[MAX_POINTS_ON_WINDING + 4];
    int		sides[MAX_POINTS_ON_WINDING + 4];
    
    int		counts[3];
    counts[0] = counts[1] = counts[2] = 0;
    // determine sides for each point
    unsigned int i;
    for (i = 0; i < in.size(); i++)
    {
        float dotp = dot(in[i].position, normal);
        dotp -= dist;
        dists[i] = dotp;
        if (dotp > epsilon)
        {
            sides[i] = SIDE_FRONT;
        }
        else if (dotp < -epsilon)
        {
            sides[i] = SIDE_BACK;
        }
        else
        {
            sides[i] = SIDE_ON;
        }
        counts[sides[i]]++;
    }
    sides[i] = sides[0];
    dists[i] = dists[0];
    
    if (!counts[0] || !counts[1])
    {
        return in;
    }

    std::vector<Vertex> f;
    for (i = 0; i < in.size(); i++)
    {

        if (sides[i] == SIDE_ON)
        {
            f.push_back(in[i]);
            continue;
        }

        if (sides[i] == SIDE_BACK)
        {
            f.push_back(in[i]);
        }

        if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
            continue;

        // generate a split point
        const Vertex& v1 = in[i];
        const Vertex& v2 = in[(i + 1) % in.size()];

        float3	mid;
        float dotp = dists[i] / (dists[i] - dists[i + 1]);

        f.push_back(Vertex(v1.position + (v2.position - v1.position) * dotp, v1.texcoord + (v2.texcoord - v1.texcoord) * dotp, v1.normal));

    }

    return f;
}

AnimatedPolyhedron::AnimatedPolyhedron(float3 origin) : m_Velocity(1.0f, 0.0f, 0.0f), m_PlaneAngle(0.0), m_CurrentAngle(1.0), m_CurrentSide(0), m_CurrentEdge(0)
{
    std::vector<Plane_t> planes;
    m_ModelToWorld = Matrix::Translation(0, 0, 8);
    m_Velocity.normalize();
    
    planes.push_back({ float3( 0,  0, -1), 8 });
    planes.push_back({ float3( 0,  0,  1), 8 });
    planes.push_back({ float3( 1,  0,  0), 8 });
    planes.push_back({ float3(-1,  0,  0), 8 });
    planes.push_back({ float3( 0,  1,  0), 8 });
    planes.push_back({ float3( 0, -1,  0), 8 });

    for (unsigned int i = 0; i < 8; ++i)
    {
        float yaw = (float)rand() / RAND_MAX * MATH_2PI;
        float pitch = - (float)rand() / RAND_MAX * MATH_2PI * 0.9f;
        float3 n(std::cosf(yaw)*std::cosf(pitch), std::sinf(yaw)*std::cosf(pitch), std::sinf(pitch));
        planes.push_back({ n, 8 });
    }

    unsigned int index = 0;
    for (unsigned int i = 0; i < planes.size(); ++i)
    {
        std::vector<Vertex>	w = BaseWindingForPlane(planes[i].normal, planes[i].dist);

        for (unsigned int j = 0; j < planes.size(); ++j)
        {
            if (i == j) continue;
            w = ChopWindingInPlace(w, planes[j].normal, planes[j].dist, 0);
        }
        
        for (unsigned int j = 2; j < w.size(); j++)
        {
            Vertex& v1 = w[0];
            Vertex& v2 = w[j];
            Vertex& v3 = w[j - 1];
            ComputeTangentSpace(v1, v2, v3);

            m_Vertices.push_back(v1);
            m_Indices.push_back(index++);
            m_Vertices.push_back(v2);
            m_Indices.push_back(index++);
            m_Vertices.push_back(v3);
            m_Indices.push_back(index++);
        }
        Side_t side;
        side.center = float3(0.0f);
        for (unsigned int j = 0; j < w.size(); ++j)
        {
            side.positions.push_back(w[j].position);
            side.center += w[j].position;
        }
        side.center *= 1.0f / w.size();
        side.plane = planes[i];
        m_Sides.push_back(side);
    }

    for (unsigned int i = 0; i < m_Vertices.size(); ++i)
    {
        float3& n = m_Vertices[i].normal;
        const float3& t = m_Vertices[i].tangent_t;

        m_Vertices[i].tangent_t = (t - n * dot(n, t)).normalize();
        m_Vertices[i].tangent_s = cross(m_Vertices[i].tangent_t, m_Vertices[i].normal);

    }

    for (unsigned int i = 0; i < m_Sides.size(); ++i)
    {
        Side_t& side1 = m_Sides[i];
        for (unsigned int k = 0; k < side1.positions.size(); ++k)
        {
            for (unsigned int j = 0; j < m_Sides.size(); ++j)
            {
                if (i == j) continue;
                Side_t& side2 = m_Sides[j];
                for (unsigned int s = 0; s < side2.positions.size(); ++s)
                {
                    const float eps = 0.001f;
                    float dist1 = distance(side1.positions[k % side1.positions.size()], side2.positions[s % side2.positions.size()]);
                    float dist2 = distance(side1.positions[(k + 1) % side1.positions.size()], side2.positions[(s + 1) % side2.positions.size()]);
                    
                    float dist3 = distance(side1.positions[k % side1.positions.size()], side2.positions[(s + 1) % side2.positions.size()]);
                    float dist4 = distance(side1.positions[(k + 1) % side1.positions.size()], side2.positions[s % side2.positions.size()]);

                    if (dist1 < eps && dist2 < eps || dist3 < eps && dist4 < eps)
                    {
                        side1.neighbors.push_back({ j, s });
                    }

                }
            }
        }
    }
    
    MeshGroup_t meshGroup;
    meshGroup.startIndex = 0;
    meshGroup.indexCount = m_Indices.size();
    strcpy(meshGroup.materialName, "debug_checker");
    m_MeshGroups.push_back(meshGroup);
    InitBuffers();

}

void AnimatedPolyhedron::Draw(bool drawDepth)
{    
    if (m_CurrentAngle >= m_PlaneAngle)
    {
        if (m_PlaneAngle > 0.0f)
        {
            m_ModelToWorld = Matrix::RotationAxisAroundPoint(v1 - v2, v1, -m_CurrentAngle + m_PlaneAngle) * m_ModelToWorld;
        }
        float dot1 = -2.0f;
        unsigned int point1 = -1;
        float3 velocity = m_Velocity.normalize();
        for (unsigned int i = 0; i < m_Sides[m_CurrentSide].positions.size(); ++i)
        {
            float3 point = m_Sides[m_CurrentSide].positions[i];
            float3 pointWorld = m_ModelToWorld * point;
            float3 center = m_Sides[m_CurrentSide].center;
            float3 centerWorld = m_ModelToWorld * center;
            float3 vec = (pointWorld - centerWorld).normalize();
            float dotp = dot(vec, velocity);
            if (dotp > dot1)
            {
                dot1 = dotp;
                point1 = i;
            }
        }

        dot1 = -2.0f;
        unsigned int point2 = -1;
        for (unsigned int i = 0; i < m_Sides[m_CurrentSide].positions.size(); ++i)
        {
            if (i == point1) continue;
            float3 point = m_Sides[m_CurrentSide].positions[i];
            float3 pointWorld = m_ModelToWorld * point;
            float3 center = m_Sides[m_CurrentSide].center;
            float3 centerWorld = m_ModelToWorld * center;
            float3 vec = (pointWorld - centerWorld).normalize();
            float dotp = dot(vec, velocity);
            if (dotp > dot1)
            {
                dot1 = dotp;
                point2 = i;
            }
        }
        const Neighbor_t& neighbor = m_Sides[m_CurrentSide].neighbors[point2 > point1 ? point1 : point2];
        m_PlaneAngle = std::acosf(dot(m_Sides[neighbor.sideIndex].plane.normal, m_Sides[m_CurrentSide].plane.normal));
        m_CurrentSide = neighbor.sideIndex;
        m_CurrentEdge = neighbor.edgeIndex;
        m_CurrentAngle = 0.0f;

        v1 = m_ModelToWorld * m_Sides[m_CurrentSide].positions[m_CurrentEdge];
        v2 = m_ModelToWorld * m_Sides[m_CurrentSide].positions[(m_CurrentEdge + 1) % m_Sides[m_CurrentSide].positions.size()];
        float speed = m_Velocity.length();
        float3 cr = cross(v1 - v2, float3(0.0f, 0.0f, 1.0f));

        m_Velocity = cr.normalize() * speed;

    }

    float deltaAngle = MATH_PI / 4.0 * render->GetDeltaTime();
    m_CurrentAngle += deltaAngle;
    m_ModelToWorld = Matrix::RotationAxisAroundPoint(v1 - v2, v1, deltaAngle) * m_ModelToWorld;
        
    Mesh::Draw(drawDepth);

}
