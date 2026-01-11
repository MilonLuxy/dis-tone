#pragma once
#include "pxtnWoice.h"

// (696byte) ================
typedef struct TUNEUNITTONESTRUCT
{
	b32  _bOperated;
	b32  _bPlayed  ;
	char _name_buf[ 16 ];
	s32  _name_size;

	//	TUNEUNITTONESTRUCT
	s32  _key_now   ;
	s32  _key_start ;
	s32  _key_margin;
	s32  _portament_sample_pos;
	s32  _portament_sample_num;
	s32  _pan_vols     [ 2 ];
	s32  _pan_times    [ 2 ];
	s32  _pan_time_bufs[ 2 ][ 64 ];
	s32  _v_VOLUME  ;
	s32  _v_VELOCITY;
	s32  _v_GROUPNO ;
	f32  _v_TUNING  ;

	const WOICE_STRUCT *_p_woice;

	ptvTONE _vts[ 2 ];
};


b32  pxtnUnit_Initialize( s32 unit_num );
void pxtnUnit_RemoveAll ( void );
void pxtnUnit_Release   ( void );
s32  pxtnUnit_Count     ( void );
s32  pxtnUnit_Get_NumMax( void );
TUNEUNITTONESTRUCT *pxtnUnit_GetVariable( s32 index );
b32  pxtnUnit_AddNew    ( void );