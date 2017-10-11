#include "render.hpp"
#include "mathlib.hpp"
#include "mesh.hpp"
#include "inputsystem.hpp"
#include "materialsystem.hpp"
#include <d3dcompiler.h>
#include <vector>

static Render g_Render;
Render* render = &g_Render;

void ViewSetup::ComputeMatrices()
{
    // View Matrix
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(origin.x, origin.y, origin.z, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(target.x, target.y, target.z, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);
    matWorldToCamera = DirectX::XMMatrixLookAtLH(Eye, At, Up);
    
    // Projection Matrix
    if (ortho)
    {
        matWorldToCamera *= DirectX::XMMatrixOrthographicLH(viewSize.x, viewSize.y, nearZ, farZ);
    }
    else
    {
        matWorldToCamera *= DirectX::XMMatrixPerspectiveFovLH(fov, viewSize.x / viewSize.y, nearZ, farZ);
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
    float m_Pitch;
    float m_Yaw;
    float m_Speed;
    
};

Camera::Camera(const D3D11_VIEWPORT* viewport)
    :   m_Pitch(DirectX::XM_PIDIV2),
        m_Yaw(0.0f),
        m_Speed(0.1f)
{
    RECT rc;
    GetClientRect(render->GetHWND(), &rc);

    m_View.x = rc.top;
    m_View.y = rc.left;
    m_View.width = rc.right - rc.left;
    m_View.height = rc.bottom - rc.top;
    m_View.viewSize = float2(m_View.width, m_View.height);

}

void Camera::Update()
{
    static POINT point = { 0, 0 };
    unsigned short x, y;
    input->GetMousePos(&x, &y);
    POINT mouseClient = { x, y };
    ScreenToClient(render->GetHWND(), &mouseClient);

    if (input->IsMousePressed(MK_RBUTTON) && mouseClient.x > 0 && mouseClient.y > 0 && mouseClient.x < m_View.width && mouseClient.y < m_View.height)
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
        
    // Setup Back Buffer
    ScopedObject<ID3D11Texture2D> pBackBuffer;
    m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);

    // use the back buffer address to create the render target
    GetDevice()->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_RenderTargetView);

    // Setup Depth Buffer
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = 1280;
    descDepth.Height = 720;
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
    

}

void Render::InitScene()
{    
    // Create meshes
    //m_Meshes.push_back(Mesh("meshes/plane.obj"));
    //m_Meshes.push_back(Mesh("meshes/cube.obj"));
    //m_Meshes.push_back(Mesh("meshes/cone.obj"));

    m_Meshes.push_back(std::make_shared<Mesh>("meshes/teapot.obj"));
    //m_Meshes.push_back(std::make_shared<Mesh>("meshes/room.obj"));
    
    m_Camera = std::make_unique<Camera>(&m_Viewport);
  
}

void Render::Init(HWND hWnd)
{
    m_hWnd = hWnd;
    
    guimanager->AddTrackbar(20, 100, 200, "light_pitch", 0.0f, DirectX::XM_PIDIV2, DirectX::XM_PIDIV4, DirectX::XM_PIDIV2 / 32);
    guimanager->AddTrackbar(20, 150, 200, "light_yaw", 0.0f, DirectX::XM_2PI, DirectX::XM_PIDIV4, DirectX::XM_2PI / 32);

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

    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.TopLeftX = view.x;
    viewport.TopLeftY = view.y;
    viewport.Width = view.width;
    viewport.Height = view.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    if (renderTexture == nullptr)
    {
        float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        GetDeviceContext()->ClearRenderTargetView(m_RenderTargetView.Get(), clearColor);
        GetDeviceContext()->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        GetDeviceContext()->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView.Get());
    }
    else
    {
        ID3D11RenderTargetView* renderTargetView = renderTexture->GetRenderTargetView();
        ID3D11DepthStencilView* depthStencilView = renderTexture->GetDepthStencilView();
        float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GetDeviceContext()->ClearRenderTargetView(renderTargetView, clearColor);
        GetDeviceContext()->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        GetDeviceContext()->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    }
    GetDeviceContext()->RSSetViewports(1, &viewport);

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
    float pitch = guimanager->GetElementByName<Trackbar>("light_pitch")->GetValue();
    pitch = clamp(pitch, 0.0f, DirectX::XM_PIDIV2 - 0.0001f);
    float yaw = guimanager->GetElementByName<Trackbar>("light_yaw")->GetValue();
    view.origin = float3(std::cosf(yaw)*std::cosf(pitch), std::sinf(yaw)*std::cosf(pitch), std::sinf(pitch)) * 500;
    view.target = float3(0, 0, 0);
    view.ortho = true;
    view.viewSize = float2(256, 256);
    view.width = 2048;
    view.height = 2048;

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
