#include <StdAfx.h>


#include "../Fixture/pxMem.h"

#include "pxtnGroup.h"
#include "pxtnStrain.h"


static STRAIN_STRUCT *_strn_main = NULL;
static s32            _strn_num  = 0;

////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _Remove( STRAIN_STRUCT *p_strn )
{
	p_strn->_b_played = _false;
	p_strn->_group    =      0;
	p_strn->_amp      =    1.0;
	for( s32 g = 0; g < GATE_NUM; g++ ) p_strn->_gates[ g ] = 100.0;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

b32 pxtnStrain_Initialize( s32 strain_num )
{
	if( !pxMem_zero_alloc( (void**)&_strn_main, strain_num * sizeof(STRAIN_STRUCT) ) ) return _false;
	_strn_num = strain_num;
	pxtnStrain_RemoveAll();
	return _true;
}

void pxtnStrain_Release( void ){ pxMem_free( (void**)&_strn_main ); }

void pxtnStrain_Get_Values( s32 idx, s32 *p_group, s32 *p_gates, f64 *p_amp )
{
	if( p_group ) *p_group = _strn_main[ idx ]._group;
	if( p_amp   ) *p_amp   = _strn_main[ idx ]._amp  ;

	for( s32 g = 0; g < GATE_NUM; g++ ){ if( p_gates ) p_gates[ g ] = _strn_main[ idx ]._gates[ g ]; }
}

void pxtnStrain_Set_Values( s32 idx, s32 group, s32 *gates, f64 amp )
{
	_strn_main[ idx ]._group = group;
	_strn_main[ idx ]._amp   = amp  ;

	for( s32 g = 0; g < GATE_NUM; g++ ){ _strn_main[ idx ]._gates[ g ] = (f64)gates[ g ]; }
}

b32 pxtnStrain_Add( s32 group, s32 *gates, f64 amp )
{
	s32 index;
	for( index = 0; index < _strn_num; index++ ){ if( !_strn_main[ index ]._b_played ) break; }
	if( index == _strn_num ) return _false;

	s32 max_group = pxtnGroup_Get();
	if( group >= max_group ){ group = max_group - 1; }

	_strn_main[ index ]._b_played = _true;
	_strn_main[ index ]._group    = group;
	_strn_main[ index ]._amp      = amp  ;

	for( s32 g = 0; g < GATE_NUM; g++ ){ _strn_main[ index ]._gates[ g ] = (f64)gates[ g ]; }

	return _true;
}

void pxtnStrain_RemoveAll( void ){ if( _strn_main ){ for( s32 s = 0; s < _strn_num; s++ ) _Remove( &_strn_main[ s ] ); } }

b32  pxtnStrain_IsValid( s32 index )
{
	if( _strn_main[ index ]._amp < 0.0 || _strn_main[ index ]._amp > 10.0 ) return _false;

	for( s32 g = 0; g < GATE_NUM; g++ )
	{
		if( _strn_main[ index ]._gates[ g ] < 0 || _strn_main[ index ]._gates[ g ] > 100.0 ) return _false;
	}
	return _true;
}

void pxtnStrain_Tone_Supple( s32 index, s32 *group_smps )
{
	if( pxtnStrain_IsValid( index ) )
	{
		group_smps[ _strn_main[ index ]._group ] *= _strn_main[ index ]._amp;
		pxMem_cap( &group_smps[ _strn_main[ index ]._group ], MAX_S16BIT, -MAX_S16BIT );
		group_smps[ _strn_main[ index ]._group ] /= _strn_main[ index ]._amp;
	}
}

STRAIN_STRUCT *pxtnStrain_GetVariable( s32 index ){ return &_strn_main[ index ]; }

s32 pxtnStrain_Get_NumMax( void ){ return _strn_num; }
