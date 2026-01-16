#include <StdAfx.h>

#include "pxSound.h"

#ifdef pxINCLUDE_PT4i

#include "../Streaming/DirectX5/dsound.h"

static LPDIRECTSOUND       _p_DirectSound             =  NULL ; // DirectSoundオブジェクト
static LPDIRECTSOUNDBUFFER _p_secondarys[ MAX_VOICE ] = {NULL}; // 二時バッファ


////////////////////////////
// グローバル関数 //////////
////////////////////////////

bool DS_CreateBuffer( WAVEFORMATEX *p_format, int p_size, unsigned char *p_data, int no )
{
	// 二次バッファの初期化
	DSBUFFERDESC dsbd  = {0};
	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_GLOBALFOCUS|DSBCAPS_STATIC|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN|DSBCAPS_CTRLFREQUENCY;
	dsbd.dwBufferBytes = p_size;
	dsbd.lpwfxFormat   = p_format;
	if( _p_DirectSound->CreateSoundBuffer( &dsbd, &_p_secondarys[ no ], NULL ) != DS_OK ) return false;

	// 二次バッファのロック
	void         *lpbuf1, *lpbuf2;
	unsigned long dwbuf1,  dwbuf2;
	if( _p_secondarys[ no ]->Lock( 0, p_size, &lpbuf1, &dwbuf1, &lpbuf2, &dwbuf2, 0 ) != DS_OK ) return false;

	// 音源データの設定
	memcpy( lpbuf1, p_data, dwbuf1 );
	if( dwbuf2 ) memcpy( lpbuf2, p_data + dwbuf1, dwbuf2 );

	// 二次バッファのロック解除
	_p_secondarys[ no ]->Unlock( lpbuf1, dwbuf1, lpbuf2, dwbuf2 );

	return true;
}

// DirectSoundの開始 
bool DS_Initialize( HWND hWnd )
{
	if( DirectSoundCreate( NULL, &_p_DirectSound, NULL ) != DS_OK ){ _p_DirectSound = NULL; return false; }
	if( _p_DirectSound->SetCooperativeLevel( hWnd, DSSCL_PRIORITY ) != DS_OK ) return false;
	for( int i = 0; i < MAX_VOICE; i++ ) _p_secondarys[ i ] = NULL;

	return true;
}

// DirectSoundの終了 
void DS_Release( void )
{
	for( int i = 0; i < MAX_VOICE; i++ )
	{
		if( _p_secondarys[ i ] ){ _p_secondarys[ i ]->Release(); _p_secondarys[ i ] = NULL; }
	}
	if( _p_DirectSound ) _p_DirectSound->Release();
}

void DS_Voice_Freq  ( int no, float rate   ){ _p_secondarys[ no ]->SetFrequency( (long)(         rate *  44100 ) ); }
void DS_Voice_Volume( int no, float volume ){ _p_secondarys[ no ]->SetVolume   ( (long)( -1000 + 1000 * volume ) ); }
void DS_Voice_Stop  ( int no               ){ _p_secondarys[ no ]->Stop(); }
void DS_Voice_Play  ( int no, bool b_loop  ){ DS_Voice_Stop( no ); _p_secondarys[ no ]->SetCurrentPosition( 0 ); _p_secondarys[ no ]->Play( 0, 0, b_loop ); }

#endif
