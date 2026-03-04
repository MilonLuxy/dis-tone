#include <StdAfx.h>


#include "../Fixture/DebugLog.h"
#include "../Fixture/pxMem.h"
#include "../pxtone/pxtnService.h"
#include "../PT4i/PT4i.h"
#include "../PT4i/pxSound.h"
#include "../vc/pxtone.h"
#include "pxmAL.h"
#include "pxwXAudio2.h"
#include "ActiveTone.h"
#include "Streaming.h"


static enum STREAM_PROC
{
    _PROC_STOPPED,
    _PROC_PLAYING,
    _PROC_PAUSED ,
};

typedef struct STREAM_CONFIG
{
	HWND hWnd;
	long channel_num;
	long sps;
	long bps;
	long smp_per_buf;
	BOOL bDirectSound;
	PXTNPLAY_CALLBACK callback;       // 'clock' only
	PXTONEPLAY_CALLBACK callback_old; // 'clock' and 'bEnd'
};

static BOOL _b_init    = FALSE;
static BOOL _b_pti     = FALSE;
static CRITICAL_SECTION _cs_proc;
static STREAM_PROC      _proc_state;
static STREAM_CONFIG    _strm_current;
static long _ch_num = 0;
static long _sps    = 0;
static long _bps    = 0;


////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _SilenceFill( void *p, long buf_len ){ if( buf_len ) memset( p, (_strm_current.bps == 8 ? 0x80 : 0), buf_len ); }
static BOOL _CS_Lock    ( void ){ EnterCriticalSection( &_cs_proc ); return _b_init; }
static void _CS_Unlock  ( void ){ LeaveCriticalSection( &_cs_proc );                 }
static void _ResetVolume( void ){ _proc_state = _PROC_STOPPED; pxtnServiceMoo_Set_Master_Volume( 1 ); }
static void _ProcStop   ( void ){ _proc_state = _PROC_STOPPED; }

////////////////////////////
// グローバル関数 //////////
////////////////////////////

void *Streaming_GetDirectSound( void ){ return NULL; }

BOOL Streaming_Release( void )
{
	dlog( "streaming release(1)" );
	if( !_b_init ) return TRUE;
#ifdef pxINCLUDE_PT4i
	pxSound_Release();
#endif
	_b_init = FALSE;

	dlog( "streaming release(2)" );
	pxmAL_Release();

	dlog( "streaming release(3)" );
	if( !pxwXAudio2_Release() ){ MessageBox( NULL, "release XAudio2", "fatal", MB_OK ); return FALSE; }

	dlog( "streaming release(4)" );
	DeleteCriticalSection( &_cs_proc );

	dlog( "streaming release(5)" );
	ActiveTone_Voice_Release();

	dlog( "streaming release(6)" );
	return TRUE;
}

BOOL Streaming_Initialize( DWORD *strm_cfg, long size )
{
	if( _b_init ) goto End;

	InitializeCriticalSection( &_cs_proc );
	_ResetVolume();
	
	pxMem_cap( (long*)&strm_cfg[2], 44100, 10 );
	if( !ActiveTone_Voice_Initialize( strm_cfg[1], strm_cfg[2], strm_cfg[3], size ) ) goto End;

	memcpy( &_strm_current, strm_cfg, sizeof(DWORD) * 7 );

	if( _strm_current.bDirectSound )
	{
		if( !pxmAL_Initialize(
				_strm_current.hWnd,
				_strm_current.channel_num,
				_strm_current.sps,
				_strm_current.bps,
				_strm_current.smp_per_buf,
				pxmAL_Proc ) ) goto End;
	}
	else
	{
		if( !pxwXAudio2_Initialize(
				_strm_current.hWnd,
				_strm_current.channel_num,
				_strm_current.sps,
				_strm_current.bps,
				_strm_current.smp_per_buf,
				pxwXAudio2_Proc ) ) goto End;
	}

#ifdef pxINCLUDE_PT4i
	if( !PT4i_Initialize(
		_strm_current.hWnd,
		_strm_current.channel_num,
		_strm_current.sps,
		_strm_current.bps,
		_strm_current.smp_per_buf,
		_strm_current.bDirectSound ) ) goto End;
#endif

	_b_init  = TRUE;
End:
	if( !_b_init ) Streaming_Release();

	return _b_init;
}

BOOL Streaming_Tune_Start( const void *p_prep, BOOL b_pti )
{
	if( _proc_state != _PROC_STOPPED ) return FALSE;

	dlog( "Streaming_Tune_Start();" );

#ifdef pxINCLUDE_PT4i
	if( b_pti )
	{
		if( !PT4i_Start() ) return FALSE;
	}
	else
	{
		if( !pxtnServiceMoo_Preparation( (pxtnVOMITPREPARATION*)p_prep ) ) return FALSE;
	}
	_b_pti = b_pti;
#else
	if( !pxtnServiceMoo_Preparation( (pxtnVOMITPREPARATION*)p_prep ) ) return FALSE;
#endif

	_proc_state = _PROC_PLAYING;
	return TRUE;
}
void Streaming_Tune_Stop( void )
{
	if( _proc_state == _PROC_PLAYING )
	{
		_proc_state = _PROC_PAUSED;
		pxtnServiceMoo_Release();
#ifdef pxINCLUDE_PT4i
		PT4i_Stop();
#endif
		_proc_state = _PROC_STOPPED;
	}
}
void Streaming_Tune_Fadeout( int msec )
{
	if( _proc_state == _PROC_PLAYING )
	{
#ifdef pxINCLUDE_PT4i
		if( _b_pti ) PT4i_SetFade(  msec );
#endif
		pxtnServiceMoo_SetFade( _FADE_OUT, msec );
	}
}
BOOL Streaming_Is( void ){ return ( _proc_state == _PROC_PLAYING || _proc_state == _PROC_PAUSED ); }

void Streaming_GetQuality( s32 *p_ch_num, s32 *p_sps, s32 *p_bps, s32 *p_smp_buf )
{
	if( _b_init )
	{
		if( p_ch_num  ) *p_ch_num  = _strm_current.channel_num;
		if( p_sps     ) *p_sps     = _strm_current.sps        ;
		if( p_bps     ) *p_bps     = _strm_current.bps        ;
		if( p_smp_buf ) *p_smp_buf = _strm_current.smp_per_buf;
	}
	else
	{
		if( p_ch_num  ) *p_ch_num  = 0;
		if( p_sps     ) *p_sps     = 0;
		if( p_bps     ) *p_bps     = 0;
		if( p_smp_buf ) *p_smp_buf = 0;
	}
}
void Streaming_Set_SampleInfo( s32    ch_num, s32    sps, s32    bps )
{
	_ch_num = ch_num;
	_sps    = sps   ;
	_bps    = bps   ;
}
void Streaming_Get_SampleInfo( s32 *p_ch_num, s32 *p_sps, s32 *p_bps )
{
	if( p_ch_num ) *p_ch_num = _ch_num;
	if( p_sps    ) *p_sps    = _sps   ;
	if( p_bps    ) *p_bps    = _bps   ;
}






/////////////////////////////
// 再生の処理
/////////////////////////////

// PTI playback
#ifdef pxINCLUDE_PT4i
void _PTI_Proc( void )
{
	if( !PT4i_Procedure() ) Streaming_Tune_Stop();
	
	if( _strm_current.callback )
	{
		long clock = PT4i_Get_NowEve();
		_strm_current.callback_old = (PXTONEPLAY_CALLBACK)_strm_current.callback;
		_strm_current.callback_old( clock, PT4i_IsFinised() );
	}
}
#endif

// Open AL
BOOL pxmAL_Proc( void *arg )
{
	LPSTR p_smp;
	DWORD p_size = _strm_current.smp_per_buf * _strm_current.channel_num * _strm_current.bps / 8;
	if( !(p_smp  = (LPSTR)malloc( p_size )) ) return FALSE;
	UINT buf;

	while( pxmAL_IsActive() )
	{

#ifdef pxINCLUDE_PT4i
			if( _b_pti ){ _PTI_Proc(); if( !pxSound_IsActive() ) break; }
#endif

		DWORD processed = pxmAL_GetProcessed() + 1;
		while( processed-- > 1 )
		{
			pxmAL_Unregist( &buf );

			if( _CS_Lock() )
			{
				if(      !pxtnServiceMoo_Is_Prepared() || !Streaming_Is() ) _SilenceFill( p_smp, p_size );
				else if( !pxtnServiceMoo_Proc( p_smp, p_size )            )  Streaming_Tune_Stop();
				pxmAL_Regist( &buf, p_smp, p_size );
				_CS_Unlock();
			}

			if( _strm_current.callback && !_b_pti )
			{
				long clock = pxtnServiceMoo_Get_NowClock();
				_strm_current.callback_old = (PXTONEPLAY_CALLBACK)_strm_current.callback;
				_strm_current.callback_old( clock, pxtnServiceMoo_Is_Finished() );
			}
		}
		Sleep( 2 );
	}
	if( p_smp ) free( p_smp );

	return TRUE;
}

// XAudio2
static void Convert8To16( const BYTE *src_data, short *dst_data, DWORD p_size )
{
	for( DWORD i = 0; i < p_size; i++ ) dst_data[ i ] = ((short)src_data[ i ] - 128) << 8;
}
BOOL pxwXAudio2_Proc( void *arg )
{
	DWORD buf_idx = 0;
	BYTE *p_smp8  = NULL;
	if( !pxwXAudio2_8bitAllocate( &p_smp8 ) ) return FALSE;

	while( pxwXAudio2_IsActive() )
	{

#ifdef pxINCLUDE_PT4i
		if( _b_pti ){ _PTI_Proc(); if( !pxSound_IsActive() ) break; }
#endif

		DWORD queued  = pxwXAudio2_GetState();
		while( queued < XA_BUFFER_COUNT )
		{
			BYTE *p_smp16; DWORD buf_size;
			pxwXAudio2_GetBufs( &p_smp16, &buf_size, buf_idx );

			if( _CS_Lock() )
			{
				if( !pxtnServiceMoo_Is_Prepared() || !Streaming_Is() ) _SilenceFill( p_smp16, buf_size );
				else if(  _strm_current.bps == 16 && !pxtnServiceMoo_Proc( p_smp16, buf_size  ) ) Streaming_Tune_Stop();
				else if(  _strm_current.bps ==  8 && !pxtnServiceMoo_Proc( p_smp8 , buf_size/2) ) Streaming_Tune_Stop();

				if( _strm_current.bps == 8 ) Convert8To16( p_smp8, (short*)p_smp16, buf_size/2 );
				pxwXAudio2_SubmitAudio( p_smp16, buf_size );
				buf_idx = (buf_idx + 1) % XA_BUFFER_COUNT;
				_CS_Unlock();
			}
			queued++;

			if( _strm_current.callback && !_b_pti )
			{
				long clock = pxtnServiceMoo_Get_NowClock();
				_strm_current.callback_old = (PXTONEPLAY_CALLBACK)_strm_current.callback;
				_strm_current.callback_old( clock, pxtnServiceMoo_Is_Finished() );
			}
		}
		Sleep( 2 );
	}
	if( p_smp8 ) free( p_smp8 );

	return TRUE;
}






/////////////////////////////
// コールバックのテスト
/////////////////////////////

static long _play_button_anime = 2;
static long _if_Player_GetPlayButtonAnime(         ){ return _play_button_anime;      }
static void _if_Player_SetPlayButtonAnime( long no ){        _play_button_anime = no; }

#define PROGRESS_WIDTH  126
#define PROGRESS_HEIGHT   6
#define PROGRESS_X       17
#define PROGRESS_Y       35
static RECT progress_rect = {0};
static long _clock_count, _clock_end;

// 再生開始
void Test_Callback_Init( void )
{
	long beat_num, beat_clock, meas_num;
	pxtone_Tune_GetInformation( &beat_num, NULL, &beat_clock, &meas_num );

	_clock_count = 0;
	_clock_end   = beat_clock * beat_num * meas_num;
}

BOOL Test_Callback( long clock, BOOL bEnd ) // from ptPlayer
{
	// play..
	if( !bEnd )
	{
		if( _if_Player_GetPlayButtonAnime() == 0 ) _if_Player_SetPlayButtonAnime( 2 );
		if( clock >= 0 && clock != _clock_count )
		{
			long left  = PROGRESS_X + clock * PROGRESS_WIDTH / _clock_end;
			if( left != progress_rect.left )
			{
				progress_rect.left   = left;
				progress_rect.right  = progress_rect.left + 2;
				progress_rect.top    = PROGRESS_Y;
				progress_rect.bottom = PROGRESS_Y + PROGRESS_HEIGHT;

				if( progress_rect.right > PROGRESS_X + PROGRESS_WIDTH ) progress_rect.right = PROGRESS_X + PROGRESS_WIDTH;
			}
			_clock_count = clock;
		}
	}
	// stop..
	else
	{
		if( _if_Player_GetPlayButtonAnime() > 1 )
		{
			_if_Player_SetPlayButtonAnime( 0 ); // goes back to play button
			progress_rect.left   = PROGRESS_X;
			progress_rect.top    = PROGRESS_Y;
			progress_rect.bottom = PROGRESS_Y;
			progress_rect.right  = PROGRESS_X;
		}
	}

#ifndef DLL_EXPORT
	extern const char *g_window_name;
	char str[ 64 ];
	sprintf( str, "%s [%d / %d] - [%s]", g_window_name, clock, _clock_end, bEnd ? "stopped" : "playing" );
	extern HWND g_hWnd_Main;
	SetWindowText( g_hWnd_Main, str );
#endif

	return TRUE;
}
