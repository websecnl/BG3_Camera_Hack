#pragma once
#include <d3d11.h>
#include <memory>
#define D3D11_HOOK_API
class CCameraFov;

// Use for rendering graphical user interfaces (for example: ImGui) or other.
extern D3D11_HOOK_API void ImplHookDX11_Present(ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swap_chain);

// Use for initialize hook.
D3D11_HOOK_API void	       ImplHookDX11_Init(HMODULE hModule, void* hwnd);

// Use for untialize hook (ONLY AFTER INITIALIZE).
D3D11_HOOK_API void	       ImplHookDX11_Shutdown();

class CRender
{
public:
	inline static std::shared_ptr<CCameraFov> m_CamFov;

	CRender(std::shared_ptr<CCameraFov> _mCamFov) {
		m_CamFov = _mCamFov;
	}
	static HRESULT Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	static void  DrawIndexedHook(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
	static void  D3D11CreateQuery(ID3D11Device* pDevice, const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery);
	static void  ClearRenderTargetViewHook(ID3D11DeviceContext* pContext, ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4]);
	void Hook();
	void Unhook();
	static void D3D11PSSetShaderResources(ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews);
};

