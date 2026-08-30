#pragma once
#include "pxStdDef.h"

#define GATE_NUM    2

// (48byte) =================
typedef struct STRAIN_STRUCT
{
	b32  _b_played;
	s32  _group   ;
	f64  _gates[ GATE_NUM ];
	f64  _amp     ;
	s32  _cut_16bit[ GATE_NUM ];
	s32  _amp_16bit[ GATE_NUM ];
};

b32  pxtnStrain_Initialize           ( s32 strain_num );
void pxtnStrain_Release              ( void );
void pxtnStrain_Get_Values           ( s32 idx, s32 *p_group, s32 *p_gates, f64 *p_amp );
void pxtnStrain_Set_Values           ( s32 idx, s32    group, s32 *  gates, f64    amp );
b32  pxtnStrain_Add                  (          s32    group, s32 *  gates, f64    amp );
void pxtnStrain_RemoveAll            ( void );
s32  pxtnStrain_Count                ( void );
void pxtnStrain_ReadyTone            ( void );
b32  pxtnStrain_IsValid              ( s32 index );
void pxtnStrain_Tone_Supple          ( s32 index, s32 *group_smps );
STRAIN_STRUCT *pxtnStrain_GetVariable( s32 index );
s32  pxtnStrain_Get_NumMax           ( void );