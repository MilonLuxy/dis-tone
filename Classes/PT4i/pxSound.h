#pragma once

#include "../pxtone/pxStdDef.h"

#ifdef pxINCLUDE_PT4i

#define MAX_VOICE    6

bool pxSound_Initialize( HWND hWnd, int ch_num, int sps, int bps, bool b_dxsound );	
void pxSound_Release   ( void );
bool pxSound_IsActive  ( void );
void pxSound_Play      ( int idx, bool b_loop );
void pxSound_Stop      ( int idx              );
void pxSound_SetVolume ( int idx, float vol   );
void pxSound_SetPitch  ( int idx, float pitch );

#endif
