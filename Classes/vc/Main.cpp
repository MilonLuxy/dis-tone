#include <StdAfx.h>

#ifndef DLL_EXPORT
const char *tune_file_name  = "\\opening.pti";
//const char *tune_file_name  = "\\opening.ptcop";
const char *noise_file_name = "\\ok.ptnoise";
const char *g_window_name   = "testPxtone";
HWND g_hWnd_Main = NULL;

#include "pxtone.h"
#include "../Streaming/Streaming.h"
static LRESULT CALLBACK Procedure( HWND hWnd, UINT msg, WPARAM w, LPARAM l )
{
	if( msg == WM_CLOSE ){ DestroyWindow( hWnd ); PostQuitMessage( 0 ); return 0; }
	else return DefWindowProc( hWnd, msg, w, l );
}
int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmd, int nCmd )
{
	// Initialize --------------------------------
	WNDCLASSEX wc = {0};
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.lpszClassName = "testwindow";
	wc.style         = CS_HREDRAW|CS_VREDRAW;
	wc.lpfnWndProc   = Procedure;
	wc.hInstance     = hInst;
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
	wc.hIcon         = LoadIcon( hInst, "0" );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	if( !RegisterClassEx( &wc ) ) return 1;

	if( !(g_hWnd_Main = CreateWindow( wc.lpszClassName, "testPxtone",
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
		320, 240, HWND_DESKTOP, NULL, hInst, NULL )) ) return 1;
	
	char path_tune[ MAX_PATH ], path_noise[ MAX_PATH ];
	GetModuleFileName ( NULL, path_tune, MAX_PATH );
	PathRemoveFileSpec( path_tune );
	strcpy( path_noise, path_tune );
	strcat( path_tune, tune_file_name );
	strcat( path_noise, noise_file_name );

	ShowWindow( g_hWnd_Main, SW_SHOW );
	// -------------------------------------------

	// DLL procedure -----------------------------
	if( !pxtone_Ready( g_hWnd_Main, 2, 44100, 16, 0.1, true, (PXTONEPLAY_CALLBACK)Test_Callback ) ){ MessageBox( NULL, pxtone_GetLastError(), "err", 0x30 ); return 1; }

	PXTONENOISEBUFFER *p_noise = NULL;
	if( !(p_noise = pxtone_Noise_Create( path_noise, NULL, 2, 44100, 16 )) ){ MessageBox( NULL, "noise create", "err", 0x30 ); return 1; }
	pxtone_Noise_Release( p_noise );

	if( !pxtone_Tune_Load( NULL, NULL, path_tune ) ){ MessageBox( NULL, pxtone_GetLastError(), "err", 0x30 ); return 1; }

	if( !pxtone_Tune_Start( 0, 0 ) ){ MessageBox( NULL, "start", "err", 0x30 ); return 1; }
	Test_Callback_Init();

	MSG msg; while( GetMessage( &msg, NULL, 0, 0 ) ){ TranslateMessage( &msg ); DispatchMessage( &msg ); }

	pxtone_Tune_Fadeout( 1000 );

	while( pxtone_Tune_IsStreaming() ){ Sleep( 100 ); }

	pxtone_Release();
	// -------------------------------------------

    return 0;
}
#endif
