#include <StdAfx.h>


#include "pxWASAPI.h"
#include <mmdeviceapi.h>
#include <audioclient.h>

#define AUDCLNT_DEFAULT_BUFSIZE      10000000

static IAudioClient        *_p_AudioClient  = NULL;
static IAudioRenderClient  *_p_RenderClient = NULL;
static IMMDevice           *_p_Device       = NULL;
static IMMDeviceEnumerator *_p_Enum         = NULL;
static HANDLE               _event_handle   = NULL;
static HANDLE               _hThread        = NULL;
static DWORD                _thrd_id        = NULL;



////////////////////////////
// グローバル関数 //////////
////////////////////////////

BOOL pxWASAPI_Release( void )
{
	if( _event_handle ){ CloseHandle( _event_handle ); _event_handle = NULL; }
	if( _hThread ){ WaitForSingleObject( _hThread, INFINITE ); CloseHandle( _hThread ); _hThread = NULL; }
	_thrd_id = 0;

	if( _p_AudioClient  )  _p_AudioClient ->Stop();
	if( _p_RenderClient ){ _p_RenderClient->Release(); _p_RenderClient = NULL; }
	if( _p_AudioClient  ){ _p_AudioClient ->Release(); _p_AudioClient  = NULL; }
	if( _p_Device       ){ _p_Device      ->Release(); _p_Device       = NULL; }
	if( _p_Enum         ){ _p_Enum        ->Release(); _p_Enum         = NULL; }
	CoUninitialize();

	return TRUE;
}

BOOL pxWASAPI_Initialize( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread )
{
	BOOL b_ret = FALSE;

	if( FAILED(CoInitializeEx( NULL, COINIT_MULTITHREADED )) ) goto End;
	if( FAILED(CoCreateInstance( __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (LPVOID*)&_p_Enum )) ) goto End;

	if( FAILED(_p_Enum->GetDefaultAudioEndpoint( eRender, eMultimedia, &_p_Device )) ) goto End;
	if( FAILED(_p_Device->Activate( __uuidof(IAudioClient), CLSCTX_ALL, NULL, (LPVOID*)&_p_AudioClient )) ) goto End;

	WAVEFORMATEX fmt    = {};
	fmt.cbSize          = 0              ;
	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = ( WORD)ch_num  ;
	fmt.nSamplesPerSec  = (DWORD)sps     ;
	fmt.wBitsPerSample  = ( WORD)bps     ;
	fmt.nBlockAlign     = fmt.nChannels   * fmt.wBitsPerSample / 8;
	fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
	REFERENCE_TIME buf_size = (REFERENCE_TIME)( (float)smp_buf/sps * AUDCLNT_DEFAULT_BUFSIZE );
	DWORD flags         = AUDCLNT_STREAMFLAGS_EVENTCALLBACK|AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
	if( FAILED(_p_AudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED, flags, buf_size, 0, &fmt, NULL )) ) goto End;

	if( !(_event_handle = CreateEvent( 0, 0, 0, 0 )) ) goto End;
	if( FAILED(_p_AudioClient->SetEventHandle( _event_handle )) ) goto End;
	if( FAILED(_p_AudioClient->GetService( __uuidof(IAudioRenderClient), (LPVOID*)&_p_RenderClient  )) ) goto End;

	UINT32 frames; BYTE *pData = NULL;
	_p_AudioClient ->GetBufferSize( &frames );
	_p_RenderClient->GetBuffer    (  frames, &pData );
	memset( pData, (bps == 8 ? 0x80 : 0), frames * fmt.nBlockAlign );
	_p_RenderClient->ReleaseBuffer(  frames, 0 );
	_p_AudioClient ->Start();

	// スレッド
	if( !(_hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)pThread, NULL, CREATE_SUSPENDED, &_thrd_id )) ) goto End;
	SetThreadPriority( _hThread, THREAD_PRIORITY_HIGHEST );
	ResumeThread     ( _hThread );

	b_ret = TRUE;
End:
	if( !b_ret ) pxWASAPI_Release();

	return b_ret;
}

void   pxWASAPI_GetBufCount( UINT *p_bufCount              ){        _p_AudioClient->GetBufferSize( p_bufCount ); }
void   pxWASAPI_GetPadding ( UINT *pSize                   ){        _p_AudioClient->GetCurrentPadding( pSize  ); }
HANDLE pxWASAPI_GetHandle  ( void                          ){ return _event_handle; }
BOOL   pxWASAPI_Fill       ( UINT  buf_index, BYTE **lpBuf ){ return ( SUCCEEDED(_p_RenderClient->GetBuffer    ( buf_index, lpBuf )) ); }
BOOL   pxWASAPI_Unfill     ( UINT  buf_index               ){ return ( SUCCEEDED(_p_RenderClient->ReleaseBuffer( buf_index,     0 )) ); }
