#pragma once
#include "pxStdDef.h"

// (24byte) =================
typedef struct OVERDRIVE_STRUCT
{
	b8  _b_played     ;
	b8  _b_enabled    ;
	s32 _group        ;
	f32 _cut          ;
	f32 _amp          ;
	s32 _cut_16bit_top;
	s32 _amp_16bit_top;
};

b32  pxtnOverDrive_Initialize              ( s32 overdrive_num );
void pxtnOverDrive_Release                 ( void );
void pxtnOverDrive_Tone_Clear              ( void );
s32  pxtnOverDrive_Add                     ( f32 cut, f32 amp, s32 group );
s32  pxtnOverDrive_Count                   ( void );
void pxtnOverDrive_RemoveAll               ( void );
void pxtnOverDrive_ReadyTone               ( void );
OVERDRIVE_STRUCT *pxtnOverDrive_GetVariable( s32 index );
s32  pxtnOverDrive_Get_NumMax              ( void );