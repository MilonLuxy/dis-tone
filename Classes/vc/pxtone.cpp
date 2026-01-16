#include <StdAfx.h>

#define PXTONEDLL_EXPORTS
#include "pxtone.h"
#include "../Fixture/CriticalSection.h"
#include "../Fixture/DataDevice.h"
#include "../Fixture/DebugLog.h"
#include "../pxtone/pxtnDelay.h"
#include "../pxtone/pxtnError.h"
#include "../pxtone/pxtnEvelist.h"
#include "../pxtone/pxtnGroup.h"
#include "../pxtone/pxtnMaster.h"
#include "../pxtone/pxtnOverDrive.h"
#include "../pxtone/pxtnPulse_Frequency.h"
#include "../pxtone/pxtnPulse_Noise.h"
#include "../pxtone/pxtnPulse_Oggv.h"
#include "../pxtone/pxtnService.h"
#include "../pxtone/pxtnText.h"
#include "../pxtone/pxtnUnit.h"
#include "../pxtone/pxtnWoice.h"
#include "../PT4i/PT4i.h"
#include "../Streaming/Streaming.h"

// Initializer values
#define _GROUP_NUM           7
#define _WOICE_NUM         100
#define _EVENT_NUM           0
#define _UNIT_NUM           50
#define _DELAY_NUM           4
#define _OD_NUM              2
#define _DEFAULT_COUNT      50

static bool _b_strm_active   = false;
static bool _b_pti           = false;
static int  _main_ch_num     =   0  ;
static int  _main_sps        =   0  ;
static int  _main_bps        =   0  ;
static char _error_msg[ 64 ] = { 0 }; // receives all error messages below

////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _SetError( const char *msg )
{
	memset( _error_msg,   0, sizeof(_error_msg)     );
	memcpy( _error_msg, msg, sizeof(_error_msg) - 1 );
}

static bool _Compile_Params( HWND hWnd, int channel_num, int sps, int bps, float buffer_sec, bool bDirectSound, PXTNPLAY_CALLBACK pProc )
{
	DWORD _strm_cfg[8];
	_strm_cfg[0] = (DWORD)hWnd;
	_strm_cfg[1] = channel_num;
	_strm_cfg[2] = sps;
	_strm_cfg[3] = bps;
	_strm_cfg[4] = (DWORD)(buffer_sec * sps);
	_strm_cfg[5] = bDirectSound;
	_strm_cfg[6] = (DWORD)pProc;

	if( !Streaming_Initialize( _strm_cfg, sizeof(DWORD) * 2 ) ){ _SetError( "error: initialize streaming" ); return false; }

	return true;
}

static bool _Tune_Read( DDV *p_read, const char *name )
{
#ifdef pxINCLUDE_PT4i
	if( name )
	{
		if( !pxtnService_CheckPTIFormat( &_b_pti, p_read, name ) ){ _SetError( pxtnError_Get() ); return false; }
		ddv_Seek( p_read, 0, SEEK_SET );

		if( _b_pti )
		{
			if( !PT4i_Load( p_read->fp ) ){ _SetError( PT4i_Get_Error() ); return false; }
			return true;
		}
	}
#endif

	int event_num = 0;
	if( !(event_num = pxtnService_Pre_Count_Event( p_read, _DEFAULT_COUNT )) ){ _SetError( "error: no event" ); return false; }

	ddv_Seek( p_read, 0, SEEK_SET );
	pxtnUnit_Release   ();
	pxtnEvelist_Release();

	if( !pxtnEvelist_Initialize ( event_num    ) ){ _SetError( "error: events"     ); return false; }
	if( !pxtnUnit_Initialize    ( _UNIT_NUM    ) ){ _SetError( "error: units"      ); return false; }
	if( !pxtnService_Read       ( p_read, true ) ){ _SetError( pxtnError_Get()     ); return false; }
	if( !pxtnWoice_ReadyTone    (              ) ){ _SetError( "error: woice-work" ); return false; }
	if( !pxtnDelay_ReadyTone    (              ) ){ _SetError( "error: delay-work" ); return false; }
		 pxtnOverDrive_ReadyTone(              );

	return true;
}

static bool _Read_FromMemory( void *p, int size, DDV *p_read )
{
	memset( p_read, 0, sizeof(DDV) );
	p_read->p    = (unsigned char *)p;
	p_read->size = size;
	return true;
}

////////////////////////////
// グローバル関数 //////////
////////////////////////////

bool DLLAPI pxtn_Ready( HWND hWnd, int channel_num, int sps, int bps, float buffer_sec, bool bDirectSound, PXTNPLAY_CALLBACK pProc )
{
	bool b_ret     = false;
	_b_strm_active = false;

	CriticalSection_Initialize();
	pxtnService_Clear();

		 pxtnGroup_Set           ( _GROUP_NUM );
	if( !pxtnWoice_Initialize    ( _WOICE_NUM ) ){ _SetError( "error: woices"    ); goto End; }
	if( !pxtnEvelist_Initialize  ( _EVENT_NUM ) ){ _SetError( "error: event"     ); goto End; }
	if( !pxtnUnit_Initialize     ( _UNIT_NUM  ) ){ _SetError( "error: units"     ); goto End; }
	if( !pxtnDelay_Initialize    ( _DELAY_NUM ) ){ _SetError( "error: delay"     ); goto End; }
	if( !pxtnOverDrive_Initialize( _OD_NUM    ) ){ _SetError( "error: overdrive" ); goto End; }
	
//	Present in full pxtone DLL versions only.
#ifdef pxINCLUDE_OGGVORBIS
	pxtnOggv_Initialize( pxtn_free, pxtn_load, pxtn_decode, pxtn_size, pxtn_write, pxtn_read );
#endif

	pxtnFrequency_Initialize();
	Streaming_Set_SampleInfo( channel_num, sps, bps );

	if( buffer_sec > 0 )
	{
		if( !_Compile_Params( hWnd, channel_num, sps, bps, buffer_sec, bDirectSound, pProc ) ) return false;
		_b_strm_active = true;
	}
	else _b_strm_active = false;

	_main_ch_num = channel_num;
	_main_sps    = sps;
	_main_bps    = bps;

	b_ret = true;
End:

	return b_ret;
}

bool DLLAPI pxtn_Reset( HWND hWnd, int channel_num, int sps, int bps, float buffer_sec, bool bDirectSound, PXTNPLAY_CALLBACK pProc )
{
	bool b_ret = false;

	Streaming_Set_SampleInfo( channel_num, sps, bps );

	if( !pxtnWoice_ReadyTone() ){ _SetError( "error: woice-work" ); goto End; }
	if( !pxtnDelay_ReadyTone() ){ _SetError( "error: delay-work" ); goto End; }
	pxtnOverDrive_ReadyTone ();

	if( _b_strm_active && !Streaming_Release() ){ _SetError( "error: release streaming" ); goto End; }

	if( buffer_sec > 0 )
	{
		if( !_Compile_Params( hWnd, channel_num, sps, bps, buffer_sec, bDirectSound, pProc ) ) return false;
		_b_strm_active = true;
	}
	else _b_strm_active = false;

	_main_ch_num = channel_num;
	_main_sps    = sps;
	_main_bps    = bps;

	b_ret = true;
End:

	return b_ret;
}

void DLLAPI *pxtn_GetDirectSound( void )
{
	return Streaming_GetDirectSound();
}

const char DLLAPI *pxtn_GetLastError( void )
{
	return _error_msg;
}

void DLLAPI pxtn_GetQuality( int *p_channel_num, int *p_sps, int *p_bps, int *p_sample_per_buf )
{
	if( _b_strm_active )
	{
		Streaming_GetQuality( (s32*)p_channel_num, (s32*)p_sps, (s32*)p_bps, (s32*)p_sample_per_buf );
	}
	else
	{
		if( p_channel_num    ) *p_channel_num    = _main_ch_num;
		if( p_sps            ) *p_sps            = _main_sps   ;
		if( p_bps            ) *p_bps            = _main_bps   ;
		if( p_sample_per_buf ) *p_sample_per_buf = 0           ;
	}
}

bool DLLAPI pxtn_Release( void )
{
	pxtnEvelist_Release  ();
	pxtnUnit_Release     ();
	pxtnDelay_Release    ();
	pxtnOverDrive_Release();
	if( _b_strm_active )
	{
		if( !Streaming_Release() ) return false;
		_b_strm_active = false;
	}
#ifdef pxINCLUDE_PT4i
	PT4i_Release();
#endif
	CriticalSection_Release();
	return true;
}

bool DLLAPI pxtn_Tune_Load( HMODULE hModule, const char *type_name, const char *file_name )
{
	bool b_ret = false;

	DDV read;
	if( !ddv_Open( hModule, file_name, type_name, &read ) ){ _SetError( "error: can\'t open" ); goto End; }
	if( !_Tune_Read( &read, file_name ) ) goto End;
	
	b_ret = true;
End:
	ddv_Close( &read );

	return b_ret;
}

bool DLLAPI pxtn_Tune_Read( void *p, int size )
{
	bool b_ret = false;

	DDV read;
	if( !_Read_FromMemory( p, size, &read ) ){ _SetError( "error: can\'t open" ); goto End; }
	if( !_Tune_Read( &read, NULL ) ) goto End;

	b_ret = true;
End:
	ddv_Close( &read );

	return b_ret;
}

bool DLLAPI pxtn_Tune_Release( void )
{
	pxtnUnit_Release   ();
	pxtnEvelist_Release();
	return true;
}

bool DLLAPI pxtn_Tune_Start( int start_sample, int fadein_msec, float volume )
{
	if( _b_strm_active ) Streaming_Tune_Stop();

	pxtnVOMITPREPARATION prep;
	memset( &prep, 0, sizeof(pxtnVOMITPREPARATION) );

	prep.start_pos_meas   = 0;
	prep.start_pos_sample = start_sample;
	prep.meas_end         = pxtnMaster_Get_PlayMeas();
	prep.meas_repeat      = pxtnMaster_Get_RepeatMeas();
	prep.fadein_sec       = fadein_msec;
	prep.master_volume    = volume;

	if( _b_strm_active )
	{
		if( !Streaming_Tune_Start( &prep, _b_pti ) ){ _SetError( "error: start tune" ); return false; }
	}
	else
	{
		if( !pxtnServiceMoo_Preparation( &prep   ) ){ _SetError( "error: start tune" ); return false; }
	}

	return true;
}

int DLLAPI pxtn_Tune_Fadeout( int msec )
{
	if( _b_strm_active ) Streaming_Tune_Fadeout(     msec );
	else                 pxtnServiceMoo_SetFade( -1, msec );
#ifdef pxINCLUDE_PT4i
	if( _b_pti ) return PT4i_Get_NowEve();
#endif
	return pxtnServiceMoo_Get_SamplingOffset();
}

void DLLAPI pxtn_Tune_SetVolume( float v )
{
#ifdef pxINCLUDE_PT4i
	if( _b_pti ) PT4i_SetVolume( v );
#endif
	pxtnServiceMoo_Set_Master_Volume( v );
}

int DLLAPI pxtn_Tune_Stop( void )
{
	if( _b_strm_active ) Streaming_Tune_Stop();

#ifdef pxINCLUDE_PT4i
	if( _b_pti ) return PT4i_Get_NowEve();
#endif
	return pxtnServiceMoo_Get_SamplingOffset();
}

bool DLLAPI pxtn_Tune_IsStreaming( void )
{
	if( _b_strm_active ) return Streaming_Is();
	return false;
}

void DLLAPI pxtn_Tune_SetLoop( bool bLoop )
{
	pxtnServiceMoo_SetLoop( bLoop );
}

void DLLAPI pxtn_Tune_GetInformation( int *p_beat_num, float *p_beat_tempo, int *p_beat_clock, int *p_meas_num )
{
#ifdef pxINCLUDE_PT4i
	if( _b_pti )
	{
		PT4i_Get_Information( p_beat_num, p_beat_tempo, p_beat_clock, p_meas_num );
		return;
	}
#endif

	pxtnMaster_Get( (s32*)p_beat_num, p_beat_tempo, (s32*)p_beat_clock );
	if( p_meas_num ) *p_meas_num = pxtnMaster_Get_PlayMeas();
}

int DLLAPI pxtn_Tune_GetRepeatMeas( void )
{
#ifdef pxINCLUDE_PT4i
	if( _b_pti ) return PT4i_Get_RepeatMeas();
#endif
	return pxtnMaster_Get_RepeatMeas();
}

int DLLAPI pxtn_Tune_GetPlayMeas( void )
{
#ifdef pxINCLUDE_PT4i
	if( _b_pti ) return PT4i_Get_PlayMeas();
#endif
	return pxtnMaster_Get_PlayMeas();
}

const char DLLAPI *pxtn_Tune_GetName( void )
{
	return pxtnText_Get_Name();
}

const char DLLAPI *pxtn_Tune_GetComment( void )
{
	return pxtnText_Get_Comment();
}

bool DLLAPI pxtn_Tune_Vomit( void *p, int sample_num )
{
	if( _b_strm_active || !p || !sample_num ) return false;
	return pxtnServiceMoo_Proc( p, sample_num * _main_ch_num * _main_bps/8 );
}

void DLLAPI pxtn_Noise_Initialize( void )
{
	pxtnFrequency_Initialize();
}

void DLLAPI pxtn_Noise_Release( PXTONENOISEBUFFER *p_noise )
{
	if( p_noise ){ free( p_noise ); p_noise = NULL; }
}

PXTONENOISEBUFFER DLLAPI *pxtn_Noise_Create( const char *name, const char *type, int channel_num, int sps, int bps )
{
	bool b_ret = false;
	PXTONENOISEBUFFER *p_noise = NULL;
	DDV                read  ;
	ptnDESIGN          design;
	
	if( !(p_noise = (PXTONENOISEBUFFER*)malloc( sizeof(PXTONENOISEBUFFER) )) ) return NULL;
	memset( p_noise, 0, sizeof(PXTONENOISEBUFFER) );
	memset( &read  , 0, sizeof(DDV              ) );
	memset( &design, 0, sizeof(ptnDESIGN        ) );

	if( !ddv_Open( NULL, name, type, &read ) ) goto End;
	if( !pxtnNoise_Design_Read   (   &read, &design, NULL ) ) goto End;
	if( !pxtnNoise_Build         ( &p_noise->p_buf, &design, channel_num, sps, bps ) ) goto End;
	p_noise->size = pxtnNoise_SamplingSize( &design, channel_num, sps, bps );

	b_ret = true;
End:
	ddv_Close( &read );
	pxtnNoise_Design_Release( &design );
	if( !b_ret && p_noise ){ free( p_noise ); p_noise = NULL; }

	return p_noise;
}

PXTONENOISEBUFFER DLLAPI *pxtn_Noise_Create_FromMemory( const char *p_designdata, int designdata_size, int channel_num, int sps, int bps )
{
	bool b_ret = false;
	PXTONENOISEBUFFER *p_noise = NULL;
	DDV                read  ;
	ptnDESIGN          design;
	
	if( !(p_noise = (PXTONENOISEBUFFER*)malloc( sizeof(PXTONENOISEBUFFER)) ) ) return NULL;
	memset( p_noise, 0, sizeof(PXTONENOISEBUFFER) );
	memset( &read  , 0, sizeof(DDV              ) );
	memset( &design, 0, sizeof(ptnDESIGN        ) );

	read.p = (unsigned char*)p_designdata; read.size = designdata_size;
	if( !pxtnNoise_Design_Read   (   &read, &design, NULL ) ) goto End;
	if( !pxtnNoise_Build         ( &p_noise->p_buf, &design, channel_num, sps, bps ) ) goto End;
	p_noise->size = pxtnNoise_SamplingSize( &design, channel_num, sps, bps );

	b_ret = true;
End:

	pxtnNoise_Design_Release( &design );
	if( !b_ret && p_noise ){ free( p_noise ); p_noise = NULL; }

	return p_noise;
}










//////////////////////////
// Redirect calls /////
//////////////////////////

//	bool              DLLAPI  pxtone_Ready                  ( HWND hWnd, int  channel_num, int  sps, int  bps, float buffer_sec, bool bDirectSound, PXTONEPLAY_CALLBACK pProc ){ return pxtn_Ready                   ( hWnd, channel_num, sps, bps, buffer_sec, bDirectSound, (PXTNPLAY_CALLBACK)pProc ); }
	BOOL              DLLAPI  pxtone_Ready                  ( HWND hWnd, long channel_num, long sps, long bps, float buffer_sec, BOOL bDirectSound, PXTONEPLAY_CALLBACK pProc ){ return pxtn_Ready                   ( hWnd, channel_num, sps, bps, buffer_sec, bDirectSound, (PXTNPLAY_CALLBACK)pProc ); }
//	bool              DLLAPI  pxtone_Reset                  ( HWND hWnd, int  channel_num, int  sps, int  bps, float buffer_sec, bool bDirectSound, PXTONEPLAY_CALLBACK pProc ){ return pxtn_Reset                   ( hWnd, channel_num, sps, bps, buffer_sec, bDirectSound, (PXTNPLAY_CALLBACK)pProc ); }
	BOOL              DLLAPI  pxtone_Reset                  ( HWND hWnd, long channel_num, long sps, long bps, float buffer_sec, BOOL bDirectSound, PXTONEPLAY_CALLBACK pProc ){ return pxtn_Reset                   ( hWnd, channel_num, sps, bps, buffer_sec, bDirectSound, (PXTNPLAY_CALLBACK)pProc ); }
	BOOL              DLLAPI  pxtone_ResetSampling          ( HWND hWnd, long channel_num, long sps, long bps, float buffer_sec, BOOL bDirectSound, PXTONEPLAY_CALLBACK pProc ){ return pxtn_Reset                   ( hWnd, channel_num, sps, bps, buffer_sec, bDirectSound, (PXTNPLAY_CALLBACK)pProc ); }
	void              DLLAPI *pxtone_GetDirectSound         ( void )                                                                                                           { return pxtn_GetDirectSound          ();                                                                                  }
	const char        DLLAPI *pxtone_GetLastError           ( void )                                                                                                           { return pxtn_GetLastError            ();                                                                                  }
	const char        DLLAPI *pxtone_Tune_GetLastError      ( void )                                                                                                           { return pxtn_GetLastError            ();                                                                                  }
//	void              DLLAPI  pxtone_GetQuality             ( int  *p_channel_num, int  *p_sps, int  *p_bps, int  *p_sample_per_buf )                                          {        pxtn_GetQuality              ( p_channel_num, p_sps, p_bps, p_sample_per_buf );                                   }
	void              DLLAPI  pxtone_GetQuality             ( long *p_channel_num, long *p_sps, long *p_bps, long *p_sample_per_buf )                                          {        pxtn_GetQuality              ( (int*)p_channel_num, (int*)p_sps, (int*)p_bps, (int*)p_sample_per_buf );           }
	BOOL              DLLAPI  pxtone_Release                ( void )                                                                                                           { return pxtn_Release                 ();                                                                                  }
	BOOL              DLLAPI  pxtone_Tune_Load              ( HMODULE hModule, const char *type_name, const char *file_name )                                                  { return pxtn_Tune_Load               ( hModule, type_name, file_name );                                                   }
//	bool              DLLAPI  pxtone_Tune_Read              ( void *p, int  size )                                                                                             { return pxtn_Tune_Read               ( p, size );                                                                         }
	BOOL              DLLAPI  pxtone_Tune_Read              ( void *p, long size )                                                                                             { return pxtn_Tune_Read               ( p, size );                                                                         }
	BOOL              DLLAPI  pxtone_Tune_Release           ( void )                                                                                                           { return pxtn_Tune_Release            ();                                                                                  }
//	BOOL              DLLAPI  pxtone_Tune_Start             ( long start_sample, long fadein_msec, float volume )                                                              { return pxtn_Tune_Start              ( start_sample, fadein_msec, volume );                                               }
	BOOL              DLLAPI  pxtone_Tune_Start             ( long start_sample, long fadein_msec               )                                                              { return pxtn_Tune_Start              ( start_sample, fadein_msec, 1.0f   );                                               }
	BOOL              DLLAPI  pxtone_Tune_Play              ( long start_sample, long fade_msec                 )                                                              { return pxtn_Tune_Start              ( start_sample, fade_msec  , 1.0f   );                                               }
//	int               DLLAPI  pxtone_Tune_Fadeout           ( int  msec )                                                                                                      { return pxtn_Tune_Fadeout            ( msec );                                                                            }
	long              DLLAPI  pxtone_Tune_Fadeout           ( long msec )                                                                                                      { return pxtn_Tune_Fadeout            ( msec );                                                                            }
	void              DLLAPI  pxtone_Tune_SetVolume         ( float  v )                                                                                                       {        pxtn_Tune_SetVolume          ( v );                                                                               }
//	void              DLLAPI  pxtone_Tune_SetVolume         ( double v )                                                                                                       {        pxtn_Tune_SetVolume          ( v );                                                                               }
	long              DLLAPI  pxtone_Tune_Stop              ( void )                                                                                                           { return pxtn_Tune_Stop               ();                                                                                  }
	BOOL              DLLAPI  pxtone_Tune_IsStreaming       ( void )                                                                                                           { return pxtn_Tune_IsStreaming        ();                                                                                  }
	BOOL              DLLAPI  pxtone_Tune_IsPlaying         ( void )                                                                                                           { return pxtn_Tune_IsStreaming        ();                                                                                  }
//	void              DLLAPI  pxtone_Tune_SetLoop           ( bool bLoop )                                                                                                     { return pxtn_Tune_SetLoop            ( bLoop );                                                                           }
	void              DLLAPI  pxtone_Tune_SetLoop           ( BOOL bLoop )                                                                                                     { return pxtn_Tune_SetLoop            ( bLoop );                                                                           }
//	void              DLLAPI  pxtone_Tune_GetInformation    ( int  *p_beat_num, float *p_beat_tempo, int  *p_beat_clock, int  *p_meas_num )                                    {        pxtn_Tune_GetInformation     (       p_beat_num, p_beat_tempo,       p_beat_clock,       p_meas_num );            }
	void              DLLAPI  pxtone_Tune_GetInformation    ( long *p_beat_num, float *p_beat_tempo, long *p_beat_clock, long *p_meas_num )                                    {        pxtn_Tune_GetInformation     ( (int*)p_beat_num, p_beat_tempo, (int*)p_beat_clock, (int*)p_meas_num );            }
	long              DLLAPI  pxtone_Tune_GetRepeatMeas     ( void )                                                                                                           { return pxtn_Tune_GetRepeatMeas      ();                                                                                  }
	long              DLLAPI  pxtone_Tune_GetPlayMeas       ( void )                                                                                                           { return pxtn_Tune_GetPlayMeas        ();                                                                                  }
	const char        DLLAPI *pxtone_Tune_GetName           ( void )                                                                                                           { return pxtn_Tune_GetName            ();                                                                                  }
	const char        DLLAPI *pxtone_Tune_GetComment        ( void )                                                                                                           { return pxtn_Tune_GetComment         ();                                                                                  }
//	bool              DLLAPI  pxtone_Tune_Vomit             ( void *p, int  sample_num )                                                                                       { return pxtn_Tune_Vomit              ( p, sample_num );                                                                   }
	BOOL              DLLAPI  pxtone_Tune_Vomit             ( void *p, long sample_num )                                                                                       { return pxtn_Tune_Vomit              ( p, sample_num );                                                                   }
	void              DLLAPI  pxtone_Noise_Initialize       ( void )                                                                                                           {        pxtn_Noise_Initialize        ();                                                                                  }
	void              DLLAPI  pxtone_Noise_Release          ( PXTONENOISEBUFFER *p_noise )                                                                                     {        pxtn_Noise_Release           ( p_noise );                                                                         }
//	PXTONENOISEBUFFER DLLAPI *pxtone_Noise_Create           ( const char *name        , const char *type   , int  channel_num, int  sps, int  bps )                            { return pxtn_Noise_Create            ( name        , type           , channel_num, sps, bps );                            }
	PXTONENOISEBUFFER DLLAPI *pxtone_Noise_Create           ( const char *name        , const char *type   , long channel_num, long sps, long bps )                            { return pxtn_Noise_Create            ( name        , type           , channel_num, sps, bps );                            }
//	PXTONENOISEBUFFER DLLAPI *pxtone_Noise_Create_FromMemory( const char *p_designdata, int designdata_size, int  channel_num, int  sps, int  bps )                            { return pxtn_Noise_Create_FromMemory ( p_designdata, designdata_size, channel_num, sps, bps );                            }
	PXTONENOISEBUFFER DLLAPI *pxtone_Noise_Create_FromMemory( const char *p_designdata, int designdata_size, long channel_num, long sps, long bps )                            { return pxtn_Noise_Create_FromMemory ( p_designdata, designdata_size, channel_num, sps, bps );                            }
