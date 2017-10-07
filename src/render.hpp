#ifndef RENDER_HPP
#define RENDER_HPP

#include "gui.hpp"
#include "mathlib.hpp"
#include <Windows.h>

#include <d3d11.h>
#include <DirectXMath.h>

struct Vertex;
class Mesh;
class MorphedMesh;


class Render
{
public:
    Render();
    void Init(HWND hWnd);
    void Shutdown();
    void RenderFrame();
    ID3D11Device* GetDevice() const { return m_D3DDevice; }
    ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext; }    

private:
    void InitD3D();
    void InitScene();
    void SetupView();

    HWND                    m_hWnd;
    D3D11_VIEWPORT          m_Viewport;
    ID3D11Device*           m_D3DDevice;
    ID3D11DeviceContext*    m_DeviceContext;
    IDXGISwapChain*         m_SwapChain;
    ID3D11RenderTargetView* m_RenderTargetView;
    ID3D11DepthStencilView* m_DepthStencilView;
    
    ID3D11Buffer* m_ConstantBuffer;
    ID3D11Buffer* m_PSConstantBuffer;
    ID3D11VertexShader* m_VertexShader;
    ID3D11PixelShader* m_PixelShader;
    ID3D11InputLayout* m_InputLayout;

    DirectX::XMMATRIX m_MatView;
    DirectX::XMMATRIX m_MatProjection;
    float3 m_ViewPosition;

    ID3D11RasterizerState* m_WireframeRasterizer;
    ID3D11BlendState* m_OpacityBlend;

    std::vector<Mesh> m_Meshes;
    MorphedMesh* m_MorphedMesh;

    ID3D11SamplerState* m_SamplerLinear;
        
};

extern Render* render;

#endif // RENDER_HPP
