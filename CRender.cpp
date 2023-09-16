#include <Windows.h>
#include "CRender.h"
#include "Minhook/MinHook.h"
#include <mutex>
#include <iostream>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include <vector>
#include "CCameraFov.h"

typedef HRESULT(__stdcall* D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef void(__stdcall* D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
typedef void(__stdcall* D3D11CreateQueryHook) (ID3D11Device* pDevice, const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery);
typedef void(__stdcall* D3D11PSSetShaderResourcesHook) (ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews);
typedef void(__stdcall* D3D11ClearRenderTargetViewHook) (ID3D11DeviceContext* pContext, ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4]);
typedef HRESULT(__stdcall* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
ResizeBuffers oResizeBuffers;

static HWND                     g_hWnd = nullptr;
static HMODULE					g_hModule = nullptr;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static std::once_flag           g_isInitialized;

D3D11PresentHook                phookD3D11Present = nullptr;
D3D11DrawIndexedHook            phookD3D11DrawIndexed = nullptr;
D3D11CreateQueryHook			phookD3D11CreateQuery = nullptr;
D3D11PSSetShaderResourcesHook	phookD3D11PSSetShaderResources = nullptr;
D3D11ClearRenderTargetViewHook  phookD3D11ClearRenderTargetViewHook = nullptr;
inline ID3D11RenderTargetView* render_target_view;
DWORD_PTR* pSwapChainVTable = nullptr;
DWORD_PTR* pDeviceVTable = nullptr;
DWORD_PTR* pDeviceContextVTable = nullptr;
static bool g_bInitialised = false;
static WNDPROC g_oWndProc;
static bool g_IsMenuOpened = false;
HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	HRESULT ret = pSwapChain->GetDevice(__uuidof(ID3D11Device), (PVOID*)ppDevice);

	if (SUCCEEDED(ret))
		(*ppDevice)->GetImmediateContext(ppContext);

	return ret;
}
static LRESULT hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (!ImGui::GetCurrentContext()) {
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(hWnd);
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = io.LogFilename = nullptr;
	}

	LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
		LPARAM lParam);
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	return CallWindowProc(g_oWndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT  CRender::Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!g_bInitialised) {

		std::cout << "\t[+] Present Hook called by first time" << std::endl;
		if (FAILED(GetDeviceAndCtxFromSwapchain(pSwapChain, &g_pd3dDevice, &g_pd3dContext)))
			return phookD3D11Present(pSwapChain, SyncInterval, Flags);

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); // ...

		ImGui_ImplWin32_Init(g_hWnd);
		ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);


		ID3D11Texture2D* pBackBuffer;

		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &render_target_view);
		pBackBuffer->Release();
		g_oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(g_hWnd, GWLP_WNDPROC, reinterpret_cast<long long>(hkWndProc)));
		g_bInitialised = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Insert, false)) {
		g_IsMenuOpened = !g_IsMenuOpened;
	}
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();

	ImGui::NewFrame();


	auto& style = ImGui::GetStyle();
	auto& io = ImGui::GetIO();
	io.MouseDrawCursor = g_IsMenuOpened;

	if (g_IsMenuOpened) {
		m_CamFov->RenderGui();
	}
	m_CamFov->OnFrame();



	ImGui::EndFrame();

	ImGui::Render();

	g_pd3dContext->OMSetRenderTargets(1, &render_target_view, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return phookD3D11Present(pSwapChain, SyncInterval, Flags);
}

void CRender::DrawIndexedHook(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	return phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

void  CRender::D3D11CreateQuery(ID3D11Device* pDevice, const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery)
{
	if (pQueryDesc->Query == D3D11_QUERY_OCCLUSION)
	{
		D3D11_QUERY_DESC oqueryDesc = CD3D11_QUERY_DESC();
		(&oqueryDesc)->MiscFlags = pQueryDesc->MiscFlags;
		(&oqueryDesc)->Query = D3D11_QUERY_TIMESTAMP;

		return phookD3D11CreateQuery(pDevice, &oqueryDesc, ppQuery);
	}

	return phookD3D11CreateQuery(pDevice, pQueryDesc, ppQuery);
}

UINT pssrStartSlot;
D3D11_SHADER_RESOURCE_VIEW_DESC Descr;

void CRender::D3D11PSSetShaderResources(ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
	pssrStartSlot = StartSlot;

	for (UINT j = 0; j < NumViews; j++)
	{
		ID3D11ShaderResourceView* pShaderResView = ppShaderResourceViews[j];
		if (pShaderResView)
		{
			pShaderResView->GetDesc(&Descr);

			if ((Descr.ViewDimension == D3D11_SRV_DIMENSION_BUFFER) || (Descr.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX))
			{
				continue; //Skip buffer resources
			}
		}
	}

	return phookD3D11PSSetShaderResources(pContext, StartSlot, NumViews, ppShaderResourceViews);
}

void __stdcall CRender::ClearRenderTargetViewHook(ID3D11DeviceContext* pContext, ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
	return phookD3D11ClearRenderTargetViewHook(pContext, pRenderTargetView, ColorRGBA);
}


D3D11_HOOK_API void ImplHookDX11_Init(HMODULE hModule, void* hwnd)
{
	g_hWnd = (HWND)hwnd;
	g_hModule = hModule;

}

D3D11_HOOK_API void ImplHookDX11_Shutdown()
{
	if (MH_DisableHook(MH_ALL_HOOKS)) { return; };
	if (MH_Uninitialize()) { return; }
}
static bool CreateDeviceD3D11(HWND hWnd) {
	// Create the D3DDevice
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { };
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;

	const D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_NULL, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &g_pSwapChain, &g_pd3dDevice, nullptr, nullptr);
	if (hr != S_OK) {
		printf("[!] D3D11CreateDeviceAndSwapChain() failed. [rv: %lu]\n", hr);
		return false;
	}

	return true;
}
HRESULT hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{




	if (render_target_view) {
		g_pd3dContext->OMSetRenderTargets(0, 0, 0);
		render_target_view->Release();
	}

	HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ID3D11Texture2D* pBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
	// Perform error handling here!

	g_pd3dDevice->CreateRenderTargetView(pBuffer, NULL, &render_target_view);
	// Perform error handling here!
	pBuffer->Release();

	g_pd3dContext->OMSetRenderTargets(1, &render_target_view, NULL);

	// Set up the viewport.
	D3D11_VIEWPORT vp;
	vp.Width = Width;
	vp.Height = Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dContext->RSSetViewports(1, &vp);
	return hr;
}
void CRender::Hook() {
	AllocConsole();

	auto hConOut = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);

	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);

	SetConsoleTitleA("AME-Console");
	std::cout << "merge secundele \n";

	EnumWindows([](HWND hwnd, LPARAM lParam) {
		char windowTitle[256];


		DWORD lpdwProcessId;
		GetWindowThreadProcessId(hwnd, &lpdwProcessId);
		if (lpdwProcessId == GetCurrentProcessId()) {
			GetWindowText(hwnd, windowTitle, sizeof(windowTitle));
			std::cout << "Window title: " << windowTitle << std::endl;

			if (std::string(windowTitle).find("Baldur's Gate 3") != std::string::npos)
			{
				g_hWnd = hwnd;
				std::cout << "l-a gassit " << std::endl;
				return FALSE;
			}
		}
		return TRUE; // Continue enumerating
		}, 0);



	std::cout << "g_hWnd " << g_hWnd << std::endl;

	D3D_FEATURE_LEVEL obtainedLevel;
	DXGI_SWAP_CHAIN_DESC sd;
	{
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.OutputWindow = g_hWnd;
		sd.SampleDesc.Count = 1;
		sd.Windowed = ((GetWindowLongPtr(g_hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		sd.BufferDesc.Width = 1;
		sd.BufferDesc.Height = 1;
		sd.BufferDesc.RefreshRate.Numerator = 0;
		sd.BufferDesc.RefreshRate.Denominator = 1;
	}
	const D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_0,
	};
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_NULL, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, nullptr);
	if (FAILED(hr))
	{
		(0, "Failed to create device andMessageBox swapchain.", "Fatal Error", MB_ICONERROR);
		return;
	}

	pSwapChainVTable = (DWORD_PTR*)(g_pSwapChain);
	pSwapChainVTable = (DWORD_PTR*)(pSwapChainVTable[0]);

	pDeviceVTable = (DWORD_PTR*)(g_pd3dDevice);
	pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];



	if (MH_Initialize() != MH_OK) { return; }
	if (MH_CreateHook((DWORD_PTR*)pSwapChainVTable[8], Present, reinterpret_cast<void**>(&phookD3D11Present)) != MH_OK) { return; }

	if (MH_CreateHook((DWORD_PTR*)pSwapChainVTable[13], &hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers)) != MH_OK) {
	}

	MH_EnableHook(MH_ALL_HOOKS);
}

void CRender::Unhook() {
	
	g_pd3dDevice->Release();
	g_pd3dContext->Release();
	g_pSwapChain->Release();

	if (MH_DisableHook(MH_ALL_HOOKS)) { return; };
	if (MH_Uninitialize()) { return; }
}
