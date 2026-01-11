#include <StdAfx.h>


#include "../Fixture/DebugLog.h"
#include "DxSound.h"
#include "DirectX5/dsound.h"
#include "Streaming.h"

static IDirectSound       *_p_DirectSound  = NULL; // DirectSoundオブジェクト
static IDirectSoundBuffer *_p_DS_Primary   = NULL; // 一次バッファ
static IDirectSoundBuffer *_p_DS_Secondary = NULL; // 二次バッファ
static IDirectSoundNotify *_p_DS_Notify    = NULL;
static DWORD               _p_DS_offset;
static DSBPOSITIONNOTIFY   _dspn       [DS_BUFFER_COUNT];
static HANDLE              _eventNotify[DS_BUFFER_COUNT];
DS_STREAMING_THREAD hThread;


////////////////////////////
// グローバル関数 //////////
////////////////////////////

void *DxSound_GetDirectSoundPointer( void )
{
	return _p_DirectSound;
}

// DirectSound 開放
BOOL DxSound_Release( void )
{
	BOOL b_ret = FALSE;

	dlog( "streming ds release(1)" );
	if( !_p_DirectSound ) return TRUE;
	dlog( "streming ds release(2)" );

	if( _p_DS_Secondary )
	{
		dlog( "streming ds release(3)" );
		_p_DS_Secondary->Stop();
		dlog( "streming ds release(4)" );
	}

	dlog( "streming ds release(5)" );
	if( !WaitForMultipleObjects( 1, &hThread.handle, TRUE, INFINITE ) ) goto End;

	dlog( "streming ds release(6)" );
	CloseHandle( hThread.handle );
	dlog( "streming ds release(7)" );

	if( _p_DS_Notify )
	{
		dlog( "streming ds release(8)"  );
		_p_DS_Notify   ->Release();
		dlog( "streming ds release(9)"  );
	}
	if( _p_DS_Secondary )
	{
		dlog( "streming ds release(10)" );
		_p_DS_Secondary->Release(); _p_DS_Secondary = NULL;
		dlog( "streming ds release(11)" );
	}
	if( _p_DS_Primary )
	{
		dlog( "streming ds release(12)" );
		_p_DS_Primary  ->Release(); _p_DS_Primary   = NULL;
		dlog( "streming ds release(13)" );
	}
	if( _p_DirectSound )
	{
		dlog( "streming ds release(14)" );
		_p_DirectSound ->Release(); _p_DirectSound  = NULL;
		dlog( "streming ds release(15)" );
	}

	b_ret = TRUE;
End:

	return b_ret;
}

// DirectSound 初期化
BOOL DxSound_Initialize( HWND hWnd, long ch_num, long sps, long bps, long smp_buf )
{
	BOOL              bReturn = FALSE;
	unsigned char    *p_data  = NULL ;
	WAVEFORMATEX      fmt;
	DSBUFFERDESC      dsbd;

	if( DirectSoundCreate( NULL, &_p_DirectSound, NULL ) != DS_OK ) goto End;
	if( _p_DirectSound->SetCooperativeLevel( hWnd, DSSCL_PRIORITY ) != DS_OK ) goto End;
	if( _p_DirectSound->SetSpeakerConfig( DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE) ) != DS_OK ) goto End;

	memset( &fmt, 0, sizeof(WAVEFORMATEX) );
	fmt.cbSize          = 0                ;
	fmt.wFormatTag      = WAVE_FORMAT_PCM  ;
	fmt.nChannels       = (WORD)ch_num     ;
	fmt.nSamplesPerSec  = sps              ;
	fmt.wBitsPerSample  = (WORD)bps        ;
	fmt.nBlockAlign     = fmt.nChannels   * fmt.wBitsPerSample / 8;
	fmt.nAvgBytesPerSec = fmt.nBlockAlign * sps;
	_p_DS_offset   = fmt.nBlockAlign * smp_buf;
	long data_size = _p_DS_offset * (DS_BUFFER_COUNT - 1);

	// 一次バッファの初期化
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
	if( _p_DirectSound->CreateSoundBuffer( &dsbd, &_p_DS_Primary, NULL ) != DS_OK ) goto End;
	if( _p_DS_Primary->SetFormat( &fmt ) ) goto End;

	// 二次バッファの初期化
	dsbd.dwBufferBytes = data_size;
	dsbd.lpwfxFormat   = &fmt;
	dsbd.dwFlags = DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLPOSITIONNOTIFY|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN|DSBCAPS_CTRLFREQUENCY|DSBCAPS_STATIC;
	if( _p_DirectSound->CreateSoundBuffer( &dsbd, &_p_DS_Secondary, NULL ) != DS_OK ) goto End;

	for( long i = 0; i < DS_BUFFER_COUNT; i++ )
	{
		if( !(_eventNotify[i] = CreateEvent( NULL, FALSE, FALSE, NULL )) ) goto End;
		_dspn[i].hEventNotify = _eventNotify[i];

		if( i == DS_BUFFER_COUNT - 1 ) _dspn[i].dwOffset = DSBPN_OFFSETSTOP;
		else                           _dspn[i].dwOffset = _p_DS_offset * i;
	}

	if( _p_DS_Secondary->QueryInterface( IID_IDirectSoundNotify, (void**)&_p_DS_Notify ) != DS_OK ) goto End;

	_p_DS_Notify->SetNotificationPositions( DS_BUFFER_COUNT, _dspn );
	_p_DS_Secondary->SetCurrentPosition( 0 );

	if( !(p_data = (unsigned char*)malloc( data_size )) ) goto End;
	memset( p_data, (bps == 8 ? 0x80 : 0), data_size );

	// 二次バッファのロック
	LPVOID lpbuf1, lpbuf2;
	DWORD  dwbuf1, dwbuf2;
	if( _p_DS_Secondary->Lock( 0, data_size, &lpbuf1, &dwbuf1, &lpbuf2, &dwbuf2, 0 ) != DS_OK ) goto End;

	// 音源データの設定
	CopyMemory( lpbuf1, p_data, dwbuf1 );
	if( dwbuf1 < data_size ) CopyMemory( lpbuf2, p_data + dwbuf1, dwbuf2 );
	// 二次バッファのロック解除
	if( _p_DS_Secondary->Unlock( lpbuf1, dwbuf1, lpbuf2, dwbuf2 ) != DS_OK ) goto End;

	// スレッド
	hThread.count  = DS_BUFFER_COUNT;
	hThread.events = _eventNotify;
	if( !(hThread.handle = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)DxSound_Proc, &hThread, CREATE_SUSPENDED, &hThread.id )) ) goto End;
	SetThreadPriority( hThread.handle, THREAD_PRIORITY_HIGHEST );
	ResumeThread     ( hThread.handle );

	_p_DS_Secondary->Play( 0, 0, DSBPLAY_LOOPING );

	bReturn = TRUE;
End:
	if( p_data   ){ free( p_data ); p_data = NULL; }
	if( !bReturn ) DxSound_Release();

	return bReturn;
}

BOOL DxSound_Lock  ( DWORD buf_index, LPVOID *lpbuf1, DWORD *dwbuf1, LPVOID *lpbuf2, DWORD *dwbuf2 )
{
	return ( _p_DS_Secondary->Lock  ( _p_DS_offset * buf_index, _p_DS_offset, lpbuf1, dwbuf1, lpbuf2, dwbuf2, 0 ) == DS_OK );
}
BOOL DxSound_Unlock(                  LPVOID  lpbuf1, DWORD  dwbuf1, LPVOID  lpbuf2, DWORD  dwbuf2 )
{
	return ( _p_DS_Secondary->Unlock(                                         lpbuf1, dwbuf1, lpbuf2, dwbuf2    ) == DS_OK );
}
