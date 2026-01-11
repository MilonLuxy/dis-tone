#include <StdAfx.h>


#include "../Fixture/DebugLog.h"
#include "../Fixture/pxMem.h"
#include "DxSound.h"
#include "pxMME.h"
#include "Streaming.h"
#include "ActiveTone.h"
#include "../vc/pxtone.h"
#include "../pxtone/pxtnService.h"


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

void *Streaming_GetDirectSound( void ){ return DxSound_GetDirectSoundPointer(); }

BOOL Streaming_Release( void )
{
	dlog( "streaming release(1)" );
	if( !_b_init ) return TRUE;
	_b_init = FALSE;

	dlog( "streaming release(2)" );
	DxSound_Release();

	dlog( "streaming release(3)" );
	if( !pxMME_Release() ){ MessageBox( NULL, "release WAVEMAPPER", "fatal", MB_OK ); return FALSE; }

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
		if( !DxSound_Initialize(
				_strm_current.hWnd,
				_strm_current.channel_num,
				_strm_current.sps,
				_strm_current.bps,
				_strm_current.smp_per_buf
				           ) ) goto End;
	}
	else
	{
		if( !pxMME_Initialize  (
				_strm_current.hWnd,
				_strm_current.channel_num,
				_strm_current.sps,
				_strm_current.bps,
				_strm_current.smp_per_buf,
				pxMME_Proc ) ) goto End;
	}

	_b_init  = TRUE;
End:
	if( !_b_init ) Streaming_Release();

	return _b_init;
}

BOOL Streaming_Tune_Start( const void *p_prep )
{
	dlog( "Streaming_Tune_Start();" );
	if( _proc_state != _PROC_STOPPED ) return FALSE;
	if( !pxtnServiceMoo_Preparation( (pxtnVOMITPREPARATION*)p_prep ) ) return FALSE;
	_proc_state = _PROC_PLAYING;
	return TRUE;
}
void Streaming_Tune_Stop( void )
{
	if( _proc_state == _PROC_PLAYING )
	{
		_proc_state = _PROC_PAUSED;
		pxtnServiceMoo_Release();
		_proc_state = _PROC_STOPPED;
	}
}
void Streaming_Tune_Fadeout( long msec ){ if( _proc_state == _PROC_PLAYING ) pxtnServiceMoo_SetFade( -1, msec ); }
BOOL Streaming_Is          ( void      ){ return ( _proc_state == _PROC_PLAYING || _proc_state == _PROC_PAUSED ); }

void Streaming_GetQuality( long *p_ch_num, long *p_sps, long *p_bps, long *p_smp_buf )
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
void Streaming_Set_SampleInfo( long    ch_num, long    sps, long    bps )
{
	_ch_num = ch_num;
	_sps    = sps   ;
	_bps    = bps   ;
}
void Streaming_Get_SampleInfo( long *p_ch_num, long *p_sps, long *p_bps )
{
	if( p_ch_num ) *p_ch_num = _ch_num;
	if( p_sps    ) *p_sps    = _sps   ;
	if( p_bps    ) *p_bps    = _bps   ;
}



// Direct Sound
BOOL DxSound_Proc( void *arg )
{
	DWORD index;
	DS_STREAMING_THREAD *hThread = (DS_STREAMING_THREAD*)arg;

	while( (index = WaitForMultipleObjects( hThread->count, hThread->events, FALSE, INFINITE )) < DS_BUFFER_COUNT - 1 )
	{
		DWORD  buf_index = (index + 1) % (DS_BUFFER_COUNT - 1);
		DWORD  dwbuf1, dwbuf2;
		LPVOID lpbuf1, lpbuf2;
		if( _CS_Lock() )
		{
			if( DxSound_Lock( buf_index, &lpbuf1, &dwbuf1, &lpbuf2, &dwbuf2 ) )
			{
				if( !pxtnServiceMoo_Is_Prepared() || !Streaming_Is() )
				{
					_SilenceFill( lpbuf1, dwbuf1 );
					_SilenceFill( lpbuf2, dwbuf2 );
				}
				else
				{
					if( !pxtnServiceMoo_Proc( lpbuf1, dwbuf1 ) ) Streaming_Tune_Stop();
					if( !pxtnServiceMoo_Proc( lpbuf2, dwbuf2 ) ) Streaming_Tune_Stop();
				}

				ActiveTone_Voice_Sampling( lpbuf1, dwbuf1 );
				ActiveTone_Voice_Sampling( lpbuf2, dwbuf2 );

				DxSound_Unlock( lpbuf1, dwbuf1, lpbuf2, dwbuf2 );
			}
			_CS_Unlock();
		}

		if( _strm_current.callback )
		{
			long clock = pxtnServiceMoo_Get_NowClock();
			_strm_current.callback_old = (PXTONEPLAY_CALLBACK)_strm_current.callback;
			_strm_current.callback_old( clock, pxtnServiceMoo_Is_Finished() );
		}
	}
	ExitThread( 0 );

	return TRUE;
}

// Wave Mapper
BOOL pxMME_Proc( void *arg )
{
	MSG msg;
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		if( msg.message == MM_WOM_CLOSE ) break;
		if( msg.message == MM_WOM_DONE )
		{
			WAVEHDR *waveHdr = (WAVEHDR*)msg.lParam;
			if( _CS_Lock() )
			{
				if( waveHdr->dwFlags & WHDR_PREPARED && !( waveHdr->dwFlags & WHDR_INQUEUE ) )
				{
					pxMME_Header_Clean( waveHdr );

					if( !pxtnServiceMoo_Is_Prepared() || !Streaming_Is() )
					{
						_SilenceFill( waveHdr->lpData, waveHdr->dwBufferLength );
					}
					else
					{
						if( !pxtnServiceMoo_Proc( waveHdr->lpData, waveHdr->dwBufferLength ) ) Streaming_Tune_Stop();
					}

					ActiveTone_Voice_Sampling( waveHdr->lpData, waveHdr->dwBufferLength );

					pxMME_Header_Play( waveHdr );
				}
				_CS_Unlock();
			}
			
			if( _strm_current.callback )
			{
				long clock = pxtnServiceMoo_Get_NowClock();
				_strm_current.callback_old = (PXTONEPLAY_CALLBACK)_strm_current.callback;
				_strm_current.callback_old( clock, pxtnServiceMoo_Is_Finished() );
			}
		}
	}

	return TRUE;
}
