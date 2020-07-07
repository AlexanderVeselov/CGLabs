#include "mathlib.hpp"
#include "particles.hpp"

float Random11()
{
    return (float)rand() / RAND_MAX * 2.0f - 1.0f;
}

float Random01()
{
    return (float)rand() / RAND_MAX;
}

RectangleEmitter::RectangleEmitter(float3 origin, float3 normal, float2 size)
    : m_Origin(origin), m_Normal(normal), m_Size(size)
{
    m_DebugMesh = std::make_shared<Mesh>("meshes/plane_1x1.obj");
}

// pos = pos0 + v0*t + a * t^2 / 2
void RectangleEmitter::EmitParticle(Particle& particle)
{
    particle.acceleration = float3(0.0f, 0.0f, -9.8f);
    particle.velocity = m_Normal * 20.0f;
    particle.active = true;
    particle.size = Random01() * 2.0f + 1.0f;

    float3 right = cross(m_Normal, float3(0.0f, 0.0f, 1.0f)).normalize();
    float3 up = cross(right, m_Normal);

    particle.origin = m_Origin + right * m_Size.x * 0.5f * Random11() + up * m_Size.y * 0.5f * Random11();
    particle.emitTime = render->GetCurtime();
    particle.lifeTime = Random01() * 20.0f;
    particle.color = float3(1, 0, 0);

}


ParticleEffect::ParticleEffect(std::shared_ptr<ParticleEmitter> emitter, unsigned int maxParticles, const char* materialName)
    : m_Emitter(emitter), m_MaxParticles(maxParticles), m_MaterialName(materialName)
{
    m_Particles.resize(maxParticles);
    InitBuffers();
}

void ParticleEffect::InitBuffers()
{
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * m_MaxParticles * 4;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    render->GetDevice()->CreateBuffer(&vertexBufferDesc, nullptr, &m_VertexBuffer);

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < m_MaxParticles; ++i)
    {
        indices.push_back(i * 4);
        indices.push_back(i * 4 + 1);
        indices.push_back(i * 4 + 2);

        indices.push_back(i * 4 + 1);
        indices.push_back(i * 4 + 3);
        indices.push_back(i * 4 + 2);
    }

    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned int) * m_MaxParticles * 6;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexBufferData;
    ZeroMemory(&indexBufferData, sizeof(indexBufferData));
    indexBufferData.pSysMem = indices.data();
    render->GetDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexBuffer);
    
}

void ParticleEffect::Update()
{
    std::vector<Vertex> vertices;
    for (unsigned int i = 0; i < m_MaxParticles; ++i)
    {
        Particle& particle = m_Particles[i];
        float curtime = render->GetCurtime();
        float deltaTime = render->GetDeltaTime();
        if (curtime > particle.emitTime + particle.lifeTime)
        {
            m_Emitter->EmitParticle(particle);
        }

        float3 originXY(particle.origin.x, particle.origin.y, 0.0f);
        float3 cylinderXY(0, 0, 0);
        float radius = 8.0f;
        float height = 64.0f;

        float3 diff = originXY - cylinderXY;
        if (diff.length() < radius && particle.origin.z < height)
        {
            if (particle.origin.z > height - 1.0f)
            {
                particle.velocity *= 0.75f;
                particle.origin.z = height + 1.0f;
                particle.velocity = reflect(particle.velocity, float3(0.0f, 0.0f, 1.0f));
            }
            else
            {
                float3 normal = diff.normalize();
                particle.velocity *= 0.75f;
                particle.velocity = reflect(particle.velocity, normal);

                particle.origin.x = normal.x * (radius + 1.0f);
                particle.origin.y = normal.y * (radius + 1.0f);
            }

        }        

        if (particle.origin.z < 0.0f)
        {
            particle.origin.z = 0.0f;
            particle.velocity *= 0.75f;
            particle.velocity = reflect(particle.velocity, float3(0.0f, 0.0f, 1.0f));
        }

        particle.velocity += particle.acceleration * deltaTime;
        particle.origin += particle.velocity * deltaTime;

        particle.color.x = clamp(1.0f - (curtime - particle.emitTime) / particle.lifeTime, 0.0f, 1.0f);
        const ViewSetup* view = render->GetCurrentView();
        
        float3 front = (view->target - view->origin).normalize();
        float3 right = cross(view->up, front).normalize();
        float3 up = cross(front, right).normalize();

        vertices.push_back(Vertex(particle.origin + (-right + up) * particle.size, float2(0, 1), particle.color));
        vertices.push_back(Vertex(particle.origin + (right + up) * particle.size, float2(1, 1), particle.color));
        vertices.push_back(Vertex(particle.origin + (-right - up) * particle.size, float2(0, 0), particle.color));
        vertices.push_back(Vertex(particle.origin + (right - up) * particle.size, float2(1, 0), particle.color));
        
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    render->GetDeviceContext()->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, vertices.data(), sizeof(Vertex) * vertices.size());
    render->GetDeviceContext()->Unmap(m_VertexBuffer.Get(), 0);

}

void ParticleEffect::Draw() const
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    const ViewSetup* view = render->GetCurrentView();
    Material::VSConstantBuffer vscb;
    vscb.matWorldToCamera = view->matWorldToCamera.Transpose();
    vscb.matModelToWorld = Matrix::Identity();
    vscb.viewPosition = view->origin;

    Material::PSConstantBuffer pscb;
    pscb.viewPosition = view->origin;
    pscb.lightColor = float3(1.0f, 0.9f, 0.8f) * 2.0f;

    render->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    render->GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    render->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const ViewSetup* shadowView = render->GetPreviousView();
    vscb.matShadowToWorld = shadowView->matWorldToCamera.Transpose();
    pscb.lightPos = shadowView->origin;
    materials->FindMaterial(m_MaterialName)->SetMaterial(vscb, pscb);
    render->GetDeviceContext()->DrawIndexed(m_MaxParticles * 6, 0, 0);

    //std::shared_ptr<Mesh> debugMesh = m_Emitter->GetDebugMesh();
    //debugMesh->SetTransform(Matrix::Translation(m_Emitter->GetOrigin()) * Matrix::Scaling(float3(32, 32, 32)) * Matrix::RotationAxis(float3(0, 1, 0), MATH_PIDIV2 + m_Emitter->G));
    //debugMesh->Draw();

}
