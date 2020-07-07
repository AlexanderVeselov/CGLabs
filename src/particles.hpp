#ifndef PARTICLES_HPP
#define PARTICLES_HPP

#include "render.hpp"
#include "materialsystem.hpp"
#include "mesh.hpp"

struct Particle
{
    Particle() : lifeTime(0.0f) {}
    float3 origin;
    float3 velocity;
    float3 acceleration;
    float3 color;
    float  angle;
    float  size;
    float  emitTime;
    float  lifeTime;
    bool   active;

};

class ParticleEmitter
{
public:
    virtual void EmitParticle(Particle& particle) = 0;
    virtual const float3& GetOrigin() const = 0;
    virtual const float3& GetAngle() const = 0;
    virtual const float3& GetSize() const = 0;
    virtual void SetOrigin(const float3& origin) = 0;
    virtual void SetAngle(const float3& angle) = 0;
    virtual std::shared_ptr<Mesh> GetDebugMesh() = 0;

};

class RectangleEmitter : public ParticleEmitter
{
public:
    RectangleEmitter(float3 origin, float3 normal, float2 size);
    virtual const float3& GetOrigin() const { return m_Origin; }
    virtual const float3& GetAngle() const { return m_Normal; }
    virtual const float3& GetSize() const { return float3(m_Size.x, m_Size.y, 1.0f); }
    virtual void EmitParticle(Particle& particle);
    virtual void SetOrigin(const float3& origin) { m_Origin = origin; }
    virtual void SetAngle(const float3& angle) { m_Normal = angle; }
    virtual std::shared_ptr<Mesh> GetDebugMesh() { return m_DebugMesh; }

private:
    float3 m_Origin;
    float3 m_Normal;
    float2 m_Size;
    std::shared_ptr<Mesh> m_DebugMesh;

};

class ParticleEffect
{
public:
    ParticleEffect(std::shared_ptr<ParticleEmitter> emitter, unsigned int maxParticles, const char* materialName);
    void Draw() const;
    void Update();

private:
    void InitBuffers();

    std::shared_ptr<ParticleEmitter> m_Emitter;
    unsigned int m_MaxParticles;
    const char* m_MaterialName;
    std::vector<Particle> m_Particles;

    ScopedObject<ID3D11Buffer> m_VertexBuffer;
    ScopedObject<ID3D11Buffer> m_IndexBuffer;

};


#endif // PARTICLES_HPP
