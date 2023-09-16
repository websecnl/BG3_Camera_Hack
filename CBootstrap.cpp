#include "pch.h"

#include <functional>

#include "CBootstrap.h"
#include "CCameraFov.h"

int CBootstrap::Main(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        bootThread = new std::thread(std::bind(&CBootstrap::Initialize, this));
    }
    break;
    case DLL_PROCESS_DETACH:
        OnDetach();
        break;
    }
    return 0;
}
void CBootstrap::OnAttach()
{
  
}
void CBootstrap::OnDetach()
{
}
void CBootstrap::Initialize()
{
    m_CamFov = std::make_shared<CCameraFov>();
    m_CamFov->Init();
    m_Render = std::make_unique<CRender>(m_CamFov);
    m_Render->Hook();
}