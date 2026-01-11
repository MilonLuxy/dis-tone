#include <StdAfx.h>

#ifdef DLL_EXPORT
HINSTANCE g_h_inst_dll = NULL;

bool APIENTRY DllMain( HANDLE hInst, DWORD reason, LPVOID lpReserved )
{
    switch( reason )
	{
		case DLL_PROCESS_ATTACH: g_h_inst_dll = (HINSTANCE)hInst;
		case DLL_THREAD_ATTACH :
		case DLL_THREAD_DETACH :
		case DLL_PROCESS_DETACH:
			break;
    }
    return true;
}
#endif
