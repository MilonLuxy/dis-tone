#ifndef pxtnWoice_H
#define pxtnWoice_H

#include "pxtnPulse_Noise.h"
#include "pxtnPulse_Oggv.h"
#include "pxtnPulse_PCM.h"

#define PTV_VOICEFLAG_WAVELOOP   0x00000001
#define PTV_VOICEFLAG_SMOOTH     0x00000002
#define PTV_VOICEFLAG_BEATFIT    0x00000004
#define PTV_VOICEFLAG_UNCOVERED  0xfffffff8

#define PTV_DATAFLAG_WAVE        0x00000001
#define PTV_DATAFLAG_ENVELOPE    0x00000002
#define PTV_DATAFLAG_UNCOVERED   0xfffffffc

enum WOICETYPE
{
	WOICE_None,
	WOICE_PCM ,
	WOICE_PTV ,
	WOICE_PTN ,
	WOICE_OGGV,
};

enum VOICETYPE
{
	VOICETYPE_Coodinate,
	VOICETYPE_Overtone ,
	VOICETYPE_Noise    ,
	VOICETYPE_Sampling ,
	VOICETYPE_OggVorbis,
};

typedef struct
{
	s32  smp_head_w ;
	s32  smp_body_w ;
	s32  smp_tail_w ;
	u8  *p_smp_w;

	u8  *p_env  ;
	s32  env_size   ;
	s32  env_release;

//	b8  b_sine_over; // later versions only
}
ptvINSTANCE;

typedef struct
{
	s32      fps     ;
	s32      head_num;
	s32      body_num;
	s32      tail_num;
	pxPOINT *points  ;
}
ptvENVELOPE;

typedef struct
{
	s32      num   ;
	s32      reso  ; // COODINATE_RESOLUTION
	pxPOINT *points;
}
ptvWAVE;

typedef struct
{
	long          basic_key  ;
	long          volume     ;
	long          pan        ;
	float         tuning     ;
	unsigned long voice_flags;
	unsigned long data_flags ;

	VOICETYPE     type       ;
	pcmFORMAT    *p_pcm      ;
	ptnDESIGN    *p_ptn      ;
#ifdef  pxINCLUDE_OGGVORBIS
	oggvUNIT     *p_oggv     ;
#endif

	ptvWAVE       wave       ;
	ptvENVELOPE   envelope   ;
}
ptvUNIT;

typedef struct
{
	f64 smp_pos    ;       
	f32 offset_freq;
	s32 env_volume ;
	s32 life_count ;
	s32 on_count   ;

	s32 smp_count  ;
	s32 env_start  ;
	s32 env_pos    ;
	s32 env_release_clock;

	s32 smooth_volume;
}
ptvTONE;

// (44byte) =================
typedef struct WOICE_STRUCT
{
	long         _voice_num    ;

	char         _name[ 16 ]   ;
	long         _name_size    ;

	WOICETYPE    _type         ;
	ptvUNIT     *_voices       ;
	ptvINSTANCE *_voinsts      ;

	float        _x3x_tuning   ;
	long         _x3x_basic_key; // tuning old-fmt when key-event
};


b32           pxtnWoice_Initialize    ( s32 woice_num );
b32           pxtnWoice_Remove        ( s32 index );
void          pxtnWoice_RemoveAll     ( void );
void          pxtnWoice_Release       ( void );
b32           pxtnWoice_Voice_Allocate( WOICE_STRUCT *p_voice, s32 voice_num );
void          pxtnWoice_Voice_Init    ( ptvUNIT *p_vc, VOICETYPE type );
s32           pxtnWoice_Add           ( void );
s32           pxtnWoice_Read          ( const char *path, b32 pbNew, WOICE_STRUCT *p_voice );
s32           pxtnWoice_Duplicate     ( const WOICE_STRUCT *p_voice );
void          pxtnWoice_BuildPTV      (       WOICE_STRUCT *p_voice, s32 smp_size, s32 ch_num, s32 sps, s32 bps );
b32           pxtnWoice_Tone_Ready    ( s32 index );
s32           pxtnWoice_Count         ( void );
b32           pxtnWoice_ReadyTone     ( void );
s32           pxtnWoice_Get_NumMax    ( void );
void          pxtnWoice_Replace       ( s32 old_place, s32 new_place );
WOICE_STRUCT *pxtnWoice_GetVariable   ( s32 index );
WOICE_STRUCT *pxtnWoice_Create        ( void );

b32           pxtnWoicePTV_Write      ( FILE *fp, s32 *p_total, WOICE_STRUCT *p_voice );
b32           pxtnWoicePTV_OpenWrite  ( const char *file_name , WOICE_STRUCT *p_voice );
b32           pxtnWoicePTV_Read       ( DDV *p_read,            WOICE_STRUCT *p_voice, b32 *pbNew );

#endif
