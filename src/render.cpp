#include "render.hpp"
#include "mathlib.hpp"
#include "WICTextureLoader.hpp"
#include "mesh.hpp"
#include "inputsystem.hpp"
#include <d3dcompiler.h>
#include <vector>

Render g_Render;
Render* render = &g_Render;

struct ViewSetup
{
    ViewSetup(float2 size = float2(100.0f, 100.0f), float3 origin = float3(-10.0f, 0.0f, 10.0f),
        float3 target = float3(0.0f, 0.0f, 0.0f), float3 up = float3(0.0f, 0.0f, 1.0f),
        bool ortho = false, float fov = DirectX::XM_PIDIV4, float farZ = 1024.0f, float nearZ = 4.0f);

    void ComputeMatrices();

    float2 size;
    float3 origin;
    float3 target;
    float3 up;
    bool ortho;
    float fov;
    float farZ;
    float nearZ;

    DirectX::XMMATRIX matView;
    DirectX::XMMATRIX matProjection;
    
};


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
    //DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 10.0f, 10.0f, 0.0f);

    //m_ViewPosition = float3(std::cosf(t)*10.0f, std::sinf(t) * 10.0f, std::cosf(t) * 2.0f + 5.0f);
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
    Camera(ViewSetup* view, const D3D11_VIEWPORT* viewport);
    void Update();

private:
    ViewSetup *m_View;
    const D3D11_VIEWPORT* m_Viewport;
    float m_Pitch;
    float m_Yaw;
    float m_Speed;
    
};

Camera::Camera(ViewSetup* view, const D3D11_VIEWPORT* viewport)
    :   m_View(view),
        m_Viewport(viewport),
        m_Pitch(DirectX::XM_PIDIV2),
        m_Yaw(0.0f),
        m_Speed(0.01f)
{
    m_View->size = float2(viewport->Width, viewport->Height);
}

void Camera::Update()
{
    int frontback = input->IsKeyDown('W') - input->IsKeyDown('S');
    int strafe = input->IsKeyDown('A') - input->IsKeyDown('D');
    m_View->origin += float3(std::cosf(m_Yaw) * std::sinf(m_Pitch) * frontback + std::cosf(m_Yaw - DirectX::XM_PIDIV2) * strafe,
        std::sinf(m_Yaw) * std::sinf(m_Pitch) * frontback + std::sinf(m_Yaw - DirectX::XM_PIDIV2) * strafe, std::cosf(m_Pitch) * frontback) * m_Speed;
    
    POINT point = { m_View->size.x / 2, m_View->size.y / 2 };
    ClientToScreen(render->GetHWND(), &point);

    unsigned short x, y;
    input->GetMousePos(&x, &y);
    float sensivity = 0.00075;

    m_Yaw += (x - point.x) * sensivity;
    m_Pitch += (y - point.y) * sensivity;
    float epsilon = 0.0001f;
    m_Pitch = clamp(m_Pitch, 0.0f + epsilon, DirectX::XM_PI - epsilon);
    m_View->target = m_View->origin + float3(std::cosf(m_Yaw) * std::sinf(m_Pitch), std::sinf(m_Yaw) * std::sinf(m_Pitch), std::cosf(m_Pitch));

    input->SetMousePos(point.x, point.y);

}

Render::Render() : m_hWnd(0),
                   m_D3DDevice(0),
                   m_DeviceContext(0),
                   m_SwapChain(0),
                   m_RenderTargetView(0)
{
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
    ID3D11Texture2D *pBackBuffer;
    m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);

    // use the back buffer address to create the render target
    GetDevice()->CreateRenderTargetView(pBackBuffer, nullptr, &m_RenderTargetView);
    pBackBuffer->Release();

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

    ID3D11Texture2D *pDepthStencil;
    GetDevice()->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    GetDevice()->CreateDepthStencilView(pDepthStencil, &descDSV, &m_DepthStencilView);
    //pDepthStencil->Release();

    // set the render target as the back buffer
    GetDeviceContext()->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
    GetDeviceContext()->RSSetViewports(1, &m_Viewport);

}

struct ConstantBuffer
{
    DirectX::XMMATRIX matWorld;
    DirectX::XMMATRIX matViewProjection;
    
};

struct PSConstantBuffer
{
    float3 lightPositions[3];
    float3 lightColors[3];
    float3 viewPosition;
    float3 phongScale;
};

void Render::InitScene()
{
    // Load Shaders
    ID3DBlob* errorBlob;
    ID3DBlob* vertexShaderBuffer;
    ID3DBlob* pixelShaderBuffer;

    LPCWSTR shadername = L"src/phong.fx";

    if (FAILED(D3DCompileFromFile(shadername, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vertexShaderBuffer, &errorBlob)))
    {
        MessageBox(m_hWnd, (char*)errorBlob->GetBufferPointer(), "Error", MB_OK);
    }

    if (FAILED(D3DCompileFromFile(shadername, nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &pixelShaderBuffer, &errorBlob)))
    {
        MessageBox(m_hWnd, (char*)errorBlob->GetBufferPointer(), "Error", MB_OK);
    }
    
    GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &m_VertexShader);
    GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, &m_PixelShader);

    GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
    GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

    // Create meshes
    m_Meshes.push_back(Mesh("meshes/plane.obj"));
    m_Meshes.push_back(Mesh("meshes/cube.obj"));
    m_Meshes.push_back(Mesh("meshes/cone.obj"));
    m_Meshes.push_back(Mesh("meshes/sphere.obj"));
    
    m_Views.push_back(ViewSetup());
    m_Camera = std::make_unique<Camera>(&(m_Views.back()), &m_Viewport);

    // Set Input Layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_InputLayout);
    GetDeviceContext()->IASetInputLayout(m_InputLayout);
    
    // Create Constant Buffer
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));

    constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    constantBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_ConstantBuffer);

    // Create PS Constant Buffer
    D3D11_BUFFER_DESC PSconstantBufferDesc;
    ZeroMemory(&PSconstantBufferDesc, sizeof(PSconstantBufferDesc));

    PSconstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    PSconstantBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    PSconstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    GetDevice()->CreateBuffer(&PSconstantBufferDesc, nullptr, &m_PSConstantBuffer);
    
    GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_RASTERIZER_DESC wireframeDesc;
    ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_NONE;
    GetDevice()->CreateRasterizerState(&wireframeDesc, &m_WireframeRasterizer);
    
    ID3D11ShaderResourceView *textureView;
    CreateWICTextureFromFile(GetDevice(), GetDeviceContext(), "textures/brick.bmp", nullptr, &textureView);
    
    GetDeviceContext()->PSSetShaderResources(0, 1, &textureView);

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    GetDevice()->CreateSamplerState(&sampDesc, &m_SamplerLinear);
    GetDeviceContext()->PSSetSamplers(0, 1, &m_SamplerLinear);

    D3D11_BLEND_DESC omDesc;
    ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
    omDesc.RenderTarget[0].BlendEnable = true;
    omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    GetDevice()->CreateBlendState(&omDesc, &m_OpacityBlend);
    
}

void Render::Init(HWND hWnd)
{
    m_hWnd = hWnd;

    InitD3D();
    InitScene();

}

void Render::SetupView()
{
}

void Render::RenderFrame()
{
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    GetDeviceContext()->ClearRenderTargetView(m_RenderTargetView, clearColor);
    GetDeviceContext()->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    m_Camera->Update();

    ViewSetup& view = m_Views.back();
    view.ComputeMatrices();

    ConstantBuffer vscb;
    vscb.matViewProjection = DirectX::XMMatrixTranspose(view.matView * view.matProjection);

    PSConstantBuffer pscb;
    float t = GetTickCount() / 1000.0f;
    pscb.lightPositions[0] = float3(cos(1.321f * t), sin(0.923f * t), 1.0f) * 5.0f;
    pscb.lightColors[0] = float3(1.0f, 0, 0);

    pscb.lightPositions[1] = float3(cos(2.321f * t)* 2.0f, sin(2.125f * t) * 2.0f, 1.0f) * 5.0f;
    pscb.lightColors[1] = float3(0.0f, 1.0f, 0);

    pscb.lightPositions[2] = float3(cos(0.752f * t) * 3.0f, sin(3.687f * t) * 3.0f, 1.0f) * 5.0f;
    pscb.lightColors[2] = float3(0.0f, 0.0f, 1.0f);
    pscb.phongScale = 1.0f;

    pscb.viewPosition = view.origin;

    for (unsigned int i = 0; i < m_Meshes.size(); ++i)
    {
        GetDeviceContext()->OMSetBlendState(nullptr, 0, 0xffffffff);

        vscb.matWorld = DirectX::XMMatrixTranspose(m_Meshes[i].GetModelToWorld());
        GetDeviceContext()->UpdateSubresource(m_ConstantBuffer, 0, nullptr, &vscb, 0, 0);
        GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_ConstantBuffer);

        GetDeviceContext()->UpdateSubresource(m_PSConstantBuffer, 0, nullptr, &pscb, 0, 0);
        GetDeviceContext()->PSSetConstantBuffers(1, 1, &m_PSConstantBuffer);

        m_Meshes[i].Draw();
    }
        
    m_SwapChain->Present(0, 0);

}

void Render::Shutdown()
{
    // Delete objects in reverse order
    if (m_InputLayout) m_InputLayout->Release();
    if (m_ConstantBuffer) m_ConstantBuffer->Release();
    if (m_PixelShader) m_PixelShader->Release();
    if (m_VertexShader) m_VertexShader->Release();
    if (m_RenderTargetView) m_RenderTargetView->Release();
    if (m_SwapChain) m_SwapChain->Release();
    if (m_DeviceContext) GetDeviceContext()->Release();
    if (m_D3DDevice) GetDevice()->Release();

}
