#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "../Streaming/Streaming.h"
#include "pxtnGroup.h"
#include "pxtnOverDrive.h"


static OVERDRIVE_STRUCT *_overdrives = NULL;
static s32               _odrv_num   = 0;

////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _Remove( s32 index )
{
	if( index < 0 || index >= _odrv_num ) return;

	if( _overdrives[ index ]._b_played )
	{
		
		memset( &_overdrives[ index ], 0, sizeof(OVERDRIVE_STRUCT) );
	}
}

static void _Tone_Ready( OVERDRIVE_STRUCT *overdrive )
{
//	if( !overdrive || overdrive->_amp > 0 || overdrive->_cut > 0 ) return;

//	s32 sps; Streaming_Get_SampleInfo( NULL, &sps, NULL );
//	for( s32 i = 0; i < 2; i++ )
	{
		overdrive->_cut_16bit_top = (s32)( MAX_S16BIT * ( 100 - overdrive->_cut ) / 100 );
	}
}

////////////////////////////
// グローバル関数 //////////
////////////////////////////

b32 pxtnOverDrive_Initialize( s32 overdrive_num )
{
	if( !pxMem_zero_alloc( (void**)&_overdrives, overdrive_num * sizeof(OVERDRIVE_STRUCT) ) ) return _false;
	_odrv_num = overdrive_num;
	return _true;
}

void pxtnOverDrive_Release( void )
{



	pxMem_free( (void**)&_overdrives );
	_odrv_num = 0;

}

void pxtnOverDrive_Tone_Clear( void )
{


	for( s32 i = 0; i < _odrv_num; i++ )
	{

		if( _overdrives[ i ]._b_played )
		{
			for( s32 j = 0; j < 2; j++ ){}

		}
	}
}

s32 pxtnOverDrive_Add( f32 cut, f32 amp, s32 group )
{
	if( group >= pxtnGroup_Get() ) group = 0;

	s32 index;
	for( index = 0; index < _odrv_num; index++ )
	{
		if( !_overdrives[ index ]._b_played ) break;
	}
	if( index == _odrv_num ) return -1;
	
	OVERDRIVE_STRUCT *p_ovdrv = &_overdrives[ index ];
	p_ovdrv->_b_played  = _true;
	p_ovdrv->_b_enabled = _true;

	p_ovdrv->_cut       = cut;
	p_ovdrv->_amp       = amp;
	p_ovdrv->_group     = group;

	return index;
}

s32 pxtnOverDrive_Count( void )
{
	s32 count = 0;
	for( s32 i = 0; i < _odrv_num; i++ ){ if( _overdrives[ i ]._b_played ) count++; }
	return count;
}

void pxtnOverDrive_RemoveAll( void ){ for( s32 i = 0; i < _odrv_num; i++ ) _Remove( i ); }

void pxtnOverDrive_ReadyTone( void )
{

	for( s32 i = 0; i < _odrv_num; i++ ){ if( _overdrives[ i ]._b_played ) _Tone_Ready( &_overdrives[ i ] ); }





}

OVERDRIVE_STRUCT *pxtnOverDrive_GetVariable( s32 index )
{
	if( index < 0 || index >= _odrv_num ) return NULL;
	return &_overdrives[ index ];
}

s32 pxtnOverDrive_Get_NumMax( void ){ return _odrv_num; }
