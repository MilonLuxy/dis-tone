#include <StdAfx.h>


#include "../Fixture/CriticalSection.h"
#include "../Fixture/DebugLog.h"
#include "../Fixture/pxMem.h"
#include "../Streaming/Streaming.h"
#include "pxtnDelay.h"
#include "pxtnEvelist.h"
#include "pxtnGroup.h"
#include "pxtnMaster.h"
#include "pxtnOverDrive.h"
#include "pxtnUnit.h"
#include "pxtnWoice.h"
#include "pxtnService.h"


#define _BASE_SPS              44100
#define _DEF_TPAN                 64
#define _TPAN_ROUGH               63

static TUNEUNITTONESTRUCT **_units ;
static DELAYSTRUCT        **_delays;
static OVERDRIVE_STRUCT   **_ovdrvs;
static s32 *_moo_group_smps = NULL;

static s32 _unit_num, _delay_num, _ovdrv_num, _group_num;
static s32 _dst_ch_num, _dst_sps, _dst_byte_per_smp;

static b32 _moo_b_init          = _false;
static b32 _moo_b_mute_by_unit  = _false;
static b32 _moo_b_loop          = _true ;

static const f32 *_moo_freqtable = NULL;
static s32 _moo_tablesize      =     0;

static f32 _moo_clock_rate     =     0; // as the sample
static s32 _moo_smp_smooth     =     0;
static s32 _moo_smp_count      =     0;
static s32 _moo_smp_start      =     0;
static s32 _moo_smp_end        =     0;
static s32 _moo_smp_repeat     =     0;

static s32 _moo_fade_count     =     0;
static s32 _moo_fade_max       =     0;
static s32 _moo_fade_fade      =     0;
static f32 _moo_master_vol     =  1.0f;

static s32 _moo_top            =     0;
static f32 _moo_smp_stride     =     0;
static s32 _moo_time_pan_index =     0;

// for make now-meas
static s32 _moo_bt_num         =     0;
static f32 _moo_bt_tempo       =     0;
static s32 _moo_bt_clock       =     0;

static const EVERECORD *_moo_p_eve = NULL;


////////////////////////////////////////////////
// Units   ////////////////////////////////////
////////////////////////////////////////////////

static void _moo_ResetVoiceOn( TUNEUNITTONESTRUCT *p_u, s32 w )
{
	p_u->_key_now    = EVENTDEFAULT_KEY;
	p_u->_key_margin = 0;
	p_u->_key_start  = EVENTDEFAULT_KEY;

	if( !(p_u->_p_woice = pxtnWoice_GetVariable( w )) ) return;

	for( s32 v = 0; v < p_u->_p_woice->_voice_num; v++ )
	{
		const ptvINSTANCE *p_inst = &p_u->_p_woice->_voinsts[ v ];
		const ptvUNIT     *p_vc   = &p_u->_p_woice->_voices [ v ];

		f32 ofs_freq = 0;
		if( p_vc->voice_flags & PTV_VOICEFLAG_BEATFIT )
		{
			ofs_freq = ( p_inst->smp_body_w * _moo_bt_tempo ) / ( _BASE_SPS * 60 * p_vc->tuning );
		}
		else
		{
			s32 i = (EVENTDEFAULT_BASICKEY + EVENTDEFAULT_KEY) - p_vc->basic_key >> 4; // ÷ 16
			pxMem_cap( &i, _moo_tablesize - 1, 0 );
			ofs_freq = _moo_freqtable[ i ] * p_vc->tuning;
		}

		ptvTONE *p_tone = &p_u->_vts[ v ];
		p_tone->life_count        = 0;
		p_tone->on_count          = 0;
		p_tone->smp_pos           = 0;
		p_tone->smooth_volume     = 0;
		p_tone->env_release_clock = (s32)( p_inst->env_release / _moo_clock_rate );
		p_tone->offset_freq       = ofs_freq;
	}
}

static void _moo_InitUnitTone( void )
{
	for( s32 u = 0; u < _unit_num; u++ )
	{
		if( _units[ u ] )
		{
			TUNEUNITTONESTRUCT *p_u    = _units[ u ];
			p_u->_pan_vols [ 0 ]       = EVENTDEFAULT_PAN_VOLUME;
			p_u->_pan_vols [ 1 ]       = EVENTDEFAULT_PAN_VOLUME;
			p_u->_pan_times[ 0 ]       = EVENTDEFAULT_PAN_TIME  ;
			p_u->_pan_times[ 1 ]       = EVENTDEFAULT_PAN_TIME  ;
			p_u->_v_VELOCITY           = EVENTDEFAULT_VELOCITY  ;
			p_u->_v_VOLUME             = EVENTDEFAULT_VOLUME    ;
			p_u->_v_GROUPNO            = EVENTDEFAULT_GROUPNO   ;
			p_u->_v_TUNING             = EVENTDEFAULT_TUNING    ;
			p_u->_portament_sample_num = EVENTDEFAULT_PORTAMENT ;
			p_u->_portament_sample_pos = EVENTDEFAULT_PORTAMENT ;
			_moo_ResetVoiceOn( p_u, EVENTDEFAULT_VOICENO );
		}
	}
}



////////////////////////////////////////////////
// Render proc   ///////////////////////////////
////////////////////////////////////////////////

static void _moo_LoopSample( void )
{
	_moo_smp_count = _moo_smp_repeat;
	_moo_p_eve     = pxtnEvelist_Get_Records();
	_moo_InitUnitTone();
}

static b32 _PXTONE_SAMPLE( void *p_data )
{
	if( !_moo_b_init ) return _false;
	TUNEUNITTONESTRUCT *p_u  = NULL;
	const WOICE_STRUCT *p_w  = NULL;
	const ptvINSTANCE  *p_vi = NULL;
		  ptvTONE      *p_vt = NULL;
		  DELAYSTRUCT  *dela = NULL;
		  s32           work = 0;

	// envelope..
	for( s32 u = 0; u < _unit_num;  u++ )
	{
		// Tone_Envelope() in later versions
		p_u = _units[ u ];
		p_w = p_u->_p_woice;
		if( p_w )
		{
			for( s32 v = 0; v < p_w->_voice_num; v++ )
			{
				p_vi = &p_w->_voinsts[ v ];
				p_vt = &p_u->_vts[ v ];

				if( p_vt->life_count > 0 && p_vi->env_size )
				{
					if( p_vt->on_count > 0 )
					{
						if( p_vt->env_pos < p_vi->env_size )
						{
							p_vt->env_volume = p_vi->p_env[ p_vt->env_pos ];
							p_vt->env_pos++;
						}
					}
					// release.
					else
					{
						p_vt->env_volume = p_vt->env_start + ( 0 - p_vt->env_start ) * p_vt->env_pos / p_vi->env_release;
						p_vt->env_pos++;
					}
				}
			}
		}
	}

	s32 clock = pxtnServiceMoo_Get_NowClock();

	// events..
	for( ; _moo_p_eve && _moo_p_eve->clock <= clock; _moo_p_eve = _moo_p_eve->next )
	{
		s32 u = _moo_p_eve->unit_no;
		p_u   = _units[ u ];

		switch( _moo_p_eve->kind )
		{
		case EVENTKIND_ON       : 
			{
				s32 on_count = (s32)( (_moo_p_eve->clock + _moo_p_eve->value - clock) * _moo_clock_rate );
				if( on_count <= 0 ){ for( s32 i = 0; i < _dst_ch_num; i++ ) p_u->_vts[ i ].life_count = 0; break; }
				
				// Tone_KeyOn() in later versions
				{
					p_u->_key_now    = p_u->_key_start + p_u->_key_margin;
					p_u->_key_start  = p_u->_key_now;
					p_u->_key_margin = 0;
				}

				if( !( p_w = p_u->_p_woice ) ) break;
				for( s32 v = 0; v < p_w->_voice_num; v++ )
				{
					p_vt = &p_u->_vts    [ v ];
					p_vi = &p_w->_voinsts[ v ];

					// release..
					if( p_vi->env_release )
					{
						s32 max_life_count1 = (s32)( ( _moo_p_eve->value - ( clock - _moo_p_eve->clock ) ) * _moo_clock_rate ) + p_vi->env_release;
						s32 max_life_count2;
						s32 c     = _moo_p_eve->clock + _moo_p_eve->value + p_vt->env_release_clock;
						EVERECORD *next = NULL;
						for( EVERECORD *p = _moo_p_eve->next; p; p = p->next )
						{
							if( p->clock > c ) break;
							if( p->unit_no == u && p->kind == EVENTKIND_ON ){ next = p; break; }
						}
						if( !next ) max_life_count2 = _moo_smp_end - (s32)(  clock   * _moo_clock_rate );
						else        max_life_count2 = (s32)( ( next->clock - clock ) * _moo_clock_rate );
						p_vt->life_count = (max_life_count1 < max_life_count2) ? max_life_count1 : max_life_count2;
					}
					// no-release..
					else
					{
						p_vt->life_count = (s32)( ( _moo_p_eve->value - ( clock - _moo_p_eve->clock ) ) * _moo_clock_rate );
					}

					if( p_vt->life_count > 0 )
					{
						p_vt->on_count   = on_count;
						p_vt->smp_pos    = 0;
						p_vt->env_pos    = 0;
						p_vt->env_volume = p_vt->env_start = (p_vi->env_size) ? 0 : 128; // envelope | no-envelope
					}
				}
				break;
			}

		case EVENTKIND_KEY       :
			p_u->_key_start            = p_u->_key_now;
			p_u->_key_margin           = _moo_p_eve->value - p_u->_key_start;
			p_u->_portament_sample_pos = 0;
			break;
		case EVENTKIND_PAN_VOLUME:
			p_u->_pan_vols[ 0 ] = _DEF_TPAN;
			p_u->_pan_vols[ 1 ] = _DEF_TPAN;

			if( _dst_ch_num == 2 )
			{
				if( _moo_p_eve->value >= _DEF_TPAN ) p_u->_pan_vols[ 0 ] = 128 - _moo_p_eve->value;
				else                                 p_u->_pan_vols[ 1 ] =       _moo_p_eve->value;
			}
			break;
		case EVENTKIND_PAN_TIME  :
			p_u->_pan_times[ 0 ] = 0;
			p_u->_pan_times[ 1 ] = 0;

			if( _dst_ch_num == 2 )
			{
				s32 idx = (_moo_p_eve->value >= _DEF_TPAN) ? 0 : 1;
				//                                          positive                 |   negative
					p_u->_pan_times[ idx ] = (idx == 0) ? (_moo_p_eve->value - _DEF_TPAN) : (_DEF_TPAN - _moo_p_eve->value);
				if( p_u->_pan_times[ idx ] >= _DEF_TPAN ) p_u->_pan_times[ idx ] = _TPAN_ROUGH;
					p_u->_pan_times[ idx ] =             (p_u->_pan_times[ idx ] * _BASE_SPS) / _dst_sps;
			}
			break;
		case EVENTKIND_VELOCITY  : p_u->_v_VELOCITY = _moo_p_eve->value; break;
		case EVENTKIND_VOLUME    : p_u->_v_VOLUME   = _moo_p_eve->value; break;
		case EVENTKIND_PORTAMENT : p_u->_portament_sample_num = (s32)( _moo_p_eve->value * _moo_clock_rate ); break;
		case EVENTKIND_BEATCLOCK : break;
		case EVENTKIND_BEATTEMPO : break;
		case EVENTKIND_BEATNUM   : break;
		case EVENTKIND_REPEAT    : break;
		case EVENTKIND_LAST      : break;
		case EVENTKIND_VOICENO   : _moo_ResetVoiceOn( p_u, _moo_p_eve->value ); break;
		case EVENTKIND_GROUPNO   : p_u->_v_GROUPNO = _moo_p_eve->value; break;
		case EVENTKIND_TUNING    : p_u->_v_TUNING  = *( (f32*)(&_moo_p_eve->value) ); break;
		}
	}

	// sampling..
	for( s32 u = 0; u < _unit_num; u++ )
	{
		p_u = _units[ u ];
		if( p_w = p_u->_p_woice )
		{
//			if( _moo_b_mute_by_unit && !p_u->_bPlayed )
//			{
//				for( s32 ch = 0; ch < _dst_ch_num; ch++ ) p_u->_pan_time_bufs[ ch ][ _moo_time_pan_index ] = 0;
//				continue;
//			}

			for( s32 ch = 0; ch < _dst_ch_num; ch++ )
			{
				s32 time_pan_buf = 0;

				for( s32 v = 0; v < p_w->_voice_num; v++ )
				{
					p_vi = &p_w->_voinsts[ v ];
					p_vt = &p_u->_vts    [ v ];

					work = 0;

					if( p_vt->life_count > 0 )
					{
						s32 pos =    (s32 )( p_vt->smp_pos ) * 4 + ch * 2;
						work   += *( (s16*)&p_vi->p_smp_w[ pos ] );

						if( _dst_ch_num == 1 )
						{
							work += *( (s16*)&p_vi->p_smp_w[ pos + 2 ] );
							work  = work / 2;
						}

						work = ( work * p_u->_v_VELOCITY )   >> 7; // ÷ 128
						work = ( work * p_u->_v_VOLUME   )   >> 7; // ÷ 128
						work =   work * p_u->_pan_vols[ ch ] >> 6; // ÷ 64

						if( p_vi->env_size ) work = work * p_vt->env_volume >> 7; // ÷ 128

						// smooth tail
						if( p_w->_voices[ v ].voice_flags & PTV_VOICEFLAG_SMOOTH && p_vt->life_count < _moo_smp_smooth )
						{
							work = work * p_vt->life_count / _moo_smp_smooth;
						}
					}
					time_pan_buf += work;
				}
				p_u->_pan_time_bufs[ ch ][ _moo_time_pan_index ] = time_pan_buf;
			}
		}
	}

	for( s32 ch = 0; ch < _dst_ch_num; ch++ )
	{
		for( s32 g = 0; g < _group_num; g++ ) _moo_group_smps[ g ] = 0;
		for( s32 u = 0; u < _unit_num ; u++ )
		{
			s32 idx = ( _moo_time_pan_index - _units[ u ]->_pan_times[ ch ] ) & _TPAN_ROUGH;
			_moo_group_smps[ _units[ u ]->_v_GROUPNO ] += _units[ u ]->_pan_time_bufs[ ch ][ idx ];
		}
		for( s32 d = 0; d < _delay_num; d++ )
		{
			dela = _delays[ d ];
			if( dela->_smp_num )
			{
				s32 a = dela->_bufs[ ch ][ dela->_offset ] * dela->_rate_s32/ 100;
				if( dela->_b_played ) _moo_group_smps[ dela->_group ] += a;
				         dela->_bufs[ ch ][ dela->_offset ] = _moo_group_smps[ dela->_group ];
			}
		}
		for( s32 o = 0; o < _ovdrv_num; o++ )
		{
			if( _ovdrvs[ o ]->_b_played )
			{
				work = _moo_group_smps[ _ovdrvs[ o ]->_group ];
				if(      work >  _ovdrvs[ o ]->_cut_16bit_top ) work =  _ovdrvs[ o ]->_cut_16bit_top;
				else if( work < -_ovdrvs[ o ]->_cut_16bit_top ) work = -_ovdrvs[ o ]->_cut_16bit_top;
				_moo_group_smps[ _ovdrvs[ o ]->_group ] = (s32)( (f32)work * _ovdrvs[ o ]->_amp );
			}
		}

		// collect.
		work = 0;
		for( s32 g = 0; g < _group_num; g++ ) work += _moo_group_smps[ g ];

		// fade..
		if( _moo_fade_fade && _moo_fade_max ) work = work * ( _moo_fade_count >> 8 ) / _moo_fade_max;

		// master volume
		work = (s32)( (f32)work * _moo_master_vol );
		if( _dst_byte_per_smp == 8 ) work = work >> 8; // ÷ 256

		// to buffer..
		pxMem_cap( &work, _moo_top, -_moo_top );
		
		if( _dst_byte_per_smp == 8 ) *( (u8 *)p_data + ch ) = (u8 )( work + 128 );
		else                         *( (s16*)p_data + ch ) = (s16)work;
	}

	// --------------
	// increments..

	_moo_smp_count++;
	_moo_time_pan_index = ( _moo_time_pan_index + 1 ) & _TPAN_ROUGH;

	for( s32 u = 0; u < _unit_num;  u++ )
	{
		// Tone_Increment_Key() in later versions
		p_u = _units[ u ];
		{
			// prtament..
			if( p_u->_portament_sample_num && p_u->_key_margin )
			{
				if( p_u->_portament_sample_pos < p_u->_portament_sample_num )
				{
					p_u->_portament_sample_pos++;
					p_u->_key_now = (s32)( p_u->_key_start + (f64)p_u->_key_margin *
						p_u->_portament_sample_pos / p_u->_portament_sample_num );
				}
				else
				{
					p_u->_key_now    = p_u->_key_start + p_u->_key_margin;
					p_u->_key_start  = p_u->_key_now;
					p_u->_key_margin = 0;
				}
			}
			else p_u->_key_now = p_u->_key_start + p_u->_key_margin;
		}

		f32 freq;
		{
			s32 i = ( p_u->_key_now >> 4 ); // ÷ 16
			pxMem_cap( &i, _moo_tablesize - 1, 0 );
			freq = _moo_freqtable[ i ] * _moo_smp_stride;
		}

		// Tone_Increment_Sample() in later versions
		{
			if( p_w = _units[ u ]->_p_woice )
			{
				for( s32 v = 0; v < p_w->_voice_num; v++ )
				{
					p_vi = &p_w->_voinsts[ v ];
					p_vt = &p_u->_vts    [ v ];

					if( p_vt->life_count > 0 ) p_vt->life_count--;
					if( p_vt->life_count > 0 )
					{
						p_vt->on_count--;

						p_vt->smp_pos += p_vt->offset_freq * p_u->_v_TUNING * freq;

						if( p_vt->smp_pos >= p_vi->smp_body_w )
						{
							if( p_w->_voices[ v ].voice_flags & PTV_VOICEFLAG_WAVELOOP )
							{
								if( p_vt->smp_pos >= p_vi->smp_body_w ) p_vt->smp_pos -= p_vi->smp_body_w;
								if( p_vt->smp_pos >= p_vi->smp_body_w ) p_vt->smp_pos  = 0;
							}
							else p_vt->life_count = 0;
						}

						// OFF
						if( p_vt->on_count == 0 && p_vi->env_size ){ p_vt->env_start = p_vt->env_volume; p_vt->env_pos = 0; }
					}
				}
			}
		}
	}

	// delay
	for( s32 d = 0; d < _delay_num; d++ )
	{
		dela = _delays[ d ];
		if( dela->_smp_num )
		{
			if( ++dela->_offset >= dela->_smp_num ) dela->_offset = 0;
		}
	}

	// fade out
	if( _moo_fade_fade < 0 )
	{
		if( _moo_fade_count > 0 ) _moo_fade_count--;
		else return _false;
	}
	// fade in
	else if( _moo_fade_fade > 0 )
	{
		if( _moo_fade_count < (_moo_fade_max << 8) ) _moo_fade_count++; // × 256
		else                                         _moo_fade_fade = 0;
	}

	if( _moo_smp_count >= _moo_smp_end )
	{
		if( !_moo_b_loop ) return _false;
		_moo_LoopSample();
	}
	return _true;
}



///////////////////////
// get / set 
///////////////////////

b32  pxtnServiceMoo_Is_Finished( void ) // bEnd
{
	if( _moo_fade_fade == -1 && _moo_fade_count == 0 ) return _true;
	if( _moo_smp_count < _moo_smp_end ) return _false;
	return _true;
}
b32  pxtnServiceMoo_Is_Prepared    ( void  ){ return ( _units && _delays && _moo_group_smps ); }
s32  pxtnServiceMoo_Get_NowClock   ( void  ){ return (_moo_clock_rate) ? (s32)( _moo_smp_count / _moo_clock_rate ) : 0; }
s32  pxtnServiceMoo_Get_EndClock   ( void  ){ return (_moo_clock_rate) ? (s32)( _moo_smp_end   / _moo_clock_rate ) : 0; }
void pxtnServiceMoo_SetMute_By_Unit( b32 b ){ _moo_b_mute_by_unit = b; }
void pxtnServiceMoo_SetLoop        ( b32 b ){ _moo_b_loop         = b; }

b32  pxtnServiceMoo_SetFade( s32 fade, s32 msec )
{
	_moo_fade_max = ( _dst_sps * msec ) / 1000 >> 8; // ÷ 256
	switch( fade )
	{
		case -1: _moo_fade_fade = -1; _moo_fade_count = _moo_fade_max << 8; break; // out
		case  0: _moo_fade_fade =  1; _moo_fade_count =  0;                 break; // in
		case  1: _moo_fade_fade =  0;                                       break; // off
		default: return _false;
	}
	return _true;
}


////////////////////////////
// preparation
////////////////////////////

// preparation
b32 pxtnServiceMoo_Preparation( const pxtnVOMITPREPARATION *p_prep )
{
	if( p_prep->start_pos_sample > p_prep->start_pos_meas || p_prep->start_pos_sample > p_prep->meas_end ) return _false;
	if( !CriticalSection_In() ) return _false;

	b32 b_ret   = _false;
	_moo_b_init = _false;

	dlog( "sampleTune_Start( 0 );" );

	Streaming_Get_SampleInfo( (s32*)&_dst_ch_num, (s32*)&_dst_sps, (s32*)&_dst_byte_per_smp );

	_moo_freqtable      = pxtnFrequency_GetDirect ( &_moo_tablesize );
	_unit_num           = pxtnUnit_Count          ();
	_delay_num          = pxtnDelay_Count         ();
	_group_num          = pxtnGroup_Get           ();
	_ovdrv_num          = pxtnOverDrive_Count     ();
	_moo_bt_clock       = pxtnMaster_Get_BeatClock();
	_moo_bt_num         = pxtnMaster_Get_BeatNum  ();
	_moo_bt_tempo       = pxtnMaster_Get_BeatTempo();
	_moo_clock_rate     = (f32)( 60.0f * (f64)_dst_sps / ( (f64)_moo_bt_tempo * (f64)_moo_bt_clock ) );
	_moo_smp_stride     = ( 44100.0f / _dst_sps );
	_moo_time_pan_index = 0;
	_moo_smp_start      = (s32)( (f64)p_prep->start_pos_meas * (f64)_moo_bt_num * (f64)_moo_bt_clock * _moo_clock_rate );
	_moo_smp_end        = (s32)( (f64)p_prep->meas_end       * (f64)_moo_bt_num * (f64)_moo_bt_clock * _moo_clock_rate );
	_moo_smp_repeat     = (s32)( (f64)p_prep->meas_repeat    * (f64)_moo_bt_num * (f64)_moo_bt_clock * _moo_clock_rate );

	if( p_prep->meas_repeat && p_prep->meas_repeat < _moo_smp_end ) _moo_smp_start = p_prep->meas_repeat;

	_moo_smp_count  = _moo_smp_start;
	_moo_smp_smooth = _dst_sps / 250; // (0.004sec) // (0.010sec)
	_moo_top = (_dst_byte_per_smp == 8) ? MAX_S8BIT : MAX_S16BIT;

	if( p_prep->fadein_sec > 0 ) pxtnServiceMoo_SetFade( 1, p_prep->fadein_sec );
	else                         pxtnServiceMoo_SetFade( 0,                  0 );
	
	if( !pxMem_zero_alloc( (void**)&_units         , sizeof(s32) * _unit_num  ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_ovdrvs        , sizeof(s32) * _ovdrv_num ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_delays        , sizeof(s32) * _delay_num ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_moo_group_smps, sizeof(s32) * _group_num ) ) goto End;

	for( s32 u = 0; u < _unit_num ; u++ )
	{
		_units [ u ] = pxtnUnit_GetVariable( u );
		memset( _units[ u ]->_pan_time_bufs, 0, sizeof(_units[ u ]->_pan_time_bufs) );
	}
	for( s32 o = 0; o < _ovdrv_num; o++ ) _ovdrvs[ o ] = pxtnOverDrive_GetVariable( o );
	for( s32 d = 0; d < _delay_num; d++ ) _delays[ d ] = pxtnDelay_GetVariable    ( d );

	pxtnDelay_Tone_Clear    ();
	pxtnOverDrive_Tone_Clear();

	_moo_p_eve = pxtnEvelist_Get_Records();

	_moo_InitUnitTone();

	_moo_b_init = _true;
	b_ret       = _true;
End:
	CriticalSection_Out();
	if( !b_ret ){ dlog( "sampleTune_Start( false );" ); pxtnServiceMoo_Release(); }
	else          dlog( "sampleTune_Start( true );"  );

	return b_ret;
}

void pxtnServiceMoo_Release( void )
{
	if( !CriticalSection_In() ) return;
	_moo_b_init = _false;
	pxMem_free( (void**)&_delays         );
	pxMem_free( (void**)&_ovdrvs         );
	pxMem_free( (void**)&_units          );
	pxMem_free( (void**)&_moo_group_smps );
	CriticalSection_Out();
}

s32  pxtnServiceMoo_Get_SamplingOffset( void ){ return _moo_smp_count; }

void pxtnServiceMoo_Set_Master_Volume( f32 v ){ pxMem_cap( &v, 1, 0 ); _moo_master_vol = v; }



////////////////////
// Moo..
////////////////////

b32 pxtnServiceMoo_Proc( void *p_buf, s32 size )
{
	if( !_moo_b_init || !CriticalSection_In() ) return _false;
	b32 b_ret = _false;

//	s32 smp_num = _dst_ch_num * _dst_byte_per_smp/8;

	s32 ch;
	s32 t = 0;
	// 8bits.
	if( _dst_byte_per_smp == 8 )
	{
		u8 *p8 = (u8 *)p_buf;
		u8 sample[ 2 ];

		for( s32 b = 0; b < size; b++ )
		{
			ch = ( t % _dst_ch_num );
			if( !ch ){ if( !_PXTONE_SAMPLE( sample ) ) goto End; }
			p8[ b ] = sample[ ch ];
			t++;
		}
	}
	// 16bits.
	else
	{
		s16 *p16 = (s16*)p_buf;
		s16 sample[ 2 ];

		for( s32 b = 0; b < size / 2; b++ )
		{
			ch = ( t % _dst_ch_num );
			if( !ch ){ if( !_PXTONE_SAMPLE( sample ) ) goto End; }
			p16[ b ] = sample[ ch ];
			t++;
		}
	}

	b_ret = _true;
End:
	CriticalSection_Out();

	return b_ret;
}
