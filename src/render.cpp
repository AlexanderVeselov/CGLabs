#include "render.hpp"
#include "mathlib.hpp"
#include "WICTextureLoader.hpp"
#include "mesh.hpp"
#include <d3dcompiler.h>
#include <vector>

Render g_Render;
Render* render = &g_Render;

#define TASK_2

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

#ifdef TASK_1
    LPCWSTR shadername = L"src/unlit.fx";
#else
    LPCWSTR shadername = L"src/phong.fx";
#endif

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
#ifdef TASK_1
    //m_Meshes.push_back(Mesh("meshes/teapot.obj"));
    //m_Meshes.push_back(Mesh::CreateTetrahedron());
    m_Meshes.push_back(Mesh("meshes/plane.obj"));
    m_Meshes.push_back(Mesh("meshes/cylinder.obj"));
    m_Meshes.push_back(Mesh("meshes/sphere.obj"));
    m_Meshes.push_back(Mesh("meshes/tetrahedron.obj"));
    m_Meshes.push_back(Mesh("meshes/torus.obj"));
#else
    m_Meshes.push_back(Mesh("meshes/plane.obj"));
    m_Meshes.push_back(Mesh("meshes/cube.obj"));
    m_Meshes.push_back(Mesh("meshes/cone.obj"));
    m_Meshes.push_back(Mesh("meshes/sphere.obj"));

#endif

    m_MorphedMesh = new MorphedMesh();

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
#ifdef TASK_1
    CreateWICTextureFromFile(GetDevice(), GetDeviceContext(), "textures/checker.jpg", nullptr, &textureView);
#else
    CreateWICTextureFromFile(GetDevice(), GetDeviceContext(), "textures/brick.bmp", nullptr, &textureView);
#endif

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
    
#ifdef TASK_1
    guimanager->AddCheckbox(50, 32, 160, "wireframe");
    guimanager->AddTrackbar(50, 96, 160, "cylinder_x", -10.0f, 10.0f, 5.0f, 0.5f);
    guimanager->AddTrackbar(50, 160, 160, "torus_rot", 0.0f, DirectX::XM_PIDIV2, 0.0f, DirectX::XM_PIDIV2 / 10.0f);
    guimanager->AddTrackbar(50, 224, 160, "morph_factor", 0.0f, 1.0f, 0.0f, 0.05f);

    guimanager->AddTrackbar(50, 288, 160, "tetrahedron_x", -10.0f, 10.0f, -7.0f, 0.5f);
    guimanager->AddTrackbar(50, 352, 160, "tetrahedron_y", -10.0f, 10.0f, 0.0f, 0.5f);
    guimanager->AddTrackbar(50, 416, 160, "tetrahedron_z",  0.0f, 5.0f, 0.0f, 0.5f);

    guimanager->AddTrackbar(50, 480, 160, "torus_x", -10.0f, 10.0f, -7.0f, 0.5f);
    guimanager->AddTrackbar(50, 544, 160, "torus_y", -10.0f, 10.0f, 0.0f, 0.5f);
    guimanager->AddTrackbar(50, 608, 160, "torus_z",  0.0f, 5.0f, 3.5f, 0.5f);
#else


#endif

}

void Render::SetupView()
{
    unsigned int width = 1280;
    unsigned int height = 720;
    
    // View Matrix
    //DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 10.0f, 10.0f, 0.0f);

    float t = GetTickCount() / 1000.0f;
    m_ViewPosition = float3(std::cosf(t)*10.0f, std::sinf(t) * 10.0f, std::cosf(t) * 2.0f + 5.0f);
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(m_ViewPosition.x, m_ViewPosition.y, m_ViewPosition.z, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 2.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    m_MatView = DirectX::XMMatrixLookAtLH(Eye, At, Up);

    // Projection Matrix
    m_MatProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, width / (float)height, 0.01f, 100.0f);
    
}

void Render::RenderFrame()
{
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    GetDeviceContext()->ClearRenderTargetView(m_RenderTargetView, clearColor);
    GetDeviceContext()->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

#ifdef TASK_1
    if (guimanager->GetElementByName<Checkbox>("wireframe")->IsChecked())
    {
        GetDeviceContext()->RSSetState(m_WireframeRasterizer);
    }
    else
    {
        GetDeviceContext()->RSSetState(nullptr);
    }
#endif

    SetupView();
    ConstantBuffer cb;
    cb.matViewProjection = DirectX::XMMatrixTranspose(m_MatView * m_MatProjection);
    
#ifdef TASK_1
    cb.matWorld = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0, 0, 2));
    GetDeviceContext()->OMSetBlendState(nullptr, 0, 0xffffffff);
    GetDeviceContext()->UpdateSubresource(m_ConstantBuffer, 0, nullptr, &cb, 0, 0);
    GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_ConstantBuffer);
    m_MorphedMesh->Draw(guimanager->GetElementByName<Trackbar>("morph_factor")->GetValue());

    for (int i = 0; i < m_Meshes.size(); ++i)
    {
        GetDeviceContext()->OMSetBlendState(nullptr, 0, 0xffffffff);
        DirectX::XMMATRIX matModel = DirectX::XMMatrixIdentity();
        
        if (i == 1) // Cylinder
        {
            matModel = DirectX::XMMatrixTranslation(guimanager->GetElementByName<Trackbar>("cylinder_x")->GetValue(), 0, 0);
        }
        else if(i == 2) // Sphere
        {
            matModel = DirectX::XMMatrixTranslation(5, 5, 0);
        }
        else if(i == 3) // Tetrahedron
        {
            float x = guimanager->GetElementByName<Trackbar>("tetrahedron_x")->GetValue();
            float y = guimanager->GetElementByName<Trackbar>("tetrahedron_y")->GetValue();
            float z = guimanager->GetElementByName<Trackbar>("tetrahedron_z")->GetValue();
            matModel = DirectX::XMMatrixTranslation(x, y, z);
        }
        else if (i == 4) // Torus
        {
            matModel = DirectX::XMMatrixRotationX(guimanager->GetElementByName<Trackbar>("torus_rot")->GetValue());
            float x = guimanager->GetElementByName<Trackbar>("torus_x")->GetValue();
            float y = guimanager->GetElementByName<Trackbar>("torus_y")->GetValue();
            float z = guimanager->GetElementByName<Trackbar>("torus_z")->GetValue();
            matModel *= DirectX::XMMatrixTranslation(x, y, z);
        }
        //matModel *= DirectX::XMMatrixTranslation(0.0f, 0.0f, -2.0f);
        cb.matWorld = DirectX::XMMatrixTranspose(matModel);
        GetDeviceContext()->UpdateSubresource(m_ConstantBuffer, 0, nullptr, &cb, 0, 0);
        GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_ConstantBuffer);
        if (i == 4)
        {
            GetDeviceContext()->OMSetBlendState(m_OpacityBlend, 0, 0xffffffff);
        }

        m_Meshes[i].Draw();
    }
#else
    PSConstantBuffer pscb;
    float t = GetTickCount() / 1000.0f;

    pscb.lightPositions[0] = float3(cos(1.321 * t), sin(0.923 * t), 1.0f) * 5.0f;
    pscb.lightColors[0] = float3(1.0f, 0, 0);

    pscb.lightPositions[1] = float3(cos(2.321 * t)* 2.0f, sin(2.125 * t) * 2.0f, 1.0f) * 5.0f;
    pscb.lightColors[1] = float3(0, 1.0f, 0);

    pscb.lightPositions[2] = float3(cos(0.752 * t) * 3.0f, sin(3.687 * t) * 3.0f, 1.0f) * 5.0f;
    pscb.lightColors[2] = float3(0, 0, 1.0f);

    pscb.viewPosition = m_ViewPosition;

    for (int i = 0; i < m_Meshes.size(); ++i)
    {
        GetDeviceContext()->OMSetBlendState(nullptr, 0, 0xffffffff);
        DirectX::XMMATRIX matModel = DirectX::XMMatrixIdentity();
        pscb.phongScale = 0.0f;
        if (i == 1) // Cube
        {
        }
        else if (i == 2) // Cone
        {
            matModel = DirectX::XMMatrixTranslation(5, 5, 0);
            pscb.phongScale = 1.0f;
        }
        else if (i == 3) // Sphere
        {
            GetDeviceContext()->OMSetBlendState(m_OpacityBlend, 0, 0xffffffff);
        }
        cb.matWorld = DirectX::XMMatrixTranspose(matModel);
        GetDeviceContext()->UpdateSubresource(m_ConstantBuffer, 0, nullptr, &cb, 0, 0);
        GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_ConstantBuffer);

        GetDeviceContext()->UpdateSubresource(m_PSConstantBuffer, 0, nullptr, &pscb, 0, 0);
        GetDeviceContext()->PSSetConstantBuffers(1, 1, &m_PSConstantBuffer);

        m_Meshes[i].Draw();
    }
#endif

    
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
