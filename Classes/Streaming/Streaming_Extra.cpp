#include <StdAfx.h>


#include "../Fixture/DebugLog.h"
#include "../pxtone/pxtnService.h"
#include "../vc/pxtone.h"
#include "pxmAL.h"
#include "pxwXAudio2.h"
#include "pxWASAPI.h"
#include "Streaming_Extra.h"
#include "Streaming.h"

static enum enum_EXTRA
{
	enum_EXTRA_OPENAL,
	enum_EXTRA_XAUDIO,
	enum_EXTRA_WASAPI,
};
typedef struct STREAM_CONFIG
{
	HWND hWnd;
	long ch ;          // channel num
	long sps;          // samples per second
	long bps;          // bits per sample
	long buf;          // sample per buffer
	enum_EXTRA driver; // OpenAL | XAudio2 | WASAPI...
	PXTNPLAY_CALLBACK callback;       // 'clock' only
	PXTONEPLAY_CALLBACK callback_old; // 'clock' and 'bEnd'
};
static STREAM_CONFIG    _strm_current;

static BOOL _b_init    = FALSE;
static CRITICAL_SECTION _cs_proc_extra;


////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _SilenceFill( void *p, long buf_len ){ if( buf_len ) memset( p, (_strm_current.bps == 8 ? 0x80 : 0), buf_len ); }
static BOOL _CS_Lock    ( void ){ EnterCriticalSection( &_cs_proc_extra ); return _b_init; }
static void _CS_Unlock  ( void ){ LeaveCriticalSection( &_cs_proc_extra );                 }

////////////////////////////
// グローバル関数 //////////
////////////////////////////

BOOL Streaming_Extra_Release( void )
{
	if( !_b_init ) return TRUE;
	dlog( "stream extra release(b_init)" );
	_b_init = FALSE;
	
	dlog( "stream extra release(pxmAL)" );
	pxmAL_Release();
	
	dlog( "stream extra release(pxwXAudio2)" );
	pxwXAudio2_Release();
	
	dlog( "stream extra release(pxWASAPI)" );
	pxWASAPI_Release();
	
	dlog( "stream extra release(cs_proc_extra)" );
	DeleteCriticalSection( &_cs_proc_extra );
	
	dlog( "stream extra release(end)" );
	return TRUE;
}

BOOL Streaming_Extra_Initialize( DWORD *strm_cfg, long size )
{
	if( _b_init ) goto End;

	InitializeCriticalSection( &_cs_proc_extra );
	if( strm_cfg[ 2 ] > 44100 ) strm_cfg[ 2 ] = 44100;
	if( strm_cfg[ 2 ] <    10 ) strm_cfg[ 2 ] =    10;
	memcpy( &_strm_current, strm_cfg, sizeof(DWORD) * 7 );

	switch( _strm_current.driver )
	{
		case enum_EXTRA_OPENAL: if( !pxmAL_Initialize     ( _strm_current.hWnd, _strm_current.ch, _strm_current.sps, _strm_current.bps, _strm_current.buf, pxmAL_Proc      ) ) goto End; break;
		case enum_EXTRA_XAUDIO: if( !pxwXAudio2_Initialize( _strm_current.hWnd, _strm_current.ch, _strm_current.sps, _strm_current.bps, _strm_current.buf, pxwXAudio2_Proc ) ) goto End; break;
		case enum_EXTRA_WASAPI: if( !pxWASAPI_Initialize  ( _strm_current.hWnd, _strm_current.ch, _strm_current.sps, _strm_current.bps, _strm_current.buf, pxWASAPI_Proc   ) ) goto End; break;
		default: goto End;
	}

	_b_init  = TRUE;
End:
	if( !_b_init ) Streaming_Extra_Release();

	return _b_init;
}






/////////////////////////////
// 再生の処理
/////////////////////////////

// Open AL
BOOL pxmAL_Proc( void *arg )
{
	LPSTR p_smp;
	DWORD p_size = _strm_current.buf * _strm_current.ch * _strm_current.bps / 8;
	if( !(p_smp  = (LPSTR)malloc( p_size )) ) return FALSE;
	UINT buf;

	while( pxmAL_IsActive() )
	{
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

			if( _strm_current.callback )
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

			if( _strm_current.callback )
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

// WASAPI
BOOL pxWASAPI_Proc( void *arg )
{
	BYTE *p_smp = NULL;
	UINT bufCount, padding, p_frames, block = _strm_current.ch * _strm_current.bps / 8;
	pxWASAPI_GetBufCount( &bufCount );

	while( WaitForSingleObject( pxWASAPI_GetHandle(), INFINITE ) == WAIT_OBJECT_0 )
	{
		pxWASAPI_GetPadding( &padding );
		p_frames = bufCount - padding;
		if( p_frames != 0 )
		{
			if( _CS_Lock() )
			{
				if( !pxWASAPI_Fill( p_frames, &p_smp ) ){ _CS_Unlock(); continue; }
				if(      !pxtnServiceMoo_Is_Prepared() || !Streaming_Is() ) _SilenceFill( p_smp, p_frames * block );
				else if( !pxtnServiceMoo_Proc( p_smp, p_frames * block )  )  Streaming_Tune_Stop();
				pxWASAPI_Unfill( p_frames );
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
