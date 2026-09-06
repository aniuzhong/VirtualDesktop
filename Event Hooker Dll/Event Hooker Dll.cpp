#include "stdafx.h"

extern HINSTANCE g_hInstance;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    if (DLL_PROCESS_ATTACH == fdwReason)
    {
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}
