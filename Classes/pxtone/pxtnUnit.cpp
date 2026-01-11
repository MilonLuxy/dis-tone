#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "pxtnUnit.h"


static s32                  _unit_num  =   0 ;
static TUNEUNITTONESTRUCT **_unit_main = NULL;

////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static b32 _Remove( s32 index )
{
	if( index < 0 || index >= _unit_num ) return _false;

	pxMem_free( (void**)&_unit_main[ index ] );
	_unit_main[ index ] = NULL;
	return _true;
}

static void _SetOpratedAll( b32 b )
{
	for( s32 u = 0; u < _unit_num; u++ )
	{
		if( _unit_main[ u ] )
		{
			_unit_main[ u ]->_bOperated = b;
			if( b ) _unit_main[ u ]->_bPlayed = _true;
		}
	}
}

static void _Solo( s32 index )
{
	for( s32 u = 0; u < _unit_num; u++ )
	{
		if( _unit_main[ u ] ) _unit_main[ u ]->_bPlayed = ( u == index ) ? _true : _false;
	}
}

static void _Replace( s32 old_place, s32 new_place )
{
	TUNEUNITTONESTRUCT *p_u = _unit_main[ old_place ];
	s32 max_place = pxtnUnit_Count() - 1;

	if( new_place >  max_place ) new_place = max_place;
	if( new_place == old_place ) return;

	if( old_place < new_place )
	{
		for( s32 w = old_place; w < new_place; w++ ){ if( _unit_main[ w ] ) _unit_main[ w ] = _unit_main[ w + 1 ]; }
	}
	else
	{
		for( s32 w = old_place; w > new_place; w-- ){ if( _unit_main[ w ] ) _unit_main[ w ] = _unit_main[ w - 1 ]; }
	}
	_unit_main[ new_place ] = p_u;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

b32 pxtnUnit_Initialize( s32 unit_num )
{
	if( !pxMem_zero_alloc( (void**)&_unit_main, unit_num * sizeof(TUNEUNITTONESTRUCT*) ) ) return _false;
	_unit_num = unit_num;
	return _true;
}

void pxtnUnit_RemoveAll( void ){ for( s32 i = 0; i < _unit_num; i++ ) _Remove( i ); }

void pxtnUnit_Release( void )
{
	pxtnUnit_RemoveAll();
	pxMem_free( (void**)&_unit_main );
	_unit_num = 0;
}

s32 pxtnUnit_Count( void )
{
	s32 count = 0;
	for( s32 i = 0; i < _unit_num; i++ ){ if( _unit_main[ i ] ) count++; }
	return count;
}

s32 pxtnUnit_Get_NumMax( void ){ return _unit_num; }

TUNEUNITTONESTRUCT *pxtnUnit_GetVariable( s32 index )
{
	if( index < 0 || index >= _unit_num ) return NULL;
	return _unit_main[ index ];
}

b32 pxtnUnit_AddNew( void )
{
	s32 i;
	for( i = 0; i < _unit_num; i++ ){ if( !_unit_main[ i ] ) break; }
	if( i == _unit_num ) return _false;

	if( !pxMem_zero_alloc( (void**)&_unit_main[ i ], sizeof(TUNEUNITTONESTRUCT) ) ) return _false;
	_unit_main[ i ]->_bPlayed = _true;
	return _true;
}
