#ifndef RENDER_HPP
#define RENDER_HPP

#include "gui.hpp"
#include "mathlib.hpp"
#include "utils.hpp"
#include <memory>
#include <Windows.h>

#include <d3d11.h>
// https://stackoverflow.com/questions/30267730/is-this-a-data-alignment-crash-potentially-involving-stack-misalignment-xnama
// https://msdn.microsoft.com/en-us/library/windows/desktop/ee418725.aspx#type_usage_guidelines_
// On Windows x64, all heap allocations are 16-byte aligned, but for Windows x86, they are only 8-byte aligned.
//#define _XM_NO_INTRINSICS_
//#include <DirectXMath.h>

struct Vertex;
struct ViewSetup;
class Mesh;
class Camera;
class Texture;
class ParticleEffect;
class ParticleEmitter;

struct ViewSetup
{
    ViewSetup() :
        x(0), y(0),
        width(128), height(128),
        viewSize(128.0f, 128.0f),
        origin(0.0f, 0.0f, 0.0f),
        target(1.0f, 0.0f, 0.0f),
        up(0.0f, 0.0f, 1.0f),
        ortho(false),
        fov(MATH_PIDIV2),
        farZ(1024.0f),
        nearZ(1.0f)
    {}

    void ComputeMatrices();

    int x, y;
    int width, height;
    float2 viewSize;
    float3 origin;
    float3 target;
    float3 up;
    bool ortho;
    float fov;
    float farZ;
    float nearZ;

    Matrix matWorldToCamera;
    Matrix matWorldToView;
    Matrix matViewToProjection;

};

struct ShadowState_t
{
    std::shared_ptr<Texture> depthTexture;
    ViewSetup view;
};

class Render
{
public:
    void Init(HWND hWnd);
    void Shutdown();
    void RenderFrame();
    const HWND GetHWND() const { return m_hWnd; }
    ID3D11Device* GetDevice() const { return m_D3DDevice.Get(); }
    ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext.Get(); }    
    const ViewSetup* GetCurrentView() const { return &m_ViewStack.back(); }
    const ViewSetup* GetPreviousView() const { return &m_ViewStack[m_ViewStack.size() - 2]; }
    double GetCurtime() { return GetTickCount() * 0.001; }
    double GetDeltaTime() { return GetCurtime() - m_PreviousFrameTime; }

    void PushView(ViewSetup& view, std::shared_ptr<Texture> renderTexture = nullptr);
    void PopView();

private:
    void InitD3D();
    void InitScene();
    void SetupView();

    HWND                                    m_hWnd;
    D3D11_VIEWPORT                          m_Viewport;
    ScopedObject<ID3D11Device>              m_D3DDevice;
    ScopedObject<ID3D11DeviceContext>       m_DeviceContext;
    ScopedObject<IDXGISwapChain>            m_SwapChain;
    ScopedObject<ID3D11RenderTargetView>    m_RenderTargetView;
    ScopedObject<ID3D11DepthStencilView>    m_DepthStencilView;
    std::vector<ShadowState_t>              m_ShadowStates;
    double m_PreviousFrameTime;

    std::vector<std::shared_ptr<Mesh> > m_Meshes;
    std::vector<std::shared_ptr<ParticleEffect> > m_ParticleEffects;
    std::shared_ptr<ParticleEmitter> emitter;
    std::vector<ViewSetup> m_ViewStack;
    std::unique_ptr<Camera> m_Camera;

        
};

extern Render* render;

#endif // RENDER_HPP
