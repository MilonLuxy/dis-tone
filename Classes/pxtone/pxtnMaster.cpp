#include <StdAfx.h>


#include "pxtnMaster.h"
#include "pxtnEvelist.h"

static s32 _beat_num    = 0;
static f32 _beat_tempo  = 0;
static s32 _beat_clock  = 0;
static s32 _meas_num    = 0;
static s32 _repeat_meas = 0;
static s32 _last_meas   = 0;
static s32 _volume_     = 0;


void pxtnMaster_Reset( void )
{
	_beat_num    = EVENTDEFAULT_BEATNUM  ;
	_beat_tempo  = EVENTDEFAULT_BEATTEMPO;
	_beat_clock  = EVENTDEFAULT_BEATCLOCK;
	_meas_num    = 1;
	_repeat_meas = 0;
	_last_meas   = 0;
}

void pxtnMaster_Set( s32    beat_num, f32    beat_tempo, s32    beat_clock )
{
	_beat_num   = beat_num;
	_beat_tempo = beat_tempo;
	_beat_clock = beat_clock;
}
void pxtnMaster_Get( s32 *p_beat_num, f32 *p_beat_tempo, s32 *p_beat_clock )
{
	if( p_beat_num   ) *p_beat_num   = _beat_num  ;
	if( p_beat_tempo ) *p_beat_tempo = _beat_tempo;
	if( p_beat_clock ) *p_beat_clock = _beat_clock;
}

s32 pxtnMaster_Get_BeatNum   (){ return _beat_num   ;}
f32 pxtnMaster_Get_BeatTempo (){ return _beat_tempo ;}
s32 pxtnMaster_Get_BeatClock (){ return _beat_clock ;}
s32 pxtnMaster_Get_MeasNum   (){ return _meas_num   ;}
s32 pxtnMaster_Get_RepeatMeas(){ return _repeat_meas;}
s32 pxtnMaster_Get_LastMeas  (){ return _last_meas  ;}
s32 pxtnMaster_Get_LastClock (){ return _last_meas * _beat_clock * _beat_num; }
s32 pxtnMaster_Get_PlayMeas  (){ return ( _last_meas ) ? _last_meas : _meas_num; }
s32 pxtnMaster_Get_ThisClock ( s32 meas, s32 beat, s32 clock )
{
	return _beat_num * _beat_clock * meas + _beat_clock * beat + clock;
}

void pxtnMaster_Adjust_MeasNum( s32 clock )
{
	s32 b_num = ( clock + _beat_clock  - 1 ) / _beat_clock;
	s32 m_num = ( b_num + _beat_num    - 1 ) / _beat_num;
	if( _meas_num    <= m_num     ) _meas_num    = m_num;
	if( _repeat_meas >= _meas_num ) _repeat_meas = 0;
	if( _last_meas   >  _meas_num ) _last_meas   = _meas_num;
}
void pxtnMaster_Set_MeasNum( s32 meas_num )
{
	if( meas_num < 1             ) meas_num = 1;
	if( meas_num <= _repeat_meas ) meas_num = _repeat_meas + 1;
	if( meas_num <  _last_meas   ) meas_num = _last_meas;
	_meas_num = meas_num;
}

void pxtnMaster_Set_RepeatMeas( s32 meas ){ if( meas < 0 ) meas = 0; _repeat_meas = meas; }
void pxtnMaster_Set_LastMeas  ( s32 meas ){ if( meas < 0 ) meas = 0; _last_meas   = meas; }
