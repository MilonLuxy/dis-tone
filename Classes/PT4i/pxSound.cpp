#include <StdAfx.h>

#include "pxSound.h"

#ifdef pxINCLUDE_PT4i

#include "DxSound_PTI.h"
#include "pxMME_PTI.h"
#include "../pxtone/pxtnWoice.h"

// PCMロード
#define PTV_SMPSIZE 56 // adjusts sample size to tune voices correctly
#define PCMHEADSIZE 44 // RIFF + WAVE + fmt_ + size x 3 + WAVEFORMATEX(20)

static bool _bDirectSound = false;
static bool _b_init       = false;
static int  _ch_num, _sps, _bps;

////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static bool _Load_PCM( const char *file_name, WAVEFORMATEX *p_format, unsigned int *p_size, unsigned char **pp_data )
{
	bool b_ret = false;
	char buf[ PCMHEADSIZE ];
	DDV  read;

	*pp_data = NULL;
	if( !ddv_Open( NULL, file_name, "PTI", &read ) ) return false;
	if( !ddv_Read( buf , 1, PCMHEADSIZE  , &read ) ) goto End;

	// 'RIFFxxxxWAVEfmt '
	if( buf[ 0] != 'R' || buf[ 1] != 'I' || buf[ 2] != 'F' || buf[ 3] != 'F' ||
		buf[ 8] != 'W' || buf[ 9] != 'A' || buf[10] != 'V' || buf[11] != 'E' ||
		buf[12] != 'f' || buf[13] != 'm' || buf[14] != 't' || buf[15] != ' '  ) goto End;

	// フォーマットを読み込む
	memcpy( p_format, &buf[ 20 ], 18 );
	if( p_format->wFormatTag     != WAVE_FORMAT_PCM                     ) goto End;
	if( p_format->nChannels      != 1 && p_format->nChannels      !=  2 ) goto End;
	if( p_format->wBitsPerSample != 8 && p_format->wBitsPerSample != 16 ) goto End;
	ddv_Seek( &read, 12, SEEK_SET );


	// 'fmt ' 以降に 'data' を探す
	unsigned int size;
	while( ddv_Read( buf, 1, 8, &read ) )
	{
			 memcpy( &size , &buf[ 4 ], 4 );
		if( !memcmp( "data", &buf[ 0 ], 4 ) )
		{
			if( !(*pp_data = (unsigned char*)malloc( size )) ) goto End;
			memset( pp_data, 0, size );
			if( !ddv_Read( (void*)*pp_data, 1, size, &read ) ) goto End;
			*p_size = size;
			break;
		}
		ddv_Seek( &read, size, SEEK_CUR );
	}

	b_ret = true;
End:
	if( !b_ret && pp_data ){ free( pp_data ); pp_data = NULL; }
	ddv_Close( &read );

	return b_ret;
}

// サウンドの読み込み(リソースから)
bool _Voice_Load_PCM( const char *name, const char *type, int no )
{
	bool b_ret = false;

	unsigned char *p_data = NULL;
	unsigned int   p_size =  0  ;
	WAVEFORMATEX   fmt    = {0} ;

	if( !_Load_PCM( name, &fmt, &p_size, &p_data     ) ) goto End;
	if( _bDirectSound ){ if( !DS_CreateBuffer( &fmt, p_size, p_data, no    ) ) goto End; }
	else               { if( !MME_LoadBuffer(  &fmt, p_size, p_data, false ) ) goto End; }

	b_ret = true;
End:
	if( p_data ){ free( p_data ); p_data = NULL; }

	return b_ret;
}

bool _Voice_Load_PTV( const char *name, const char *type, int no )
{
	bool b_ret = false;

	WOICE_STRUCT *p_w  = NULL;
	DDV           read = {0} ;

#ifdef DLL_EXPORT
	extern HINSTANCE g_h_inst_dll;
	if( !ddv_Open( g_h_inst_dll, name, type, &read ) ) return false;
#else
	if( !ddv_Open( NULL, name, type, &read ) ) return false;
#endif

	if( !(p_w = pxtnWoice_Create()) ) goto End;
	if( !pxtnWoicePTV_Read( &read, p_w, NULL ) ) goto End;
	pxtnWoice_BuildPTV( p_w, PTV_SMPSIZE, _ch_num, _sps, _bps );

	WAVEFORMATEX fmt    = {0};
	fmt.cbSize          =              0 ;
	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = _ch_num        ;
	fmt.nSamplesPerSec  = _sps           ;
	fmt.wBitsPerSample  = _bps           ;
	fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample/8;
	fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;

	if( _bDirectSound ){ if( !DS_CreateBuffer( &fmt, p_w->_voinsts[ 0 ].smp_body_w * fmt.nBlockAlign, p_w->_voinsts[ 0 ].p_smp_w, no    ) ) goto End; }
	else               { if( !MME_LoadBuffer(  &fmt, p_w->_voinsts[ 0 ].smp_body_w * fmt.nBlockAlign, p_w->_voinsts[ 0 ].p_smp_w, false ) ) goto End; }

	b_ret = true;
End:
	ddv_Close( &read );
	pxtnWoice_Remove( 0 );

	return b_ret;
}

bool _Voice_Load_PTN( const char *name, const char *type, int no )
{
	bool b_ret = false;

	unsigned char *p_data = NULL;
	unsigned int   p_size =  0  ;
	DDV            read   = {0} ;
	ptnDESIGN      design = {0} ;

#ifdef DLL_EXPORT
	extern HINSTANCE g_h_inst_dll;
	if( !ddv_Open( g_h_inst_dll, name, type, &read ) ) return false;
#else
	if( !ddv_Open( NULL, name, type, &read ) ) return false;
#endif

	if( !pxtnNoise_Design_Read( &read  , &design, NULL ) ) goto End;
	if( !pxtnNoise_Build      ( &p_data, &design, _ch_num, _sps, _bps ) ) goto End;
	p_size = pxtnNoise_SamplingSize(     &design, _ch_num, _sps, _bps );

	WAVEFORMATEX fmt    = {0};
	fmt.cbSize          =              0 ;
	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = _ch_num        ;
	fmt.nSamplesPerSec  = _sps           ;
	fmt.wBitsPerSample  = _bps           ;
	fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample/8;
	fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;

	if( _bDirectSound ){ if( !DS_CreateBuffer( &fmt, p_size, p_data, no   ) ) goto End; }
	else               { if( !MME_LoadBuffer(  &fmt, p_size, p_data, true ) ) goto End; }

	b_ret = true;
End:
	ddv_Close( &read );
	if( p_data ){ free( p_data ); p_data = NULL; }

	return b_ret;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

bool pxSound_Initialize( HWND hWnd, int ch_num, int sps, int bps, bool b_dxsound )
{
	if( b_dxsound ){ if( !DS_Initialize ( hWnd ) ) return false; }
	else           { if( !MME_Initialize( hWnd, ch_num, sps, bps ) ) return false; }
	
	_ch_num       = ch_num;
	_sps          = sps;
	_bps          = bps;
	_bDirectSound = b_dxsound;
	_b_init = true;

	const char *type = "PTI";
	if( !_Voice_Load_PTV( "PTV_TRIA784", type, 0 ) ) return false;
	if( !_Voice_Load_PTV( "PTV_TRIA784", type, 1 ) ) return false;
	if( !_Voice_Load_PTV( "PTV_SAW784" , type, 2 ) ) return false;
	if( !_Voice_Load_PTV( "PTV_RECT784", type, 3 ) ) return false;
	if( !_Voice_Load_PTN( "PTN_HATC"   , type, 4 ) ) return false;
	if( !_Voice_Load_PTN( "PTN_BD6"    , type, 5 ) ) return false;

	return true;
}
bool pxSound_IsActive( void ){ return _b_init; }

void pxSound_Release  ( void                 ){ if( _bDirectSound ) DS_Release     (             ); else MME_Release();  _b_init = false; }
void pxSound_Play     ( int idx, bool b_loop ){ if( _bDirectSound ) DS_Voice_Play  ( idx, b_loop ); else MME_Voice_Play  ( idx, b_loop ); }
void pxSound_Stop     ( int idx              ){ if( _bDirectSound ) DS_Voice_Stop  ( idx         ); else MME_Voice_Stop  ( idx         ); }
void pxSound_SetVolume( int idx, float vol   ){ if( _bDirectSound ) DS_Voice_Volume( idx, vol    ); else MME_Voice_Volume( idx, vol    ); }
void pxSound_SetPitch ( int idx, float pitch ){ if( _bDirectSound ) DS_Voice_Freq  ( idx, pitch  ); else MME_Voice_Freq  ( idx, pitch  ); }

#endif
