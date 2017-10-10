#include "render.hpp"
#include "mathlib.hpp"
#include "mesh.hpp"
#include "inputsystem.hpp"
#include "materialsystem.hpp"
#include <d3dcompiler.h>
#include <vector>

Render g_Render;
Render* render = &g_Render;

ViewSetup::ViewSetup(float2 size,
                     float3 origin,
                     float3 target,
                     float3 up,
                     bool ortho,
                     float fov,
                     float farZ,
                     float nearZ)
    : size(size),
      origin(origin),
      target(target),
      up(up),
      ortho(ortho),
      fov(fov),
      farZ(farZ), nearZ(nearZ)
{
}

void ViewSetup::ComputeMatrices()
{
    // View Matrix
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(origin.x, origin.y, origin.z, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(target.x, target.y, target.z, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);
    matView = DirectX::XMMatrixLookAtLH(Eye, At, Up);

    // Projection Matrix
    if (ortho)
    {
        matProjection = DirectX::XMMatrixOrthographicLH(size.x, size.y, nearZ, farZ);
    }
    else
    {
        matProjection = DirectX::XMMatrixPerspectiveFovLH(fov, size.x / size.y, nearZ, farZ);
    }
}

class Camera
{
public:
    Camera(const D3D11_VIEWPORT* viewport);
    void Update();
    ViewSetup& GetView() { return m_View; }

private:
    ViewSetup m_View;
    const D3D11_VIEWPORT* m_Viewport;
    float m_Pitch;
    float m_Yaw;
    float m_Speed;
    
};

Camera::Camera(const D3D11_VIEWPORT* viewport)
    :   m_Viewport(viewport),
        m_Pitch(DirectX::XM_PIDIV2),
        m_Yaw(0.0f),
        m_Speed(0.01f)
{
    m_View.size = float2(viewport->Width, viewport->Height);
}

void Camera::Update()
{
    static POINT point = { 0, 0 };
    unsigned short x, y;
    input->GetMousePos(&x, &y);
    POINT mouseClient = { x, y };
    ScreenToClient(render->GetHWND(), &mouseClient);

    if (input->IsMousePressed(MK_RBUTTON) && mouseClient.x > 0 && mouseClient.y > 0 && mouseClient.x < m_Viewport->Width && mouseClient.y < m_Viewport->Height)
    {
        float sensivity = 0.00075f;
        m_Yaw += (x - point.x) * sensivity;
        m_Pitch += (y - point.y) * sensivity;
        float epsilon = 0.0001f;
        m_Pitch = clamp(m_Pitch, 0.0f + epsilon, DirectX::XM_PI - epsilon);
        input->SetMousePos(point.x, point.y);
    }
    else
    {
        point = { x, y };
    }
    
    int frontback = input->IsKeyDown('W') - input->IsKeyDown('S');
    int strafe = input->IsKeyDown('A') - input->IsKeyDown('D');
    m_View.origin += float3(std::cosf(m_Yaw) * std::sinf(m_Pitch) * frontback + std::cosf(m_Yaw - DirectX::XM_PIDIV2) * strafe,
        std::sinf(m_Yaw) * std::sinf(m_Pitch) * frontback + std::sinf(m_Yaw - DirectX::XM_PIDIV2) * strafe, std::cosf(m_Pitch) * frontback) * m_Speed;
    m_View.target = m_View.origin + float3(std::cosf(m_Yaw) * std::sinf(m_Pitch), std::sinf(m_Yaw) * std::sinf(m_Pitch), std::cosf(m_Pitch));


}

void Render::InitD3D()
{
    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
    scd.BufferCount = 1;                                    // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.OutputWindow = m_hWnd;                              // the window to be used
    scd.SampleDesc.Count = 1;                               // how many multisamples
    scd.SampleDesc.Quality = 0;                             // how many multisamples
    scd.Windowed = TRUE;                                    // windowed/full-screen mode

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(
        nullptr,                    // pAdapter
        D3D_DRIVER_TYPE_HARDWARE,   // DriverType
        nullptr,                    // Software
        0,                          // Flags
        &featureLevel,              // pFeatureLevels
        1,                          // FeatureLevels
        D3D11_SDK_VERSION,          // SDKVersion
        &scd,                       // pSwapChainDesc
        &m_SwapChain,               // ppSwapChain
        &m_D3DDevice,               // ppDevice
        nullptr,                    // pFeatureLevel
        &m_DeviceContext);          // ppImmediateContext

    // Set the viewport
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    ZeroMemory(&m_Viewport, sizeof(D3D11_VIEWPORT));
    m_Viewport.TopLeftX = rc.top;
    m_Viewport.TopLeftY = rc.left;
    m_Viewport.Width = rc.right - rc.left;
    m_Viewport.Height = rc.bottom - rc.top;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
    
    // Setup Back Buffer
    ScopedObject<ID3D11Texture2D> pBackBuffer;
    m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);

    // use the back buffer address to create the render target
    GetDevice()->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_RenderTargetView);

    // Setup Depth Buffer
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = m_Viewport.Width;
    descDepth.Height = m_Viewport.Height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = scd.SampleDesc.Count;
    descDepth.SampleDesc.Quality = scd.SampleDesc.Quality;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ScopedObject<ID3D11Texture2D> depthStencilTexture;
    GetDevice()->CreateTexture2D(&descDepth, nullptr, &depthStencilTexture);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    GetDevice()->CreateDepthStencilView(depthStencilTexture.Get(), &descDSV, &m_DepthStencilView);
    
    GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

void Render::InitScene()
{    
    // Create meshes
    //m_Meshes.push_back(Mesh("meshes/plane.obj"));
    //m_Meshes.push_back(Mesh("meshes/cube.obj"));
    //m_Meshes.push_back(Mesh("meshes/cone.obj"));
    //m_Meshes.push_back(Mesh("meshes/sphere.obj"));

    m_Meshes.push_back(std::make_shared<Mesh>("meshes/city.obj"));
    
    m_Camera = std::make_unique<Camera>(&m_Viewport);
  
}

void Render::Init(HWND hWnd)
{
    m_hWnd = hWnd;
    
    InitD3D();

    materials->Init();
    ShadowState_t shadowstate;
    shadowstate.depthTexture = materials->CreateRenderableTexture(2048, 2048, "_rt_ShadowDepth");
    m_ShadowStates.push_back(shadowstate);
    
    InitScene();


}

void Render::SetupView()
{
}

void Render::PushView(ViewSetup& view, std::shared_ptr<Texture> renderTexture)
{
    view.ComputeMatrices();
    m_ViewStack.push_back(view);

    if (renderTexture == nullptr)
    {
        GetDeviceContext()->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView.Get());
        float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        GetDeviceContext()->ClearRenderTargetView(m_RenderTargetView.Get(), clearColor);
        GetDeviceContext()->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        GetDeviceContext()->RSSetViewports(1, &m_Viewport);
    }
    else
    {
        ID3D11RenderTargetView* renderTargetView = renderTexture->GetRenderTargetView();
        ID3D11DepthStencilView* depthStencilView = renderTexture->GetDepthStencilView();
        GetDeviceContext()->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
        float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GetDeviceContext()->ClearRenderTargetView(renderTargetView, clearColor);
        GetDeviceContext()->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        D3D11_VIEWPORT viewport;
        ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = 2048;
        viewport.Height = 2048;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        GetDeviceContext()->RSSetViewports(1, &viewport);

    }
}

void Render::PopView()
{
    m_ViewStack.pop_back();
}

void Render::RenderFrame()
{
    m_Camera->Update();

    ShadowState_t cascade = m_ShadowStates.back();
    ViewSetup& view = cascade.view;
    view.origin = float3(1000, 1000, 500);
    view.target = float3(0, 0, 0);
    view.ortho = true;
    view.size = float2(100, 100);

    PushView(view, m_ShadowStates.back().depthTexture);
    {
        for (std::vector<std::shared_ptr<Mesh> >::iterator it = m_Meshes.begin(); it != m_Meshes.end(); ++it)
        {
            (*it)->Draw(true);
        }

        PushView(m_Camera->GetView());
        {
            for (std::vector<std::shared_ptr<Mesh> >::iterator it = m_Meshes.begin(); it != m_Meshes.end(); ++it)
            {
                (*it)->Draw();
            }
        }
        PopView();
    }
    PopView();

    m_SwapChain->Present(0, 0);

}

void Render::Shutdown()
{
}
