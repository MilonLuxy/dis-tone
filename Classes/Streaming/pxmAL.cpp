#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "OpenAL/al.h"
#include "OpenAL/alc.h"
#pragma comment( lib, "OpenAL32" )

static ALCdevice  *_p_dev     = NULL;
static ALCcontext *_p_cont    = NULL;
static UINT       *_p_buffers = NULL;
static UINT       *_p_sources = NULL;
static HANDLE      _hThread   = NULL;
static DWORD       _thrd_id   = NULL;
static volatile BOOL _thrd_active = FALSE;
static INT         _p_fmt;

#define AL_BUF_NUM      4


////////////////////////////
// グローバル関数 //////////
////////////////////////////

BOOL pxmAL_Release( void )
{
	_thrd_active = FALSE;
	if( _hThread ){ WaitForSingleObject( _hThread, INFINITE ); CloseHandle( _hThread ); _hThread = NULL; }
	_thrd_id = 0;

	if( _p_sources )
	{
		alSourceStop( _p_sources[ 0 ] );
		INT queued; alGetSourcei( _p_sources[ 0 ], AL_BUFFERS_QUEUED, &queued );
		while( queued-- > 0 ){ UINT buf; alSourceUnqueueBuffers( _p_sources[ 0 ], 1, &buf ); }
		alDeleteSources( 1, _p_sources );
		pxMem_free( (void**)&_p_sources );
	}
	if( _p_buffers ){ alDeleteBuffers( AL_BUF_NUM, _p_buffers ); pxMem_free( (void**)&_p_buffers ); }
	if( _p_cont    ){ alcMakeContextCurrent( NULL ); alcDestroyContext( _p_cont ); _p_cont = NULL; }
	if( _p_dev     ){ alcCloseDevice( _p_dev ); _p_dev = NULL; }

	return TRUE;
}

BOOL pxmAL_Initialize( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread )
{
	BOOL  b_ret = FALSE;
	void *dummy = NULL;

	if( !(_p_dev  = alcOpenDevice   (         NULL )) ) goto End;
	if( !(_p_cont = alcCreateContext( _p_dev, NULL )) ) goto End;
	if( alcMakeContextCurrent( _p_cont ) == ALC_FALSE ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_buffers, sizeof(UINT) * AL_BUF_NUM ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_sources, sizeof(UINT) *          1 ) ) goto End;
	alGenBuffers( AL_BUF_NUM, _p_buffers );
	alGenSources( 1, _p_sources );

	long buf_size = smp_buf * ch_num * ( bps == 8 ? 1 : 2 );
	if( bps == 8 ) _p_fmt = ( ch_num > 1 ? AL_FORMAT_STEREO8  : AL_FORMAT_MONO8  );
	else           _p_fmt = ( ch_num > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16 );
	if( !(dummy = malloc( buf_size )) ) goto End;
	memset( dummy, ( bps == 8 ? 0x80 : 0 ), buf_size );

	for( long i = 0; i < AL_BUF_NUM; i++ )
	{
		alBufferData( _p_buffers[ i ], _p_fmt, dummy, buf_size, sps );
		alSourceQueueBuffers( _p_sources[ 0 ], 1, &_p_buffers[ i ] );
		if( alGetError() != AL_NO_ERROR ) goto End;
	}
	alSourcePlay( _p_sources[ 0 ] );

	// スレッド
	_thrd_active = TRUE;
	if( !(_hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)pThread, NULL, CREATE_SUSPENDED, &_thrd_id )) ) goto End;
	SetThreadPriority( _hThread, THREAD_PRIORITY_HIGHEST );
	ResumeThread     ( _hThread );

	b_ret = TRUE;
End:
	pxMem_free( (void**)&dummy );
	if( !b_ret ) pxmAL_Release();

	return b_ret;
}

BOOL pxmAL_IsActive    ( void ){ return _thrd_active; }
INT  pxmAL_GetProcessed( void ){ int value; alGetSourcei( _p_sources[ 0 ], AL_BUFFERS_PROCESSED, &value ); return value; }

BOOL pxmAL_Unregist( UINT *buf )
{
	alSourceUnqueueBuffers( _p_sources[ 0 ], 1, buf );
	return ( alGetError() == AL_NO_ERROR );
}
BOOL pxmAL_Regist( UINT *buf, void *p_smp, long p_size )
{
	int sps; alGetBufferi( *buf, AL_FREQUENCY, &sps );
	alBufferData( *buf, _p_fmt, p_smp, p_size, sps );
	alSourceQueueBuffers( _p_sources[ 0 ], 1, buf );

	int state;
	alGetSourcei( _p_sources[ 0 ], AL_SOURCE_STATE, &state );
	if( state != AL_PLAYING ) alSourcePlay( _p_sources[ 0 ] );
	return ( alGetError() == AL_NO_ERROR );
}
