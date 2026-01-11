#include "pxStdDef.h"

enum DELAYUNIT
{
	DELAYUNIT_Beat = 0,
	DELAYUNIT_Meas    ,
	DELAYUNIT_Second  ,
	DELAYUNIT_num     ,
};

// (40byte) =================
typedef struct DELAYSTRUCT
{
	b8        _b_played ;
	b8        _b_enabled;
	
	DELAYUNIT _unit     ;
	s32       _group    ;
	f32       _freq     ;
	f32       _rate     ;

	s32       _smp_num  ;
	s32       _offset   ;
	s32      *_bufs[ 2 ];
	s32       _rate_s32 ;
};

b32  pxtnDelay_Initialize         ( s32 delay_num );
void pxtnDelay_Release            ( void );
void pxtnDelay_Tone_Clear         ( void );
s32  pxtnDelay_Add                ( DELAYUNIT unit, f32 freq, f32 rate, s32 group );
s32  pxtnDelay_Count              ( void );
void pxtnDelay_RemoveAll          ( void );
b32  pxtnDelay_ReadyTone          ( void );
DELAYSTRUCT *pxtnDelay_GetVariable( s32 index );
s32  pxtnDelay_Get_NumMax         ( void );