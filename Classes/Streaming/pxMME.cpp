#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "pxMME.h"
#include <mmeapi.h>
#pragma comment( lib, "winmm" )

static HWAVEOUT _p_hWaveOut             =  NULL ; // WAVEMAPPERオブジェクト
static WAVEHDR  _p_waveHdr[WAVEHDR_NUM] = {NULL};
static BYTE    *_p_buffers[WAVEHDR_NUM] = { 0 } ; // 2つのバッファ
static HANDLE   _hThread                =  NULL ;
static DWORD    _thrd_id                =  NULL ;

typedef struct MME_STREAMING_THREAD
{
    long bps;
    long sps;
    long channels;
    long smp_buf;
};
MME_STREAMING_THREAD _cfg;


////////////////////////////
// グローバル関数 //////////
////////////////////////////

// WAVEMAPPER 初期化
BOOL pxMME_Initialize( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread )
{
    BOOL bReturn = FALSE;
    WAVEFORMATEX fmt;

    memset( &_cfg, 0, sizeof(_cfg) );
    _cfg.bps      = bps;
    _cfg.sps      = sps;
    _cfg.channels = ch_num;
    _cfg.smp_buf  = smp_buf;
    long buf_size = smp_buf * ch_num * ((bps == 8) ? 1 : 2);

    memset( &fmt, 0, sizeof(WAVEFORMATEX) );
    fmt.cbSize          = 0                ;
    fmt.wFormatTag      = WAVE_FORMAT_PCM  ;
    fmt.nChannels       = (WORD)ch_num     ;
    fmt.nSamplesPerSec  = sps              ;
    fmt.wBitsPerSample  = (WORD)bps        ;
    fmt.nBlockAlign		= fmt.nChannels   * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec	= fmt.nBlockAlign * fmt.nSamplesPerSec;

    if( waveOutOpen( NULL, WAVE_MAPPER, &fmt, 0, 0, WAVE_FORMAT_QUERY ) != MMSYSERR_NOERROR ) goto End;
    memset( &_p_waveHdr, 0, sizeof(WAVEHDR) * WAVEHDR_NUM );

    for( long i = 0; i < WAVEHDR_NUM; i++ )
    {
        if( !(  _p_buffers[ i ] = (unsigned char*)malloc( buf_size )  ) ) goto End;
        memset( _p_buffers[ i ], ( bps == 8 ) ? 0x80 : 0, buf_size );

        _p_waveHdr[ i ].dwBufferLength = buf_size;
        _p_waveHdr[ i ].lpData = (char*)_p_buffers[ i ];
        _p_waveHdr[ i ].dwUser = i;
    }

    // スレッド
    if( !(_hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)pThread, NULL, CREATE_SUSPENDED, &_thrd_id )) ) goto End;
    SetThreadPriority( _hThread, THREAD_PRIORITY_HIGHEST );
    ResumeThread     ( _hThread );

    if( waveOutOpen( &_p_hWaveOut, WAVE_MAPPER, &fmt, _thrd_id, 0, CALLBACK_THREAD ) != MMSYSERR_NOERROR ) goto End;
    
    for( long i = 0; i < 2; i++ )
    {
        if( waveOutPrepareHeader( _p_hWaveOut, &_p_waveHdr[ i ], sizeof(WAVEHDR) ) != MMSYSERR_NOERROR ) goto End;
        if( waveOutWrite        ( _p_hWaveOut, &_p_waveHdr[ i ], sizeof(WAVEHDR) ) != MMSYSERR_NOERROR ) goto End;
    }

    bReturn = TRUE;
End:
    if( !bReturn )
    {
        for( long i = 0; i < WAVEHDR_NUM; i++ ) pxMem_free( (void**)&_p_buffers[ i ] );
    }

    return bReturn;
}

// WAVEMAPPER 開放
BOOL pxMME_Release( void )
{
    if( !_p_hWaveOut ) return TRUE;

    if( waveOutReset( _p_hWaveOut ) != MMSYSERR_NOERROR ) return FALSE;
    if( waveOutClose( _p_hWaveOut ) != MMSYSERR_NOERROR ) return FALSE;

    if( _hThread ){ WaitForMultipleObjects( 1, &_hThread, TRUE, 10000 ); CloseHandle( _hThread ); }
    _hThread    = NULL;
    _p_hWaveOut = NULL;
    _thrd_id    = 0;

    for( long i = 0; i < WAVEHDR_NUM; i++ ) pxMem_free( (void**)&_p_buffers[ i ] );

    return TRUE;
}

BOOL pxMME_Header_Clean( WAVEHDR *p_waveHdr )
{
    return ( waveOutUnprepareHeader( _p_hWaveOut, p_waveHdr, sizeof(WAVEHDR) ) == MMSYSERR_NOERROR );
}
BOOL pxMME_Header_Play( WAVEHDR *p_waveHdr )
{
    if( waveOutPrepareHeader( _p_hWaveOut, p_waveHdr, sizeof(WAVEHDR) ) != MMSYSERR_NOERROR ) return FALSE;
    if( waveOutWrite        ( _p_hWaveOut, p_waveHdr, sizeof(WAVEHDR) ) != MMSYSERR_NOERROR ) return FALSE;

    return TRUE;
}

