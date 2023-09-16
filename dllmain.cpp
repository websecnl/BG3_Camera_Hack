// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "CBootstrap.h"
#include "CRender.h"
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        static  CBootstrap instance;
        instance.Main(hModule, ul_reason_for_call, lpReserved);
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

