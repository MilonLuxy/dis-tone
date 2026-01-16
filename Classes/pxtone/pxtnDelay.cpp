#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "../Streaming/Streaming.h"
#include "pxtnDelay.h"
#include "pxtnGroup.h"
#include "pxtnMaster.h"


static DELAYSTRUCT *_delays   = NULL;
static s32         _delay_num =   0 ;

////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _Tone_Release( DELAYSTRUCT *delay )
{
	pxMem_free( (void**)&delay->_bufs[ 0 ] );
	pxMem_free( (void**)&delay->_bufs[ 1 ] );
	delay->_smp_num = 0;
	delay->_offset  = 0;
}

static void _Remove( s32 index )
{
	if( index < 0 || index >= _delay_num ) return;

	if( _delays[ index ]._b_played )
	{
		_Tone_Release( &_delays[ index ] );
		memset( &_delays[ index ], 0, sizeof(DELAYSTRUCT) );
	}
}

static b32 _Tone_Ready( DELAYSTRUCT *delay )
{
	_Tone_Release( delay );

	if( !delay->_freq || !delay->_rate ) return _true;
	
	b32 b_ret = _false;
	
	s32 beat_num, sps; f32 beat_tempo;
	pxtnMaster_Get( &beat_num, &beat_tempo, NULL );
	Streaming_Get_SampleInfo( NULL, (s32*)&sps, NULL );

	delay->_offset   = 0;
	delay->_rate_s32 = (s32)delay->_rate; // /100;

	switch( delay->_unit )
	{
		case DELAYUNIT_Beat  : delay->_smp_num = (s32)( sps * 60            / beat_tempo / delay->_freq ); break;
		case DELAYUNIT_Meas  : delay->_smp_num = (s32)( sps * 60 * beat_num / beat_tempo / delay->_freq ); break;
		case DELAYUNIT_Second: delay->_smp_num = (s32)( sps                              / delay->_freq ); break;
	}

	for( s32 c = 0; c < 2; c++ )
	{
		if( !pxMem_zero_alloc( (void**)&delay->_bufs[ c ], delay->_smp_num * sizeof(s32) ) ) goto End;
	}
	
	b_ret = _true;
End:
	if( !b_ret ) _Tone_Release( delay );

	return b_ret;
}

////////////////////////////
// グローバル関数 //////////
////////////////////////////

b32 pxtnDelay_Initialize( s32 delay_num )
{
	if( !pxMem_zero_alloc( (void**)&_delays, sizeof(DELAYSTRUCT) * delay_num ) ) return _false;
	_delay_num = delay_num;
	return _true;
}

void pxtnDelay_Release( void )
{
	if( _delays )
	{
		for( s32 i = 0; i < _delay_num; i++ ){ if( _delays[ i ]._b_played ) _Tone_Release( &_delays[ i ] ); }
		pxMem_free( (void**)&_delays );
		_delay_num = 0;
	}
}

void pxtnDelay_Tone_Clear( void )
{
	s32 bps; Streaming_Get_SampleInfo( NULL, NULL, (s32*)&bps );

	for( s32 i = 0; i < _delay_num; i++ )
	{
		DELAYSTRUCT *p_dela = &_delays[ i ];
		if( p_dela->_smp_num )
		{
			if( p_dela->_bufs[ 0 ] ) memset( p_dela->_bufs[ 0 ], 0, p_dela->_smp_num * sizeof(s32) );
			if( p_dela->_bufs[ 1 ] ) memset( p_dela->_bufs[ 1 ], 0, p_dela->_smp_num * sizeof(s32) );
		}
	}
}

s32 pxtnDelay_Add( DELAYUNIT unit, f32 freq, f32 rate, s32 group )
{
	if( group >= pxtnGroup_Get() ) group = 0;

	s32 index;
	for( index = 0; index < _delay_num; index++ )
	{
		if( !_delays[ index ]._b_played ) break;
	}
	if( index == _delay_num ) return -1;

	DELAYSTRUCT *delay = &_delays[ index ];
	delay->_b_played  = _true;
	delay->_b_enabled = _true;
	delay->_unit      = unit;
	delay->_freq      = freq;
	delay->_rate      = rate;
	delay->_group     = group;

	return index;
}

s32 pxtnDelay_Count( void )
{
	s32 count = 0;
	for( s32 i = 0; i < _delay_num; i++ ){ if( _delays[ i ]._b_played ) count++; }
	return count;
}

void pxtnDelay_RemoveAll( void ){ for( s32 i = 0; i < _delay_num; i++ ) _Remove( i ); }

b32 pxtnDelay_ReadyTone( void )
{
	b32 b_ret = _false;
	for( s32 i = 0; i < _delay_num; i++ ){ if( _delays[ i ]._b_played ){ if( !_Tone_Ready( &_delays[ i ] ) ) goto End; } }
	b_ret = _true;
End:
	if( !b_ret ) pxtnDelay_RemoveAll();

	return b_ret;
}

DELAYSTRUCT *pxtnDelay_GetVariable( s32 index )
{
	if( index < 0 || index >= _delay_num ) return NULL;
	return &_delays[ index ];
}

s32 pxtnDelay_Get_NumMax( void ){ return _delay_num; }
