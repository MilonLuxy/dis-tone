#include <StdAfx.h>

#include "pxSound.h"

#ifdef pxINCLUDE_PT4i

#include "../Fixture/pxMem.h"
#include "../Streaming/pxMME.h"

static CRITICAL_SECTION _cs;
static HWAVEOUT  _p_hWaveOut                    =  NULL ; // WAVEMAPPERオブジェクト
static WAVEHDR   _p_waveHdr     [ WAVEHDR_NUM ] = {NULL};
static BYTE     *_p_buffers     [ WAVEHDR_NUM ] = {NULL}; // 2つのバッファ
static HANDLE    _p_hThread                     =  NULL ;
static HANDLE    _p_bufferEvents[ WAVEHDR_NUM ] = {NULL};
static int       _smp_buf, _chs, _bps;
static bool      _b_init  = false;


// ----- internal structures -----
typedef struct SampleData
{
	BYTE *data;
	int   size;
	int   ch  ;
	int   sps ;
	int   bps ;
};
static SampleData *g_samples     = NULL;
static int         g_sampleCount = 0;
typedef struct Voice
{
	SampleData *sample;
	float       pos   ;
	float       freq  ;
	float       vol   ;
	float       pan   ;
	bool        loop  ;
	bool        active;
};
static Voice *g_voices = NULL;



////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static bool _CS_Lock  ( void ){ EnterCriticalSection( &_cs ); return _b_init; }
static void _CS_Unlock( void ){ LeaveCriticalSection( &_cs );                 }
static float _Panning_Left ( float pan ){ return ( pan <= 0.0f ? 1.0f : ( 1.0f - pan ) ); }
static float _Panning_Right( float pan ){ return ( pan >= 0.0f ? 1.0f : ( 1.0f + pan ) ); }

static void _Fill_Buffers( int idx )
{
	if( !_p_buffers[ idx ] ) return;
	const float MAX_16 = 0x7FFF;
	const float MAX_8  = 0x7F;

	int *mixL, *mixR;
	if( !pxMem_zero_alloc( (void**)&mixL, sizeof(int) * _smp_buf ) ) return;
	if( !pxMem_zero_alloc( (void**)&mixR, sizeof(int) * _smp_buf ) ) return;

	// for each active voice, mix into accumulators
	if( !_CS_Lock() ) return;
	for( int v = 0; v < MAX_VOICE; ++v )
	{
		Voice *voice = &g_voices[ v ];
		if( !voice->active || !voice->sample->data ) continue;

		SampleData *s = voice->sample;
		int srcFrames = ( s->bps == 16 ? s->size / (s->ch * 2) : s->size / s->ch );

		for( int f = 0; f < _smp_buf; f++ )
		{
			int   idx0 = (int)voice->pos;
			float frac = voice->pos - (float)idx0;

			if( idx0 >= srcFrames )
			{
				// wraps
				if( voice->loop )
				{
					idx0 = idx0 % srcFrames;
					voice->pos = (float)idx0;
				}
				// stops voice
				else
				{
					voice->active = false;
					break;
				}
			}

			// reads frame idx0 and idx1 for linear interpolation
			int idx1 = idx0 + 1;
			if( idx1 >= srcFrames )
			{
				if( voice->loop ) idx1 = idx1 % srcFrames;
				else              idx1 = srcFrames - 1;
			}

			int base0 = ( idx0 * s->ch ) * ( s->bps == 16 ? 2 : 1 );
			int base1 = ( idx1 * s->ch ) * ( s->bps == 16 ? 2 : 1 );
			float sL = 0.0f, sR = 0.0f;

			// 16-bit signed little-endian
			if( s->bps == 16 )
			{
				if( s->ch == 1 )
				{
					short a0  = *(short*)( s->data + base0         ); float va0  = a0  * ( 1.0f / MAX_16 );
					short a1  = *(short*)( s->data + base1         ); float va1  = a1  * ( 1.0f / MAX_16 );

					sL = sR = ( va0 + (va1 - va0) * frac );
				}
				else
				{
					short a0l = *(short*)( s->data + base0 + 0 * 2 ); float va0l = a0l * ( 1.0f / MAX_16 );
					short a0r = *(short*)( s->data + base0 + 1 * 2 ); float va0r = a0r * ( 1.0f / MAX_16 );
					short a1l = *(short*)( s->data + base1 + 0 * 2 ); float va1l = a1l * ( 1.0f / MAX_16 );
					short a1r = *(short*)( s->data + base1 + 1 * 2 ); float va1r = a1r * ( 1.0f / MAX_16 );

					sL = va0l + (va1l - va0l) * frac;
					sR = va0r + (va1r - va0r) * frac;
				}
			}
			// 8-bit unsigned PCM (0..255) => converts to -1..1
			else
			{
				if( s->ch == 1 )
				{
					BYTE a0  = *(BYTE*)( s->data + base0     ); float va0  = ( (int)a0  - MAX_8 ) / MAX_8;
					BYTE a1  = *(BYTE*)( s->data + base1     ); float va1  = ( (int)a1  - MAX_8 ) / MAX_8;

					sL = sR = ( va0 + (va1 - va0) * frac );
				}
				else
				{
					BYTE a0l = *(BYTE*)( s->data + base0 + 0 ); float va0l = ( (int)a0l - MAX_8 ) / MAX_8;
					BYTE a0r = *(BYTE*)( s->data + base0 + 1 ); float va0r = ( (int)a0r - MAX_8 ) / MAX_8;
					BYTE a1l = *(BYTE*)( s->data + base1 + 0 ); float va1l = ( (int)a1l - MAX_8 ) / MAX_8;
					BYTE a1r = *(BYTE*)( s->data + base1 + 1 ); float va1r = ( (int)a1r - MAX_8 ) / MAX_8;

					sL = va0l + (va1l - va0l) * frac;
					sR = va0r + (va1r - va0r) * frac;
				}
			}

			// applies volume & pan
			float leftScale  = voice->vol * _Panning_Left ( voice->pan );
			float rightScale = voice->vol * _Panning_Right( voice->pan );
			mixL[ f ] += (int)( (sL * leftScale ) * MAX_16);
			mixR[ f ] += (int)( (sR * rightScale) * MAX_16);

			voice->pos += voice->freq;
			if( (int)voice->pos >= srcFrames && !voice->loop ){ voice->active = false; break; }
		}
	}
	_CS_Unlock();

	// 16-bit
	if( _bps == 16 )
	{
		short *out = (short*)_p_buffers[ idx ];
		for( int f = 0; f < _smp_buf; ++f )
		{
			int vL = mixL[ f ]; pxMem_cap( (long*)&vL, (long)MAX_16, (long)-MAX_16 );
			int vR = mixR[ f ]; pxMem_cap( (long*)&vR, (long)MAX_16, (long)-MAX_16 );

			if( _chs == 2 )
			{
				out[ f * 2 + 0 ] = (short)vL;
				out[ f * 2 + 1 ] = (short)vR;
			}
			else out[ f ] = (short)( (vL + vR) / 2 );
		}
	}
	// 8-bit
	else
	{
		BYTE *out = _p_buffers[ idx ];
		for( int f = 0; f < _smp_buf; ++f )
		{
			int vL = mixL[ f ]; pxMem_cap( (long*)&vL, (long)MAX_16, (long)-MAX_16 );
			int vR = mixR[ f ]; pxMem_cap( (long*)&vR, (long)MAX_16, (long)-MAX_16 );

			if( _chs == 2 )
			{
				int tl = (int)( (vL  / MAX_16) * MAX_8 + (MAX_8 + 1) ); pxMem_cap( (long*)&tl, 0xFF, 0 );
				int tr = (int)( (vR  / MAX_16) * MAX_8 + (MAX_8 + 1) ); pxMem_cap( (long*)&tr, 0xFF, 0 );
				out[ f * 2 + 0 ] = (BYTE)tl;
				out[ f * 2 + 1 ] = (BYTE)tr;
			}
			else
			{
				int avg = (vL + vR) / 2;
				int t  = (int)( (avg / MAX_16) * MAX_8 + (MAX_8 + 1) ); pxMem_cap( (long*)&t , 0xFF, 0 );
				out[ f ] = (BYTE)t;
			}
		}
	}

	pxMem_free( (void**)&mixL );
	pxMem_free( (void**)&mixR );
	_p_waveHdr[ idx ].dwFlags &= ~WHDR_DONE;
}

// ---------------- internal mixer & thread ----------------
static void CALLBACK waveOutProcInternal( HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
	if( uMsg == WOM_DONE )
	{
		WAVEHDR *hdr = (WAVEHDR*)(dwParam1);
		int idx = hdr->dwUser;
		if( _p_bufferEvents[ idx ] ) SetEvent( _p_bufferEvents[ idx ] );
	}
}

static DWORD WINAPI MixerThreadProc( LPVOID lpParam )
{
	for( int i = 0; i < WAVEHDR_NUM; i++ )
	{
		_Fill_Buffers( i );
		waveOutWrite( _p_hWaveOut, &_p_waveHdr[ i ], sizeof(WAVEHDR) );
	}

	int cur = 0;
	while( _b_init )
	{
		HANDLE ev = _p_bufferEvents[ cur ];
		if( !_b_init || !ev ) break;
		if( WaitForSingleObject( ev, 1000 ) != WAIT_TIMEOUT )
		{
			_Fill_Buffers( cur );
			if( waveOutWrite( _p_hWaveOut, &_p_waveHdr[ cur ], sizeof(WAVEHDR) ) != MMSYSERR_NOERROR )
			{
				waveOutReset( _p_hWaveOut );
				for( int i = 0; i < WAVEHDR_NUM; i++ ){ waveOutWrite( _p_hWaveOut, &_p_waveHdr[ i ], sizeof(WAVEHDR) ); }
			}

			cur = (cur + 1) % WAVEHDR_NUM;
		}
	}

	return 0;
}

static void _SetFrequency      ( DWORD no, FLOAT rate   ){ if( g_voices[ no ].sample ) g_voices[ no ].freq = rate / (float)g_voices[ no ].sample->sps; }
static void _SetCurrentPosition( DWORD no, FLOAT pos    ){     g_voices[ no ].pos    = pos  ; }
static void _SetVolume         ( DWORD no, FLOAT vol    ){     g_voices[ no ].vol    = vol  ; }
static void _Stop              ( DWORD no               ){     g_voices[ no ].active = false; }
static void _Play              ( DWORD no, BOOL  b_loop ){     g_voices[ no ].active = true ; g_voices[ no ].sample = &g_samples[ no ]; g_voices[ no ].loop = b_loop; }



////////////////////////////
// グローバル関数 //////////
////////////////////////////

bool MME_LoadBuffer( WAVEFORMATEX *p_format, int p_size, unsigned char *p_data, bool b_reset_freq )
{
	if( !p_data || p_size <= 0 ) return false;

	SampleData *s = NULL;
	if( !(s = (SampleData*)realloc( g_samples, sizeof(SampleData) * (g_sampleCount + 1) )) ) return false;
	g_samples = s;

	SampleData *dst = &g_samples[ g_sampleCount ];
	if( !pxMem_zero_alloc( (void**)&dst->data, p_size ) ) return false;
	memcpy( dst->data, p_data, p_size );

	dst->size = p_size;
	dst->ch   = p_format->nChannels;
	dst->sps  = p_format->nSamplesPerSec;
	dst->bps  = p_format->wBitsPerSample;
	if( b_reset_freq ) g_voices[ g_sampleCount ].freq = 1;

	g_sampleCount++;
	return true;
}

// WAVEMAPPERの終了
void MME_Release( void )
{
	if( !_p_hWaveOut ) return;

	_b_init = false;
	if( _p_hThread ){ WaitForSingleObject( _p_hThread, 10000 ); CloseHandle( _p_hThread ); }
	_p_hThread  = NULL;

	if( waveOutReset( _p_hWaveOut ) != MMSYSERR_NOERROR ) return;
	if( waveOutClose( _p_hWaveOut ) != MMSYSERR_NOERROR ) return;
	_p_hWaveOut = NULL;

	for( long i = 0; i < WAVEHDR_NUM; i++ ) pxMem_free( (void**)&_p_buffers[ i ] );

	pxMem_free( (void**)&g_voices );
	if( g_samples )
	{
		for( int i = 0; i < g_sampleCount; i++ ) pxMem_free( (void**)&g_samples[ i ].data );
		free( g_samples ); g_samples = NULL;
		g_sampleCount = 0;
	}
	DeleteCriticalSection( &_cs );
}

// WAVEMAPPERの開始
bool MME_Initialize( HWND hWnd, int ch_num, int sps, int bps )
{
	bool b_ret = false;

	_smp_buf = sps / 10;
	_chs     = ch_num;
	_bps     = bps;
	int p_smp_buf = _smp_buf * ch_num * _bps / 8;
	InitializeCriticalSection( &_cs );
	if( !pxMem_zero_alloc( (void**)&g_voices, sizeof(Voice) * MAX_VOICE ) ) return false;
	for( long i = 0; i < WAVEHDR_NUM; i++ ){ if( !(_p_bufferEvents[ i ] = CreateEvent( NULL, FALSE, FALSE, NULL )) ) goto End; }

	WAVEFORMATEX fmt    = {0};
	fmt.cbSize          =  0             ;
	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = ch_num         ;
	fmt.nSamplesPerSec  = sps            ;
	fmt.wBitsPerSample  = bps            ;
	fmt.nBlockAlign     = fmt.nChannels   * fmt.wBitsPerSample / 8;
	fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;

	if( waveOutOpen( &_p_hWaveOut, WAVE_MAPPER, &fmt, (DWORD_PTR)waveOutProcInternal, 0, CALLBACK_FUNCTION ) != MMSYSERR_NOERROR ) goto End;
	memset( &_p_waveHdr, 0, sizeof(WAVEHDR) * WAVEHDR_NUM );

	for( long i = 0; i < WAVEHDR_NUM; i++ )
	{
		if(  !( _p_buffers[ i ] = (unsigned char*)malloc(   p_smp_buf ) ) ) goto End;
		memset( _p_buffers[ i ],  ( _bps == 8 ) ? 0x80 : 0, p_smp_buf );
		
		_p_waveHdr[ i ].dwBufferLength = p_smp_buf;
		_p_waveHdr[ i ].lpData         = (char*)_p_buffers[ i ];
		_p_waveHdr[ i ].dwUser         = i;
		waveOutPrepareHeader( _p_hWaveOut, &_p_waveHdr[ i ], sizeof(WAVEHDR) );
	}

	// スレッド
	_b_init = true;
	if( !(_p_hThread = CreateThread( NULL, 0, MixerThreadProc, NULL, 0, NULL )) ) goto End;
	SetThreadPriority( _p_hThread, THREAD_PRIORITY_HIGHEST );
	ResumeThread     ( _p_hThread );

	b_ret    = true;
End:
	if( !b_ret ) MME_Release();

	return b_ret;
}

void MME_Voice_Freq  ( int no, float rate   ){ _SetFrequency ( no, (long)( rate * 44100 ) ); }
void MME_Voice_Volume( int no, float volume ){ _SetVolume    ( no, volume ); }
void MME_Voice_Stop  ( int no               ){ _Stop         ( no         ); }
void MME_Voice_Play  ( int no, bool  b_loop ){ MME_Voice_Stop( no ); _SetCurrentPosition( no, 0 ); _Play( no, b_loop ); }

#endif
