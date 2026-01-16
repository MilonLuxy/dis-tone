#pragma once

#include <mmeapi.h>
#pragma comment( lib, "winmm" )

#define WAVEHDR_NUM      2

BOOL pxMME_Initialize  ( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread );
BOOL pxMME_Release     ( void );
BOOL pxMME_Header_Clean( WAVEHDR *p_waveHdr );
BOOL pxMME_Header_Play ( WAVEHDR *p_waveHdr );