#pragma once

#include "../pxtone/pxStdDef.h"

void *Streaming_GetDirectSound( void );
BOOL  Streaming_Release       ( void );
BOOL  Streaming_Initialize    ( DWORD *strm_cfg, long size );
BOOL  Streaming_Tune_Start    ( const void *p_prep, BOOL b_pti );
void  Streaming_Tune_Stop     ( void );
void  Streaming_Tune_Fadeout  ( int msec );
BOOL  Streaming_Is            ( void );
void  Streaming_GetQuality    ( s32 *p_ch_num, s32 *p_sps, s32 *p_bps, s32 *p_smp_buf );
void  Streaming_Set_SampleInfo( s32    ch_num, s32    sps, s32    bps                 );
void  Streaming_Get_SampleInfo( s32 *p_ch_num, s32 *p_sps, s32 *p_bps                 );

BOOL  DxSound_Proc            ( void *arg );
BOOL  pxMME_Proc              ( void *arg );

void Test_Callback_Init( void );
BOOL Test_Callback( long clock, BOOL bEnd );