#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "pxwXAudio2.h"
#include <XAudio2.h>


static IXAudio2               *_xa2          = NULL;
static IXAudio2MasteringVoice *_voice_master = NULL;
static IXAudio2SourceVoice    *_xa2_src      = NULL;
static BYTE                   *_bufs[ XA_BUFFER_COUNT ];
static DWORD                   _buf_size     =   0 ;
static HANDLE                  _hThread      = NULL;
static DWORD                   _thrd_id      = NULL;
static volatile BOOL           _thrd_active  = FALSE;

#define XA_OK           0


////////////////////////////
// グローバル関数 //////////
////////////////////////////

BOOL pxwXAudio2_Release( void )
{
	_thrd_active = FALSE;
	if( _hThread ){ WaitForSingleObject( _hThread, INFINITE ); CloseHandle( _hThread ); _hThread = NULL; }
	_thrd_id = 0;

	if( _xa2_src      ){ _xa2_src     ->DestroyVoice(); _xa2_src      = NULL; }
	if( _voice_master ){ _voice_master->DestroyVoice(); _voice_master = NULL; }
	if( _xa2          ){ _xa2         ->Release     (); _xa2          = NULL; }
	if( _bufs )
	{
		for( long i = 0; i < XA_BUFFER_COUNT; i++ ) pxMem_free( (void**)&_bufs[ i ] );
		pxMem_free( (void**)&_bufs );
	}

	return TRUE;
}

BOOL pxwXAudio2_Initialize( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread )
{
	BOOL b_ret = FALSE;
	static const WORD _BITPERSAMPLE16 = 16; // XAudio2 doesn't support 8-bit playback

	if( XAudio2Create( &_xa2, 0, XAUDIO2_DEFAULT_PROCESSOR ) != XA_OK ) goto End;
//	UINT device_num;
//	if( _xa2->GetDeviceCount( &device_num ) != XA_OK ) goto End; // Needs to include "$(DXSDK_DIR)Include;"
//	if( device_num <= 0 ) goto End;
	if( _xa2->CreateMasteringVoice( &_voice_master, ch_num, sps, 0, 0, 0 ) != XA_OK ) goto End;

	WAVEFORMATEX fmt    = {};
	fmt.cbSize          = 0              ;
	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = (WORD)ch_num   ;
	fmt.nSamplesPerSec  = sps            ;
	fmt.wBitsPerSample  = _BITPERSAMPLE16;
	fmt.nBlockAlign     = fmt.nChannels   * fmt.wBitsPerSample / 8;
	fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;

	if( _xa2->CreateSourceVoice( &_xa2_src, &fmt, 0, XAUDIO2_DEFAULT_FREQ_RATIO, 0, 0, 0 ) != XA_OK ) goto End;

	XAUDIO2_BUFFER xabuf = {};
	_buf_size = smp_buf * ch_num * _BITPERSAMPLE16 / 8;
	for( long i = 0; i < XA_BUFFER_COUNT; i++ )
	{
		if( !pxMem_zero_alloc( (void**)&_bufs[ i ], _buf_size ) ) goto End;
		xabuf.AudioBytes = _buf_size;
		xabuf.pAudioData = _bufs[ i ];
		_xa2_src->SubmitSourceBuffer( &xabuf );
	}
	if( _xa2_src->Start( 0, 0 ) != XA_OK ) goto End;

	// スレッド
	_thrd_active = TRUE;
	if( !(_hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)pThread, NULL, CREATE_SUSPENDED, &_thrd_id )) ) goto End;
	SetThreadPriority( _hThread, THREAD_PRIORITY_HIGHEST );
	ResumeThread     ( _hThread );

	b_ret = TRUE;
End:
	if( !b_ret ) pxwXAudio2_Release();

	return b_ret;
}

DWORD pxwXAudio2_GetState    ( void          ){ XAUDIO2_VOICE_STATE state; _xa2_src->GetState( &state, 0 ); return state.BuffersQueued; }
BOOL  pxwXAudio2_IsActive    ( void          ){ return _thrd_active; }
BOOL  pxwXAudio2_8bitAllocate( BYTE **p_buf8 ){ return pxMem_zero_alloc( (void**)p_buf8, _buf_size/2 ); }
void  pxwXAudio2_GetBufs     ( BYTE **p_data, DWORD *p_size, DWORD idx ){ *p_data = _bufs[ idx ]; *p_size = _buf_size; }
BOOL  pxwXAudio2_SubmitAudio ( BYTE  *p_data, DWORD  p_size )
{
	XAUDIO2_BUFFER xabuf = {};
	xabuf.AudioBytes = p_size;
	xabuf.pAudioData = p_data;

	return ( _xa2_src->SubmitSourceBuffer( &xabuf ) == XA_OK );
}
