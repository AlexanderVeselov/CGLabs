#ifndef RENDER_HPP
#define RENDER_HPP

#include "gui.hpp"
#include "mathlib.hpp"
#include <memory>
#include <Windows.h>

#include <d3d11.h>
// https://stackoverflow.com/questions/30267730/is-this-a-data-alignment-crash-potentially-involving-stack-misalignment-xnama
// https://msdn.microsoft.com/en-us/library/windows/desktop/ee418725.aspx#type_usage_guidelines_
// On Windows x64, all heap allocations are 16-byte aligned, but for Windows x86, they are only 8-byte aligned.
#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>

struct Vertex;
class Mesh;
class MorphedMesh;
class Camera;
struct ViewSetup;


class Render
{
public:
    Render();
    void Init(HWND hWnd);
    void Shutdown();
    void RenderFrame();
    const HWND GetHWND() const { return m_hWnd; }
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
    
    ID3D11RasterizerState* m_WireframeRasterizer;
    ID3D11BlendState* m_OpacityBlend;

    std::vector<Mesh> m_Meshes;
    std::vector<ViewSetup> m_Views;

    std::unique_ptr<Camera> m_Camera;

    ID3D11SamplerState* m_SamplerLinear;
        
};

extern Render* render;

#endif // RENDER_HPP
