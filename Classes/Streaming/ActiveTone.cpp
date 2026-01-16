#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "../pxtone/pxtnEvelist.h"
#include "../pxtone/pxtnMaster.h"
#include "../pxtone/pxtnUnit.h"
#include "../pxtone/pxtnWoice.h"
#include "ActiveTone.h"


#define _8BIT_TOP           0xFF
#define _16BIT_TOP        0x7FFE
#define _BUFSIZE_TIMEPAN    0x40
#define _BUFSIZE_MAX_CH        2

typedef struct
{
	bool          b_on  ;
	bool          b_act ;
	bool          b_loop;
	unsigned long play_id;
	long          pan_vols     [ _BUFSIZE_MAX_CH ];
	long          pan_times    [ _BUFSIZE_MAX_CH ];
	long          pan_time_bufs[ _BUFSIZE_MAX_CH ][ _BUFSIZE_TIMEPAN ];
	long          velos        [ _BUFSIZE_MAX_CH ];
	float         freq_rate;
	long          min_sample_count;

	long          voice_num;
	ptvINSTANCE   voinsts [ _BUFSIZE_MAX_CH ];
	ptvTONE       voitones[ _BUFSIZE_MAX_CH ];
}
STREAMINGVOICETONE2;


// Voice..
static long  _vc_play_id_idx ;
static long  _dst_ch_num     ;
static long  _dst_sps        ;
static long  _dst_bps        ;

static long  _vc_sample_skip ;
static long  _vc_smooth      ;
static long  _vc_top         ;
static CRITICAL_SECTION _cs  ;

static BOOL  _b_init = FALSE ;
static STREAMINGVOICETONE2 *_vc_tones = NULL;
static long  _vc_max_tone    ;
static long  _vc_time_pan_idx;



////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static BOOL _CS_Lock  ( void ){ if( _b_init ) EnterCriticalSection( &_cs ); return _b_init; }
static void _CS_Unlock( void ){ if( _b_init ) LeaveCriticalSection( &_cs );                 }

//////// サンプリング進行 ////////
static void _Voice__Envelope( ptvINSTANCE *p_vi, ptvTONE *p_vt, BOOL bON )
{
	if( p_vt->life_count <= 0 || !p_vi->env_size ) return;

	if( bON )
	{
		if( p_vt->env_pos < p_vi->env_size )
		{
			p_vt->env_volume = p_vi->p_env[ p_vt->env_pos ];
			p_vt->env_pos++;
		}
	}
	else
	{
		if( p_vt->env_pos < p_vi->env_release )
		{
			p_vt->env_volume = p_vt->env_start + ( 0 - p_vt->env_start ) * p_vt->env_pos / p_vi->env_release;
		}
		else
		{
			p_vt->life_count = 0;
			p_vt->env_volume = 0;
		}
		p_vt->env_pos++;
	}
}

static void _Voice_Envelope()
{
	long v;
	for( long t = 0; t < _vc_max_tone; t++ )
	{
		STREAMINGVOICETONE2 *p_tone = &_vc_tones[ t ];

		if( p_tone->b_act )
		{
			for( v = 0; v < p_tone->voice_num; v++ ) _Voice__Envelope( &p_tone->voinsts[ v ], &p_tone->voitones[ v ], p_tone->b_on );
			for( v = 0; v < p_tone->voice_num; v++ ){ if( p_tone->voitones[ v ].life_count >= 0 ) break; }
			if(  v == p_tone->voice_num ) p_tone->b_act = FALSE;
		}
	}
}

static void _Voice_MakeSample( void *p_buf )
{
	ptvINSTANCE         *p_vi  ;
	ptvTONE             *p_vt  ;
	STREAMINGVOICETONE2 *p_tone;

	for( long ch = 0; ch < _dst_ch_num; ch++ )
	{
		for( long t = 0; t < _vc_max_tone; t++ )
		{
			long time_pan_buf = 0;

			p_tone = &_vc_tones[ t ];

			if( p_tone->b_act  )
			{
				for( long v = 0; v < p_tone->voice_num; v++ )
				{
					p_vi = &p_tone->voinsts [ v ];
					p_vt = &p_tone->voitones[ v ];

					long work = 0;

					if( p_vt->life_count >  0 )
					{

						long pos =    (long  )( p_vt->smp_pos ) * 4 + ch * 2;
						work    += *( (short *)&p_vi->p_smp_w[ pos ] );

						if( _dst_ch_num == 1 )
						{
							work += *( (short *)&p_vi->p_smp_w[ pos + 2 ] );
							work  = work / 2;
						}
						work = ( work * p_tone->velos   [ v  ] ) / 128;
						work =   work * p_tone->pan_vols[ ch ]   /  64;

						// smooth
						if( p_vi->env_release )
						{

							if( p_vt->env_volume > 30 &&
								p_vt->env_volume < 100 )
							{
								p_vt->env_volume = p_vt->env_volume;
							}
							work = work * p_vt->env_volume / 128;
						}
						else if( p_tone->b_loop && p_vt->smp_count > p_tone->min_sample_count -_vc_smooth )
						{
							work  = work * ( p_tone->min_sample_count - p_vt->smp_count ) /_vc_smooth;
						}

					} // voice.bActive;

					time_pan_buf += work;

				}// v
			}
			p_tone->pan_time_bufs[ ch ][ _vc_time_pan_idx ] = time_pan_buf;
		}// t

	}
		
	// time pan
	for( long ch = 0; ch < _dst_ch_num; ch++ )
	{
		long work = 0;
		long index;

		for( long t = 0; t < _vc_max_tone; t++ )
		{
			p_tone = &_vc_tones[ t ];
			index = ( _vc_time_pan_idx - p_tone->pan_times[ ch ] ) & ( _BUFSIZE_TIMEPAN - 1 );
			work += p_tone->pan_time_bufs[ ch ][ index ];
		}
		
		if( _dst_bps == 8 ) work = work >> 8;
//		work = (long)( (float)work * _vc_master_vol );

		// to buf.
		if( work >  _vc_top ) work =  _vc_top;
		if( work < -_vc_top ) work = -_vc_top;
		
		if( _dst_bps == 8 ) *( (char *)p_buf + ch * 2 ) = (char )( work - 128 );
		else                *( (short*)p_buf + ch     ) = (short)( work       );
	}
}

static void _Voice_ToNextSample()
{
	ptvINSTANCE         *p_vi  ;
	ptvTONE             *p_vt  ;
	STREAMINGVOICETONE2 *p_tone;

	for( long t = 0; t < _vc_max_tone; t++ )
	{
		p_tone = &_vc_tones[ t ];

		if( p_tone->b_act )
		{
			for( long v = 0; v < p_tone->voice_num; v++ )
			{
				p_vi = &p_tone->voinsts [ v ];
				p_vt = &p_tone->voitones[ v ];
			
				if( p_vt->life_count >= 0 )
				{
					p_vt->smp_pos += p_tone->freq_rate * p_vt->offset_freq * _vc_sample_skip;

					if( p_tone->b_loop )
					{
						if( p_vt->smp_pos >= p_vi->smp_body_w ) p_vt->smp_pos -= p_vi->smp_body_w;
						if( p_vt->smp_pos >= p_vi->smp_body_w ) p_vt->smp_pos  = 0;
						if( !p_vi->smp_tail_w && !p_vi->env_release && !p_tone->b_on ) p_vt->smp_count++;
					}
					else
					{
						if( p_vt->smp_pos >= p_vi->smp_body_w ) p_vt->life_count = 0;
						if( !p_tone->b_on                     ) p_vt->smp_count++;
					}
					if( p_vt->smp_count >= p_tone->min_sample_count ) p_vt->life_count = 0;
				}

			}

			p_tone->b_act = FALSE;
			for( long v = 0; v < p_tone->voice_num; v++ )
			{
				if( p_tone->voitones[ v ].life_count > 0 ){ p_tone->b_act = TRUE; break; }
			}
		}
	}
	_vc_time_pan_idx = ( _vc_time_pan_idx + 1 ) & ( _BUFSIZE_TIMEPAN - 1 );
}

static BOOL _Voice_Set_New( TUNEUNITTONESTRUCT *p_u, bool b_loop, float freq, long vol_pan, long velo, long time_pan )
{
	BOOL b_ret = FALSE;
	if( !_CS_Lock() ) goto End;

	char str[ 100 ];
	sprintf( str, "mode:%d / frea:%f / vpan:%d / vel:%d / tpan:%d\n", (long)b_loop, freq, vol_pan, velo, time_pan );
	OutputDebugString( str );

	long t;
	for( t = 0; t < _vc_max_tone; t++ ){ if( !_vc_tones[ t ].b_act ) break; }
	if( t == _vc_max_tone ) goto End;

	if( vol_pan  == -1 ) vol_pan  = 64;
	if( time_pan == -1 ) time_pan = 64;
	if( velo     == -1 ) velo     = 104;

	_vc_play_id_idx++; if( !_vc_play_id_idx ) _vc_play_id_idx = 1;

	STREAMINGVOICETONE2 *p_tone = &_vc_tones[ t ];
	p_tone->b_on             = TRUE;
	p_tone->b_act            = TRUE;
	p_tone->freq_rate        = freq;
	for( long i = 0; i < _BUFSIZE_MAX_CH; i++ ){ p_tone->velos[ i ] = velo; }
	p_tone->play_id          = _vc_play_id_idx;
	p_tone->b_loop           = b_loop;
	p_tone->min_sample_count = _dst_sps * 100 / 1000;
	p_tone->voice_num        = p_u->_p_woice->_voice_num;

	// volume pan
	p_tone->pan_vols[ 0 ] = 64;
	p_tone->pan_vols[ 1 ] = 64;
	if( _dst_ch_num == 2 )
	{
		if( vol_pan >= 64 ) p_tone->pan_vols[ 0 ] = 128 - vol_pan;
		else                p_tone->pan_vols[ 1 ] =       vol_pan;
	}

	// time pan
	p_tone->pan_times[ 0 ] = 0;
	p_tone->pan_times[ 1 ] = 0;
	if( _dst_ch_num == 2 )
	{
		long idx = (time_pan >= 64) ? 0 : 1;
		//                                            positive        |   negative
			p_tone->pan_times[ idx ] = (idx == 0) ? ( time_pan - 64 ) : ( 64 - time_pan );
		if( p_tone->pan_times[ idx ] >= 64 ) p_tone->pan_times[ idx ] = 63;
			p_tone->pan_times[ idx ] = (long)( ((double)p_tone->pan_times[ idx ] * 44100) / _dst_sps );
	}

	for( long v = 0; v < _BUFSIZE_MAX_CH; v++ ) p_tone->voitones[ v ].life_count = 0;

	for( long v = 0; v < p_tone->voice_num; v++ )
	{
			  ptvTONE     *p_vt     = &p_tone->voitones       [ v ];
			  ptvINSTANCE *p_vi     = &p_tone->voinsts        [ v ];
		const ptvINSTANCE *p_vi_src = &p_u->_p_woice->_voinsts[ v ];
		const ptvUNIT     *p_tv_src = &p_u->_p_woice->_voices [ v ];

		p_vt->life_count = 1;
		p_vt->smp_pos    = 0;
		p_vt->smp_count  = 0;
		p_vt->env_pos    = 0;


		if( p_tv_src->voice_flags & PTV_VOICEFLAG_BEATFIT )
		{
			p_vt->offset_freq = (float)( ( (double)p_vi_src->smp_body_w * pxtnMaster_Get_BeatTempo() ) / ( 44100.0 * 60 * p_tv_src->tuning ) );
		}
		else
		{
			p_vt->offset_freq = pxtnFrequency_Get( EVENTDEFAULT_BASICKEY - p_tv_src->basic_key ) * p_tv_src->tuning;
		}

		p_vi->p_smp_w     = (unsigned char *)p_vi_src->p_smp_w;
		p_vi->smp_head_w  = p_vi_src->smp_head_w;
		p_vi->smp_body_w  = p_vi_src->smp_body_w;
		p_vi->smp_tail_w  = p_vi_src->smp_tail_w;

		p_vi->env_size    = p_vi_src->env_size   ;
		p_vi->p_env       = p_vi_src->p_env      ;
		p_vi->env_release = p_vi_src->env_release;

		p_vt->env_volume  = p_vt->env_start = (p_vi_src->env_size) ? 0 : 128;
	}

	b_ret = TRUE;
End:
	_CS_Unlock();

	return b_ret;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

void ActiveTone_Voice_Release( void )
{
	pxMem_free( (void**)&_vc_tones );
	_vc_max_tone = 0;
	_b_init = FALSE;
	DeleteCriticalSection( &_cs );
}

BOOL ActiveTone_Voice_Initialize( long ch_num, long sps, long bps, long size )
{
	BOOL b_ret = FALSE;
	if( _b_init ) goto End;

	_dst_ch_num      = ch_num;
	_dst_sps         = sps;
	_dst_bps         = bps;
	_vc_smooth       = sps / 100;
	_vc_max_tone     = size;
	_vc_sample_skip  = 44100 / sps;
	_vc_time_pan_idx = 0;
	_vc_top          = (bps == 8 ? 0x7F : 0x7FFE);

	if( !pxMem_zero_alloc( (void**)&_vc_tones, sizeof(STREAMINGVOICETONE2) * size ) ) goto End;
	
	InitializeCriticalSection( &_cs );

	_b_init = TRUE;
	b_ret = TRUE;
End:
	if( !b_ret ) ActiveTone_Voice_Release();

	return b_ret;
}

void ActiveTone_Voice_Sampling( void *p1, long size )
{
	return; // this function was somehow corrupting 8-bit streaming playback
	if( !_CS_Lock() ) return;

	long offset;
	long work  ;
	WORD word[ 2 ];
	BYTE byte[ 4 ];

	long t = 0;
	// 8bits.
	if( _dst_bps == 8 )
	{
		BYTE *p = (BYTE*)p1;
		for( long b = 0; b < size; b++ )
		{
			offset = ( t % _dst_ch_num );
			if( !offset ){ _Voice_Envelope(); _Voice_MakeSample( byte ); _Voice_ToNextSample(); }
			work = p[ b ] + byte[ offset ];
			pxMem_cap( &work, _8BIT_TOP, 1 );
			p[ b ] = (BYTE)work;
			t++;
		}
	}
	// 16bits.
	else
	{
		WORD *p = (WORD *)p1;
		for( long b = 0; b < size / 2; b++ )
		{
			offset = ( t % _dst_ch_num );
			if( !offset ){ _Voice_Envelope(); _Voice_MakeSample( word ); _Voice_ToNextSample(); }
			work = p[ b ] + word[ offset ];
			pxMem_cap( &work, _16BIT_TOP, -_16BIT_TOP );
			p[ b ] = (WORD)work;
			t++;
		}
	}
	_CS_Unlock();
}

void ActiveTone_Voice_Stop( long play_id, BOOL b_force )
{
	if( !_CS_Lock() ) return;

	for( long t = 0; t < _vc_max_tone; t++ )
	{
		if( _vc_tones[ t ].b_act && _vc_tones[ t ].play_id == play_id )
		{
			_vc_tones[ t ].b_on = FALSE;
			if( b_force )
			{
				_vc_tones[ t ].b_act = FALSE;
				for( long v = 0; v < _vc_tones[ t ].voice_num; v++ ) _vc_tones[ t ].voitones[ v ].life_count = 0;
			}
			else
			{
				ptvTONE *p_vt;
				for( long v = 0; v < _vc_tones[ t ].voice_num; v++ )
				{
					p_vt            = &_vc_tones[ t ].voitones[ v ];
					p_vt->env_start = p_vt->env_volume;
					p_vt->env_pos   = 0;
				}
			}
		}
	}
	_CS_Unlock();
}

long ActiveTone_Voice_Get_PlayId( void )
{
	for( long idx = 0; idx < _vc_max_tone; idx++ ){ if( _vc_tones[ idx ].play_id ) return idx; }
	return -1;
}

void ActiveTone_Voice_StopAll( void )
{
	if( !_CS_Lock() ) return;

	for( long t = 0; t < _vc_max_tone; t++ )
	{
		if( _vc_tones[ t ].b_act )
		{
			_vc_tones[ t ].b_on = FALSE;
			for( long v = 0; v < _vc_tones[ t ].voice_num; v++ ) _vc_tones[ t ].voitones[ v ].life_count = 0;
		}
	}
	_CS_Unlock();
}

void ActiveTone_Voice_Freq( long play_id, float freq )
{
	if( !_CS_Lock() ) return;

	for( long t = 0; t < _vc_max_tone; t++ )
	{
		if( _vc_tones[ t ].b_act && _vc_tones[ t ].play_id == play_id ){ _vc_tones[ t ].freq_rate = freq; break; }
	}
	_CS_Unlock();
}
