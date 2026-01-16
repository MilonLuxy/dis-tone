#include <StdAfx.h>

#include "pxSound.h"

#ifdef pxINCLUDE_PT4i

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "PT4i.h"


#define MAX_CHANNEL      6
#define MAX_PITCH       81
#define WAIT_ROUGH     600 // for wait conversion
#define MAX_EVENT   0xFFFF // old -> 2048
#define PITCH_ROUGH 1.059463f

enum CHANNELFLAGS
{
	_CH_FLAG_on         = 0x01,
	_CH_FLAG_loop       = 0x10,
	_CH_FLAG_reset_mask = ~(_CH_FLAG_on),
};

enum EVETYPE
{
	_EVE_TYPE_none = 0,
	_EVE_TYPE_wait    ,
	_EVE_TYPE_on      ,
	_EVE_TYPE_vol     ,
	_EVE_TYPE_key     ,
	_EVE_TYPE_repeat  ,
	_EVE_TYPE_last    ,
};

static bool          _flag_playing;

static float         _wait_tpb   ;
static float         _wait_tpb_c ;
//static unsigned char _wait1      ;
//static unsigned char _wait_count ;
static unsigned char _clock_count;

static unsigned int  _eve_num    ;
static unsigned int  _eve_idx    ;
static unsigned int  _eve_idx_repeat;

static PT4iEVENT    *_eves   = NULL;
static PT4iCHANNEL  *_chs    = NULL;
static float        *_pitchs = NULL;

static unsigned char _beat_tempo ;
static unsigned char _beat_divide;
static float         _buf_sec    ;

static float         _volume       ;
static float         _volume_master;
static float         _volume_speed ;

static char          _err_msg[ 32 ] = {0};


// manual GetTickCount() function
unsigned long long  *_tickQuad       = (unsigned long long *)(0x7FFE0320);
unsigned long       *_tickMultiplier = (unsigned long      *)(0x7FFE0004);
static unsigned long _tick_old = 0;
static unsigned long _GetTicks(){ return ( *_tickQuad * (unsigned long long)(*_tickMultiplier) >> 24 ); }
static float _GetPassSec( void )
{
	unsigned long tick_now = _GetTicks();
	float ret = (float)( tick_now - _tick_old ) / 1000;
	_tick_old = tick_now;
	return ret;
}

static void _SendError( const char *msg )
{
	memset( _err_msg, 0, sizeof(_err_msg) );
	strcpy( _err_msg, msg );
}

static bool _set_event( unsigned char ch, unsigned char type, unsigned short val )
{
	if( _eve_num >= MAX_EVENT ){ _SendError( "pti max event" ); return false; }
	
	_eves[ _eve_num ].ch   = ch  ;
	_eves[ _eve_num ].type = type;
	_eves[ _eve_num ].val  = val ;
	_eve_num++;
	return true;
}

static void _process_event( const PT4iEVENT *p_eve )
{
	PT4iCHANNEL *p_ch = &_chs[ p_eve->ch ];

	switch( p_eve->type )
	{
		case _EVE_TYPE_on:

			if( !( p_ch->flags & _CH_FLAG_on ) )
			{
				p_ch->flags |= _CH_FLAG_on;
				pxSound_Play( p_ch->vo_id, (p_ch->flags & _CH_FLAG_loop) );
			}
			p_ch->clock_count = (unsigned char)p_eve->val;
			break;

		case _EVE_TYPE_key   : pxSound_SetPitch( p_ch->vo_id, _pitchs[ p_eve->val ]      ); break;
		case _EVE_TYPE_vol   : p_ch->target_volume =            (float)p_eve->val / 128.0f; break;
		case _EVE_TYPE_wait  : _clock_count    =        (unsigned char)p_eve->val         ; break;
		case _EVE_TYPE_repeat: _eve_idx_repeat = _eve_idx; _clock_count = (unsigned char)p_eve->val; break;
		case _EVE_TYPE_last  : _eve_idx = _eve_idx_repeat; _clock_count = (unsigned char)p_eve->val; break;

		default: return;
	}
}

//-----------------------------------------------------------------------------------------

bool PT4i_Initialize( HWND hWnd, int ch_num, int sps, int bps, int smp_buf, bool b_dxsound )
{
	// zero..
	_flag_playing = false;
	_buf_sec      = ( (float)smp_buf / (float)sps );

	if( !pxSound_Initialize( hWnd, ch_num, sps, bps, b_dxsound ) ) return false;
	if( _eves && _chs && _pitchs ) return true;

	if( !(_eves   = (PT4iEVENT  *)malloc( sizeof(PT4iEVENT  ) * MAX_EVENT   )) ) return false;
	if( !(_chs    = (PT4iCHANNEL*)malloc( sizeof(PT4iCHANNEL) * MAX_CHANNEL )) ) return false;
	if( !(_pitchs = (float      *)malloc( sizeof(float      ) * MAX_PITCH   )) ) return false;
	memset( _eves  , 0,                   sizeof(PT4iEVENT  ) * MAX_EVENT   );
	memset( _chs   , 0,                   sizeof(PT4iCHANNEL) * MAX_CHANNEL );
	memset( _pitchs, 0,                   sizeof(float      ) * MAX_PITCH   );
	_eve_num       = 0;
	_tick_old      = _GetTicks();
	_volume_master = 1.0f;

	// pitch..
	{
		float pi;
		pi = 1.0f; for( int i = 40; i < MAX_PITCH; i++ ){ _pitchs[ i ] = pi; pi *= PITCH_ROUGH; }
		pi = 1.0f; for( int i = 40; i >=        0; i-- ){ _pitchs[ i ] = pi; pi /= PITCH_ROUGH; }
	}

	return true;
}

void PT4i_Release( void )
{
	if( _eves   ){ free( _eves   ); _eves   = NULL; }
	if( _chs    ){ free( _chs    ); _chs    = NULL; }
	if( _pitchs ){ free( _pitchs ); _pitchs = NULL; }
}

bool PT4i_Load( FILE *fp )
{
	bool b_ret = false;

	PT4i_Stop();
	_eve_num = 0;

	if( fread( &_beat_tempo , sizeof(unsigned char), 1, fp ) != 1 ) goto End;
	if( fread( &_beat_divide, sizeof(unsigned char), 1, fp ) != 1 ) goto End;

	// tempo -> wait.
//	_wait1 = (unsigned char)( ( _buf_sec * WAIT_ROUGH ) / ( _beat_tempo / 60 * _beat_divide) );

	_wait_tpb = (float)( ( _buf_sec * WAIT_ROUGH ) / ( (float)_beat_tempo * (float)_beat_divide ) );

	_chs[ 0 ].vo_id = 0; _chs[ 0 ].flags |= _CH_FLAG_loop;
	_chs[ 1 ].vo_id = 1; _chs[ 1 ].flags |= _CH_FLAG_loop;
	_chs[ 2 ].vo_id = 2; _chs[ 2 ].flags |= _CH_FLAG_loop;
	_chs[ 3 ].vo_id = 3; _chs[ 3 ].flags |= _CH_FLAG_loop;
	_chs[ 4 ].vo_id = 4; _chs[ 4 ].flags |=             0;
	_chs[ 5 ].vo_id = 5; _chs[ 5 ].flags |=             0;

	unsigned char kind;
	unsigned char val ;
	unsigned char ch  ;

	while( 1 )
	{
		if( fread( &ch  , sizeof(unsigned char), 1, fp ) != 1 ) break;
		if( fread( &kind, sizeof(unsigned char), 1, fp ) != 1 ) break;
		if( fread( &val , sizeof(unsigned char), 1, fp ) != 1 ) break;
		if( !_set_event( ch, kind, val ) ) goto End;
	}

	b_ret = true;
End:
	if( !b_ret ) _SendError( "pti read" );

	return b_ret;
}

bool PT4i_Start( void )
{
	if( _flag_playing ) return false;
	
//	_wait_count     = 0;
	_eve_idx        = 0;
	_eve_idx_repeat = 0;
	_clock_count    = 0;
	_wait_tpb_c     = 0;
	_flag_playing   = true;
	_volume         = _volume_master;
	_volume_speed   = 0;
	
	for( int i = 0; i < MAX_CHANNEL; i++ )
	{
		_chs[ i ].flags &= _CH_FLAG_reset_mask;
		_chs[ i ].target_volume = 1.0f;
		pxSound_SetVolume( _chs[ i ].vo_id, _chs[ i ].target_volume * _volume );
	}
	
	return true;
}

void PT4i_Stop( void )
{
	_flag_playing = false;
	
	for( int ch = 0; ch < MAX_CHANNEL; ch++ )
	{
		PT4iCHANNEL *p_ch = &_chs[ ch ];
		if( p_ch->flags & _CH_FLAG_on )
		{
			pxSound_Stop( p_ch->vo_id );
			p_ch->flags &= ~_CH_FLAG_on;
		}
	}
}

//static bool _bb = false;

bool PT4i_Procedure( void )
{
	if( !_flag_playing ) return false;

	_wait_tpb_c += _GetPassSec();

	if( _wait_tpb_c > _wait_tpb - 0.008f )
	{
		_wait_tpb_c -= _wait_tpb;
		if( _wait_tpb_c > _wait_tpb ) _wait_tpb_c = 0;
		
		if( !_clock_count )
		{
			while( _eve_idx < _eve_num )
			{
				_process_event( &_eves[ _eve_idx ] );
				_eve_idx++;
				if( _clock_count ) break;
			}
			if( _eve_idx >= _eve_num ) _flag_playing = false;
		}
		if( _clock_count ) _clock_count--;

		if( _volume_speed > 0 ){ _volume -= ( _volume_speed * _wait_tpb ); }

		for( int ch = 0; ch < MAX_CHANNEL; ch++ )
		{
			PT4iCHANNEL *p_ch = &_chs[ ch ];
			if( p_ch->flags & _CH_FLAG_on )
			{
				pxSound_SetVolume( _chs[ ch ].vo_id, _chs[ ch ].target_volume * _volume );
				if( p_ch->clock_count )
				{
					p_ch->clock_count--;
				}
				else
				{
					pxSound_Stop( p_ch->vo_id );
					p_ch->flags &= ~_CH_FLAG_on;
				}
			}
		}

		if( _volume <= 0.01f ) return false;

//		_wait_count = _wait1;
	}
//	_wait_count--;
	
	return true;
}

bool        PT4i_IsFinised     ( void ){ return !_flag_playing;  }
void        PT4i_SetFade       ( int msec ){ if( msec > 0 ) _volume_speed = _volume / ( (float)msec / 1000 ); }
int         PT4i_Get_NowEve    ( void ){ return _eve_idx;        }
int         PT4i_Get_RepeatMeas( void ){ return _eve_idx_repeat; }
const char *PT4i_Get_Error     ( void ){ return _err_msg;        }

void PT4i_SetVolume( float vol )
{
	_volume_master = vol;
	_volume_speed  = 0;

	for( int i = 0; i < MAX_CHANNEL; i++ )
	{
		PT4iCHANNEL *p_ch = &_chs[ i ];
		if( p_ch->flags & _CH_FLAG_on ) pxSound_SetVolume( p_ch->vo_id, p_ch->target_volume * _volume_master );
	}
}

void PT4i_Get_Information( int *p_beat_divide, float *p_beat_tempo, int *p_wait, int *p_meas_num )
{
	if( p_beat_divide ) *p_beat_divide = _beat_divide;
	if( p_beat_tempo  ) *p_beat_tempo  = (float)_beat_tempo;
	if( p_meas_num    ) *p_meas_num    = PT4i_Get_PlayMeas();
	if( p_wait        ) *p_wait        = (p_meas_num ? _eve_num / ( *p_meas_num * _beat_divide ) : 0);
}

int PT4i_Get_PlayMeas()
{
	if( _beat_divide == 0 ){ _SendError( "beat divide 0" ); return 0; }

	unsigned int total_ticks = 0;
	unsigned int meas        = 0;
	bool         has_loop    = false;

	for( unsigned int i = 0; i < _eve_num; i++ )
	{
		const PT4iEVENT *p_eve = &_eves[ i ];
		switch( p_eve->type )
		{
			case _EVE_TYPE_last  : has_loop = true;
			case _EVE_TYPE_wait  :
			case _EVE_TYPE_repeat: total_ticks += p_eve->val; break;
		}
	}

	meas = total_ticks / ( _beat_divide * 4 );
	if( !has_loop ) meas += 1;
	return meas;
}
#endif
