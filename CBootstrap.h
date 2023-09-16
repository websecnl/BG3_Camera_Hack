#pragma once
#include <thread>

#include "CRender.h"
#include <memory>
class CCameraFov;
class CBootstrap
{
public:
	CBootstrap() { instance = this; };

	int Main(HMODULE hModule,
		DWORD  ul_reason_for_call,
		LPVOID lpReserved);
	void OnAttach();
	void OnDetach();
	void Initialize();
	void InitializeConsole();
	HWND gameWindow = NULL;
	inline static CBootstrap* instance;

	std::unique_ptr<CRender> m_Render;
	std::shared_ptr<CCameraFov> m_CamFov;
private:
	std::thread* bootThread = nullptr;
};
