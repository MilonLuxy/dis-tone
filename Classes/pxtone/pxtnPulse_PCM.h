#pragma once
#include "pxStdDef.h"

typedef struct
{
	s32  _ch      ;
	s32  _sps     ;
	s32  _bps     ;
	s32  _smp_head; // no use. 0
	s32  _smp_body;
	s32  _smp_tail; // no use. 0
	u8  *_p_smp;
}
pcmFORMAT;

b32 pxtnPCM_Read    ( const char *file_name,   pcmFORMAT *p_pcm );
b32 pxtnPCM_Write   ( const char *file_name,   pcmFORMAT *p_pcm, const char *pstrLIST );
b32 pxtnPCM_Create  ( pcmFORMAT  *p_pcm, s32     ch, s32     sps, s32     bps, s32 sample_num );
b32 pxtnPCM_Convert ( pcmFORMAT  *p_pcm, s32 new_ch, s32 new_sps, s32 new_bps );
b32 pxtnPCM_CopyFrom( pcmFORMAT  *p_src, const pcmFORMAT *p_dst );
b32 pxtnPCM_Copy    ( pcmFORMAT  *p_src,       pcmFORMAT *p_dst, s32 start, s32 end );