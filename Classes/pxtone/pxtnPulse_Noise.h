#ifndef pxtnPulse_Noise_H
#define pxtnPulse_Noise_H

#include "pxtnPulse_Frequency.h"
#include "../Fixture/DataDevice.h"
#include "pxStdDef.h"

enum WAVETYPE
{
	// version 0
	WAVETYPE_None   ,
	WAVETYPE_Sine   ,
	WAVETYPE_Saw    ,
	WAVETYPE_Rect   ,
	WAVETYPE_Random ,
	WAVETYPE_Saw2   ,
	WAVETYPE_Rect2  ,

	// version 1
	WAVETYPE_Tri    ,
	WAVETYPE_Random2,
	WAVETYPE_Rect3  ,
	WAVETYPE_Rect4  ,
	WAVETYPE_Rect8  ,
	WAVETYPE_Rect16 ,
	WAVETYPE_Saw3   ,
	WAVETYPE_Saw4   ,
	WAVETYPE_Saw6   ,
	WAVETYPE_Saw8   ,

	WAVETYPE_Num,
};

typedef struct
{
	WAVETYPE type  ;
	f32      freq  ;
	f32      volume;
	f32      offset;
	b8       b_rev ;
}
ptnOSCILLATOR;

typedef struct
{
	b8            b_use   ;
	s32           enve_num;
	pxPOINT      *enves   ;
	s32           pan     ;
	ptnOSCILLATOR main    ;
	ptnOSCILLATOR freq    ;
	ptnOSCILLATOR volu    ;
}
ptnUNIT;

typedef struct
{
	s32     smp_num_44k;
	s32     unit_num   ;
	ptnUNIT *units     ;
}
ptnDESIGN;

b32  pxtnNoise_Initialize    ( void );
void pxtnNoise_Release       ( void );
b32  pxtnNoise_Allocate      (              ptnDESIGN *p_design, s32 unit_num, s32 envelope_num );
b32  pxtnNoise_CopyFrom      ( ptnDESIGN *p_dst, const ptnDESIGN *p_src    );
b32  pxtnNoise_Design_Write  ( FILE *fp, s32 *p_add,   ptnDESIGN *p_design );
b32  pxtnNoise_Design_Read   ( DDV *p_read, ptnDESIGN *p_design, b32 *pbNew );
s32  pxtnNoise_SamplingSize  (        const ptnDESIGN *p_design, s32 channel_num, s32 sps, s32 bps );
b32  pxtnNoise_Build         ( u8 **pp_smp, ptnDESIGN *p_design, s32 channel_num, s32 sps, s32 bps );
void pxtnNoise_Design_Release( ptnDESIGN *p );

#endif
