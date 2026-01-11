#include <StdAfx.h>


#include "pxtnDelay.h"
#include "pxtnEvelist.h"
#include "pxtnGroup.h"
#include "pxtnMaster.h"
#include "pxtnPulse_Noise.h"
#include "pxtnText.h"
#include "pxtnIO.h"
#pragma comment( lib,"version" )


// リソースバージョンを取得
u16 pxtnIO_GetCompileVersion( void )
{
	void             *p    = NULL;
	DWORD             size =    0;
	VS_FIXEDFILEINFO *info = NULL;

	char path[ MAX_PATH ]; GetModuleFileName( NULL, path, MAX_PATH );
	if( !(size = GetFileVersionInfoSize( path, &size )        ) ) goto End;
	if( !(p = malloc( size )                                  ) ) goto End;
	if( !GetFileVersionInfo( path, 0, size, p )                 ) goto End;
	if( !VerQueryValue( p, "\\", (LPVOID*)&info, (PUINT)&size ) ) goto End;

	size =  HIWORD( info->dwFileVersionMS ) * 1000 + LOWORD( info->dwFileVersionMS ) * 100 +
			HIWORD( info->dwFileVersionLS ) *   10 + LOWORD( info->dwFileVersionLS );
End:
	if( p ) free( p );
	return (u16)size;
}


#define TUNEOVERDRIVE_CUT_MAX      99.9f
#define TUNEOVERDRIVE_CUT_MIN      50.0f
#define TUNEOVERDRIVE_AMP_MAX       8.0f
#define TUNEOVERDRIVE_AMP_MIN       0.1f
#define _MAX_PROJECTNAME_x1x          16
#define _MAX_TUNEUNITNAME             16

////////////////////////////////////////
// Structs            //////////////////
////////////////////////////////////////

typedef struct
{
	u16  unit_index;
	u16  rrr;
	char name[ _MAX_TUNEUNITNAME ];
}
_ASSIST_UNIT;

typedef struct
{
	s16  num;
	s16  rrr;
}
_NUM_UNIT;

typedef struct
{
	u16  woice_index;
	u16  rrr;
	char name[ _MAX_TUNEUNITNAME ];
}
_ASSIST_WOICE;

// 24byte =================
typedef struct
{
	u16  x3x_unit_no;
	u16  basic_key  ;
	u32  voice_flags;
	u16  ch         ;
	u16  bps        ;
	u32  sps        ;
	f32  tuning     ;
	u32  data_size  ;
}
_MATERIALSTRUCT_PCM;

// 16byte =================
typedef struct
{
	u16  x3x_unit_no;
	u16  basic_key  ;
	u32  voice_flags;
	f32  tuning     ;
	s32  rrr        ; // 0: -v.0.9.2.3
	                  // 1:  v.0.9.2.4-
}
_MATERIALSTRUCT_PTN;

// 24byte =================
typedef struct
{
	u16  x3x_unit_no;
	u16  rrr        ;
	f32  x3x_tuning ;
	s32  size       ;
}
_MATERIALSTRUCT_PTV;

#ifdef pxINCLUDE_OGGVORBIS
// 16byte =================
typedef struct
{
	u16  xxx        ; //ch;
	u16  basic_key  ;
	u32  voice_flags;
	f32  tuning     ;
}
_MATERIALSTRUCT_OGGV;
#endif

// (12byte) =================
typedef struct
{
	u16  unit ;
	u16  group;
	f32  rate ;
	f32  freq ;
}
_EFFECT_DELAY;

// (8byte) =================
typedef struct
{
	u16  xxx  ;
	u16  group;
	f32  cut  ;
	f32  amp  ;
	f32  yyy  ;
}
_EFFECT_OVERDRIVE;

// master info(8byte) ================
typedef struct
{
	u16  data_num ; // data-num is 3 ( clock / status / volume ）
	u16  rrr      ;
	u32  event_num;
}
_x4x_MASTER;

typedef struct
{
	u16  type ;
	u16  group;
}
_x3x_UNIT;

// v1x (20byte) ================= 
typedef struct
{
	char name[ _MAX_PROJECTNAME_x1x ];
	u16  type ;
	u16  group;
}
_x1x_UNIT;

// event struct(12byte) =================
typedef struct
{
	u16  unit_index;
	u16  event_kind;
	u16  data_num  ; // １イベントのデータ数。現在は 2 ( clock / volume ）
	u16  rrr       ;
	u32  event_num ;
}
_x4x_EVENTSTRUCT;

// プロジェクト情報(36byte) ================
typedef struct
{
	char x1x_name[ _MAX_PROJECTNAME_x1x ];

	f32  x1x_beat_tempo   ;
	u16  x1x_quarter_clock;
	u16  x1x_beat_num     ;
	u16  x1x_beat_note    ;
	u16  x1x_meas_num     ;

	u16  x1x_channel_num;
	u16  x1x_bps;
	u32  x1x_sps;
}
_x1x_PROJECT;



////////////////////////////////////////
// Write              //////////////////
////////////////////////////////////////

b32 pxtnIO_assiUNIT_Write( FILE *fp, TUNEUNITTONESTRUCT *p_u, s32 idx )
{
	_ASSIST_UNIT assi = {0};
	memcpy( assi.name, p_u[ idx ]._name_buf, p_u[ idx ]._name_size );
	assi.unit_index = (u16)idx;

	s32 size = sizeof(_ASSIST_UNIT);
	if( fwrite( &size, sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &assi, size       , 1, fp ) != 1 ) return _false;
	return _true;
}

b32 pxtnIO_num_UNIT_Write( FILE *fp, s32 num )
{
	_NUM_UNIT data = {0};
	data.num = (s16)num;

	s32 size = sizeof(_NUM_UNIT);
	if( fwrite( &size, sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &data, size       , 1, fp ) != 1 ) return _false;
	return _true;
}

b32 pxtnIO_assiWOIC_Write( FILE *fp, WOICE_STRUCT *p_w, s32 idx )
{
	_ASSIST_WOICE assi = {0};
	memcpy( assi.name, p_w[ idx ]._name, p_w[ idx ]._name_size );
	assi.woice_index = (u16)idx;

	s32 size = sizeof(_ASSIST_WOICE);
	if( fwrite( &size, sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &assi, size,        1, fp ) != 1 ) return _false;
	return _true;
}

b32 pxtnIO_matePCM_Write( FILE *fp, WOICE_STRUCT *p_voice )
{
	const pcmFORMAT *p_pcm =  p_voice->_voices[ 0 ].p_pcm;
	ptvUNIT         *p_vc  = &p_voice->_voices[ 0 ];
	u32 buf_size = ( p_pcm->_smp_head + p_pcm->_smp_body + p_pcm->_smp_tail ) * p_pcm->_ch * p_pcm->_bps / 8;

	_MATERIALSTRUCT_PCM pcm = {0};
	pcm.x3x_unit_no =                      0;
	pcm.basic_key   = (u16)p_vc->basic_key  ;
	pcm.voice_flags =      p_vc->voice_flags;
	pcm.ch          = (u16)p_pcm->_ch       ;
	pcm.bps         = (u16)p_pcm->_bps      ;
	pcm.sps         = (u32)p_pcm->_sps      ;
	pcm.tuning      =      p_vc->tuning     ;
	pcm.data_size   =      buf_size         ;

	u32 size = sizeof(_MATERIALSTRUCT_PCM) + buf_size;
	if( fwrite( &size        , sizeof(u32                ),        1, fp ) !=        1 ) return _false;
	if( fwrite( &pcm         , sizeof(_MATERIALSTRUCT_PCM),        1, fp ) !=        1 ) return _false;
	if( fwrite( p_pcm->_p_smp, sizeof(u8                 ), buf_size, fp ) != buf_size ) return _false;
	return _true;
}

b32 pxtnIO_matePTN_Write( FILE *fp, WOICE_STRUCT *p_voice )
{
	_MATERIALSTRUCT_PTN ptn = {0};

	// ptn -------------------------
	ptvUNIT *p_vc   = &p_voice->_voices[ 0 ];
	ptn.x3x_unit_no =                      0;
	ptn.basic_key   = (u16)p_vc->basic_key  ;
	ptn.voice_flags =      p_vc->voice_flags;
	ptn.tuning      =      p_vc->tuning     ;
	ptn.rrr         =                      1;

	// pre write
	s32 size = 0;
	if( fwrite( &size, sizeof(s32                ), 1, fp ) != 1 ) return _false;
	if( fwrite( &ptn , sizeof(_MATERIALSTRUCT_PTN), 1, fp ) != 1 ) return _false;

	size += sizeof(_MATERIALSTRUCT_PTN);
	if( !pxtnNoise_Design_Write( fp, &size, p_vc->p_ptn ) ) return _false;
	if( fseek ( fp, -size -sizeof(s32), SEEK_CUR ) !=   0 ) return _false;
	if( fwrite(     &size, sizeof(s32),    1, fp ) !=   1 ) return _false;
	if( fseek ( fp,  size             , SEEK_CUR ) !=   0 ) return _false;
	return _true;
}

b32 pxtnIO_matePTV_Write( FILE *fp, WOICE_STRUCT *p_voice )
{
	_MATERIALSTRUCT_PTV ptv = {0};

	// ptv -------------------------
	ptv.x3x_unit_no = (u16)0;
	ptv.x3x_tuning  =      0;//1.0f;//p_w->tuning;
	ptv.size        =      0;

	// pre write
	s32 size = 0;
	s32 head_size =    sizeof(_MATERIALSTRUCT_PTV) + sizeof(s32);
	if( fwrite( &size, sizeof(s32                ),    1, fp ) != 1 ) return _false;
	if( fwrite( &ptv , sizeof(_MATERIALSTRUCT_PTV),    1, fp ) != 1 ) return _false;
	if( !pxtnWoicePTV_Write( fp, &ptv.size, p_voice )               ) return _false;
	if( fseek ( fp   , -( ptv.size + head_size )  , SEEK_CUR ) != 0 ) return _false;

	size = ptv.size +  sizeof(_MATERIALSTRUCT_PTV);
	if( fwrite( &size, sizeof(s32                ),    1, fp ) != 1 ) return _false;
	if( fwrite( &ptv , sizeof(_MATERIALSTRUCT_PTV),    1, fp ) != 1 ) return _false;
	if( fseek ( fp   ,    ptv.size                , SEEK_CUR ) != 0 ) return _false;
	return _true;
}

b32 pxtnIO_mateOGGV_Write( FILE *fp, WOICE_STRUCT *p_voice )
{
#ifdef pxINCLUDE_OGGVORBIS
	extern s32 (* ogg_size  )( oggvUNIT *p_pcm          );
	extern b32 (* ogg_write )( oggvUNIT *p_pcm, FILE *fp);
	if( !ogg_size || !ogg_write ) return _false;

	_MATERIALSTRUCT_OGGV mate = {0};

	// oggv ------------------------
	ptvUNIT *p_vc    = &p_voice->_voices[ 0 ];
	mate.basic_key   = (u16)p_vc->basic_key  ;
	mate.voice_flags =      p_vc->voice_flags;
	mate.tuning      =      p_vc->tuning     ;

	s32 size = sizeof(_MATERIALSTRUCT_OGGV) + ogg_size( p_vc->p_oggv );
	if( fwrite( &size, sizeof(s32                 ), 1, fp ) != 1 ) return _false;
	if( fwrite( &mate, sizeof(_MATERIALSTRUCT_OGGV), 1, fp ) != 1 ) return _false;
	if( !ogg_write( p_vc->p_oggv, fp )                            ) return _false;
	return _true;
#else
	return _false;
#endif
}

b32 pxtnIO_effeOVER_Write( FILE *fp, OVERDRIVE_STRUCT *p_ovdrv )
{
	_EFFECT_OVERDRIVE over = {0};

	over.group = (u16)p_ovdrv->_group;
	over.cut   =      p_ovdrv->_cut  ;
	over.amp   =      p_ovdrv->_amp  ;

	// ovdrv ----------
	s32 size = sizeof(_EFFECT_OVERDRIVE);
	if( fwrite( &size, sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &over, size,        1, fp ) != 1 ) return _false;
	return _true;
}

b32 pxtnIO_effeDELA_Write( FILE *fp, DELAYSTRUCT *p_dela )
{
	_EFFECT_DELAY dela = {0};
	dela.unit  = (u16)p_dela->_unit ;
	dela.group = (u16)p_dela->_group;
	dela.rate  =      p_dela->_rate ;
	dela.freq  =      p_dela->_freq ;

	// dela ----------
	s32 size = sizeof(_EFFECT_DELAY);
	if( fwrite( &size, sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &dela, size,        1, fp ) != 1 ) return _false;
	return _true;
}

static b32 _Text_Write( const char *p, FILE *fp )
{
	if( !p ) return _false;
	u32 len = strlen( p );
	if( fwrite( &len, sizeof(u32 ),   1, fp ) !=   1 ) return _false;
	if( fwrite(  p  , sizeof(char), len, fp ) != len ) return _false;
	return _true;
}
b32 pxtnIO_textCOMM_Write( FILE *fp ){ return _Text_Write( pxtnText_Get_Comment(), fp ); }
b32 pxtnIO_textNAME_Write( FILE *fp ){ return _Text_Write( pxtnText_Get_Name   (), fp ); }

b32 pxtnIO_Event_V5_Write( FILE *fp, s32 rough )
{
	s32 clock          = 0;
	s32 relatived_size = 0;
	s32 absolute       = 0;
	for( const EVERECORD *p = pxtnEvelist_Get_Records(); p; p = p->next )
	{
		clock = p->clock - absolute;

		relatived_size += ddv_Variable_CheckSize( p->clock );
		relatived_size += 1;
		relatived_size += 1;
		relatived_size += ddv_Variable_CheckSize( p->value );

		absolute = p->clock;
	}

	s32 size    = sizeof(s32) + relatived_size;
	s32 eve_num = pxtnEvelist_Get_EventNum();
	if( fwrite( &size   , sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &eve_num, sizeof(s32), 1, fp ) != 1 ) return _false;

	absolute  = 0;
	s32 value = 0;
	for( const EVERECORD *p = pxtnEvelist_Get_Records(); p; p = p->next )
	{
		clock = p->clock - absolute;

		if( pxtnEvelist_Kind_IsTail( p->kind ) ) value = p->value / rough;
		else                                     value = p->value        ;

		if( !ddv_Variable_Write( clock / rough, fp,  NULL ) ) return _false;
		if( fwrite( &p->unit_no, sizeof(u8), 1, fp ) != 1 )   return _false;
		if( fwrite( &p->kind   , sizeof(u8), 1, fp ) != 1 )   return _false;
		if( !ddv_Variable_Write( value        , fp,  NULL ) ) return _false;

		absolute = p->clock;
	}

	return _true;
}

b32 pxtnIO_MasterV5_Write( FILE *fp, s32 rough )
{
	s32 beat_num; f32 beat_tempo; s32 beat_clock;
	pxtnMaster_Get( &beat_num, &beat_tempo, &beat_clock );

	s32 size         =         15;
	s16 bclock       = beat_clock / rough;
	s32 clock_repeat = bclock * beat_num * pxtnMaster_Get_RepeatMeas();
	s32 clock_last   = bclock * beat_num * pxtnMaster_Get_LastMeas  ();
	s8  bnum         = beat_num  ;
	f32 btempo       = beat_tempo;

	if( fwrite( &size        , sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &bclock      , sizeof(s16), 1, fp ) != 1 ) return _false;
	if( fwrite( &bnum        , sizeof(s8 ), 1, fp ) != 1 ) return _false;
	if( fwrite( &btempo      , sizeof(f32), 1, fp ) != 1 ) return _false;
	if( fwrite( &clock_repeat, sizeof(s32), 1, fp ) != 1 ) return _false;
	if( fwrite( &clock_last  , sizeof(s32), 1, fp ) != 1 ) return _false;
	return _true;
}

b32 pxtnIO_Master_x4x_Write( FILE *fp, s32 rough )
{
	s32 beat_num; f32 beat_tempo; s32 beat_clock;
	pxtnMaster_Get( &beat_num, &beat_tempo, &beat_clock );

	f32 btempo       = beat_tempo;
	s32 repeat_clock = beat_clock * beat_num * pxtnMaster_Get_RepeatMeas();
	s32 last_clock   = beat_clock * beat_num * pxtnMaster_Get_LastMeas  ();

	_x4x_MASTER mast = {0};
	mast.data_num    = 3;
	mast.rrr         = 0;
	mast.event_num   = 3;

	s32 size = 0;
	size += ddv_Variable_CheckSize( EVENTKIND_BEATCLOCK );
	size += ddv_Variable_CheckSize( EVENTKIND_NULL      );
	size += ddv_Variable_CheckSize( beat_clock / rough  );
	size += ddv_Variable_CheckSize( EVENTKIND_BEATTEMPO );
	size += ddv_Variable_CheckSize( EVENTKIND_NULL      );
	size += ddv_Variable_CheckSize( btempo              );
	size += ddv_Variable_CheckSize( EVENTKIND_BEATNUM   );
	size += ddv_Variable_CheckSize( EVENTKIND_NULL      );
	size += ddv_Variable_CheckSize( beat_num            );

	if( repeat_clock )
	{
		mast.event_num++;
		size += ddv_Variable_CheckSize( EVENTKIND_REPEAT     );
		size += ddv_Variable_CheckSize( repeat_clock / rough );
		size += ddv_Variable_CheckSize( EVENTKIND_NULL       );
	}
	if( last_clock )
	{
		mast.event_num++;
		size += ddv_Variable_CheckSize( EVENTKIND_LAST     );
		size += ddv_Variable_CheckSize( last_clock / rough );
		size += ddv_Variable_CheckSize( EVENTKIND_NULL     );
	}

	size += sizeof(_x4x_MASTER);
	if( fwrite( &size, sizeof(u32        ), 1, fp ) != 1 ) return _false;
	if( fwrite( &mast, sizeof(_x4x_MASTER), 1, fp ) != 1 ) return _false;

	if( !ddv_Variable_Write( EVENTKIND_BEATCLOCK, fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( EVENTKIND_NULL     , fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( beat_clock / rough , fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( EVENTKIND_BEATTEMPO, fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( EVENTKIND_NULL     , fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( btempo             , fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( EVENTKIND_BEATNUM  , fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( EVENTKIND_NULL     , fp, NULL ) ) return _false;
	if( !ddv_Variable_Write( beat_num           , fp, NULL ) ) return _false;

	if( repeat_clock )
	{
		if( !ddv_Variable_Write( EVENTKIND_REPEAT    , fp, NULL ) ) return _false;
		if( !ddv_Variable_Write( repeat_clock / rough, fp, NULL ) ) return _false;
		if( !ddv_Variable_Write( EVENTKIND_NULL      , fp, NULL ) ) return _false;
	}
	if( last_clock )
	{
		if( repeat_clock > 0 ) last_clock -= repeat_clock;
		if( !ddv_Variable_Write( EVENTKIND_LAST    , fp, NULL ) ) return _false;
		if( !ddv_Variable_Write( last_clock / rough, fp, NULL ) ) return _false;
		if( !ddv_Variable_Write( EVENTKIND_NULL    , fp, NULL ) ) return _false;
	}

	return _true;
}



////////////////////////////////////////
// Read               //////////////////
////////////////////////////////////////

b32 pxtnIO_assiUNIT_Read( DDV *p_read, b32 *pbNew )
{
	_ASSIST_UNIT assi = {0};
	TUNEUNITTONESTRUCT *units;

	u32 size;
	if( !ddv_Read( &size, sizeof(u32          ), 1, p_read )                ) return _false;
	if( size != sizeof(_ASSIST_UNIT )                      ){ *pbNew = _true; return _false; }
	if( !ddv_Read( &assi, sizeof(_ASSIST_UNIT ), 1, p_read )                ) return _false;
	if( assi.rrr != 0                                      ){ *pbNew = _true; return _false; }
	if( !(units = pxtnUnit_GetVariable( assi.unit_index ))                  ) return _false;

	memset( units->_name_buf,         0, sizeof(units->_name_buf) );
	memcpy( units->_name_buf, assi.name, sizeof(units->_name_buf) );
	return _true;
}

b32 pxtnIO_num_UNIT_Read( DDV *p_read, b32 *pbNew )
{
	_NUM_UNIT data = {0};

	u32 size = 0;
	if( !ddv_Read( &size, sizeof(u32      ), 1, p_read )                ) return _false;
	if( size != sizeof(_NUM_UNIT)                      ){ *pbNew = _true; return _false; }
	if( !ddv_Read( &data, sizeof(_NUM_UNIT), 1, p_read )                ) return _false;
	if( data.rrr != 0                                  ){ *pbNew = _true; return _false; }
	if( data.num > pxtnUnit_Get_NumMax()               ){ *pbNew = _true; return _false; }
	for( s32 i = 0; i < data.num; i++ ){ if( !pxtnUnit_AddNew()         ) return _false; }

	return _true;
}

b32 pxtnIO_Unit_Read( DDV *p_read, b32 *pbNew )
{
	_x3x_UNIT unit = {0};
	s32       size =  0 ;
	TUNEUNITTONESTRUCT *p_u;

	if( !ddv_Read( &size, sizeof(u32      ), 1, p_read ) ) return _false;
	if( !ddv_Read( &unit, sizeof(_x3x_UNIT), 1, p_read ) ) return _false;
	if( unit.type != WOICE_PCM && unit.type != WOICE_PTV && unit.type != WOICE_PTN ){ *pbNew = _true; return _false; }
	if( !pxtnUnit_AddNew()                               ) return _false;

	s32 unit_num = pxtnUnit_Count() - 1;
	if( !(p_u = pxtnUnit_GetVariable( unit_num )) ) return _false;



	s32 group_num = pxtnGroup_Get();
	if( unit.group >= group_num ) unit.group = group_num - 1;
	pxtnEvelist_x4x_Read_Add( 0, (u8)unit_num, EVENTKIND_GROUPNO, unit.group );
	pxtnEvelist_x4x_Read_NewKind();
	pxtnEvelist_x4x_Read_Add( 0, (u8)unit_num, EVENTKIND_VOICENO, unit_num   );
	pxtnEvelist_x4x_Read_NewKind();
	return _true;
}

b32 pxtnIO_UnitOld_Read( DDV *p_read )
{
	_x1x_UNIT unit = {0};
	s32       size =  0 ;
	TUNEUNITTONESTRUCT *p_u;

	if( !ddv_Read( &size, sizeof(u32      ), 1, p_read ) ) return _false;
	if( !ddv_Read( &unit, sizeof(_x1x_UNIT), 1, p_read ) ) return _false;
	if( unit.type != WOICE_PCM                           ) return _false;
	if( !pxtnUnit_AddNew()                               ) return _false;
	
	s32 unit_num = pxtnUnit_Count() - 1;
	if( !(p_u = pxtnUnit_GetVariable( unit_num )) ) return _false;
	
	memcpy( p_u->_name_buf, unit.name, sizeof(p_u->_name_buf) );

	s32 group_num = pxtnGroup_Get();
	if( unit.group >= group_num ) unit.group = group_num - 1;
	pxtnEvelist_x4x_Read_Add( 0, (u8)unit_num, EVENTKIND_GROUPNO, unit.group );
	pxtnEvelist_x4x_Read_NewKind();
	pxtnEvelist_x4x_Read_Add( 0, (u8)unit_num, EVENTKIND_VOICENO, unit_num   );
	pxtnEvelist_x4x_Read_NewKind();
	return _true;
}

b32 pxtnIO_assiWOIC_Read( DDV *p_read, b32 *pbNew )
{
	_ASSIST_WOICE assi = {0};
	WOICE_STRUCT *woices;

	u32 size;
	if( !ddv_Read( &size, sizeof(u32          ), 1, p_read )                ) return _false;
	if( size != sizeof(_ASSIST_WOICE)                      ){ *pbNew = _true; return _false; }
	if( !ddv_Read( &assi, sizeof(_ASSIST_WOICE), 1, p_read )                ) return _false;
	if( assi.rrr != 0                                      ){ *pbNew = _true; return _false; }
	if( !(woices = pxtnWoice_GetVariable( assi.woice_index ))               ) return _false;
	
	memset( woices->_name,         0, sizeof(woices->_name) );
	memcpy( woices->_name, assi.name, sizeof(woices->_name) );
	return _true;
}

b32 pxtnIO_matePCM_Read( DDV *p_read, b32 *pbNew )
{
	b32 b_ret = _false;
	_MATERIALSTRUCT_PCM  pcm = {0};
	WOICE_STRUCT        *p_w;

	u32 size;
	if( !ddv_Read( &size, sizeof(u32                ), 1, p_read )   ) return _false;
	if( !ddv_Read( &pcm , sizeof(_MATERIALSTRUCT_PCM), 1, p_read )   ) return _false;
	if( pcm.voice_flags & PTV_VOICEFLAG_UNCOVERED   ){ *pbNew = _true; return _false; }

	if( !(p_w = pxtnWoice_Create()) ) return _false;
	if( !pxtnWoice_Voice_Allocate( p_w, 1 ) ) goto End;
	
	ptvUNIT *p_vc = &p_w->_voices[ 0 ];
	pxtnWoice_Voice_Init( p_vc, VOICETYPE_Sampling );

	if( !pxtnPCM_Create( p_vc->p_pcm, pcm.ch, pcm.sps, pcm.bps, pcm.data_size / ( pcm.bps / 8 * pcm.ch ) ) ) goto End;
	if( !ddv_Read( p_vc->p_pcm->_p_smp, sizeof(u8), pcm.data_size, p_read ) ) goto End;
	
	p_w->_type          = WOICE_PCM      ;
	p_vc->voice_flags   = pcm.voice_flags;
	p_vc->basic_key     = pcm.basic_key  ;
	p_vc->tuning        = pcm.tuning     ;
	p_w->_x3x_basic_key = pcm.basic_key  ;
	p_w->_x3x_tuning    =               0;

	b_ret = _true;
End:
	if( !b_ret ) pxtnWoice_Remove( p_w->_voice_num );

	return b_ret;
}

b32 pxtnIO_matePTN_Read( DDV *p_read, b32 *pbNew )
{
	b32 b_ret = _false;
	_MATERIALSTRUCT_PTN  ptn = {0};
	WOICE_STRUCT        *p_w;

	u32 size;
	if( !ddv_Read( &size, sizeof(u32                ), 1, p_read )   ) return _false;
	if( !ddv_Read( &ptn , sizeof(_MATERIALSTRUCT_PTN), 1, p_read )   ) return _false;
	if( ptn.rrr != 1 && ptn.rrr != 0                ){ *pbNew = _true; return _false; }
	
	if( !(p_w = pxtnWoice_Create()) ) return _false;
	if( !pxtnWoice_Voice_Allocate( p_w, 1 ) ) goto End;
	
	ptvUNIT *p_vc = &p_w->_voices[ 0 ];
	pxtnWoice_Voice_Init( p_vc, VOICETYPE_Noise );


	if( !pxtnNoise_Design_Read( p_read, p_vc->p_ptn, pbNew ) ) goto End;

	p_w->_type          = WOICE_PTN      ;
	p_vc->voice_flags   = ptn.voice_flags;
	p_vc->basic_key     = ptn.basic_key  ;
	p_vc->tuning        = ptn.tuning     ;
	p_w->_x3x_basic_key = ptn.basic_key  ;
	p_w->_x3x_tuning    =               0;

	b_ret = _true;
End:
	if( !b_ret ) pxtnWoice_Remove( p_w->_voice_num );

	return b_ret;
}

b32 pxtnIO_matePTV_Read( DDV *p_read, b32 *pbNew )
{
	b32 b_ret = _false;
	_MATERIALSTRUCT_PTV  ptv = {0};
	WOICE_STRUCT        *p_w;

	u32 size;
	if( !ddv_Read( &size, sizeof(u32                ), 1, p_read )   ) return _false;
	if( !ddv_Read( &ptv , sizeof(_MATERIALSTRUCT_PTV), 1, p_read )   ) return _false;
	if( ptv.rrr != 0                                ){ *pbNew = _true; return _false; }
	
	if( !(p_w = pxtnWoice_Create()) ) return _false;






	if( !pxtnWoicePTV_Read( p_read, p_w, pbNew ) ) goto End;






	p_w->_x3x_tuning    = (ptv.x3x_tuning != 1.0) ? ptv.x3x_tuning : 0;

	b_ret = _true;
End:
	if( !b_ret ) pxtnWoice_Remove( p_w->_voice_num );

	return b_ret;
}

b32 pxtnIO_mateOGGV_Read( DDV *p_read, b32 *pbNew )
{
#ifdef pxINCLUDE_OGGVORBIS
	b32 b_ret = _false;
	_MATERIALSTRUCT_OGGV mate = {0};
	WOICE_STRUCT        *p_w;

	u32 size;
	if( !ddv_Read( &size, sizeof(u32                 ), 1, p_read )   ) return _false;
	if( !ddv_Read( &mate, sizeof(_MATERIALSTRUCT_OGGV), 1, p_read )   ) return _false;
	if( mate.voice_flags & PTV_VOICEFLAG_UNCOVERED   ){ *pbNew = _true; return _false; }
	
	if( !(p_w = pxtnWoice_Create()) ) return _false;
	if( !pxtnWoice_Voice_Allocate( p_w, 1 ) ) goto End;
	
	ptvUNIT *p_vc = &p_w->_voices[ 0 ];
	pxtnWoice_Voice_Init( p_vc, VOICETYPE_OggVorbis );

	extern oggvUNIT *(* ogg_read)(DDV *p_read, oggvUNIT *p);
	if( !ogg_read || !ogg_read( p_read, p_vc->p_oggv ) ) goto End;
	
	p_w->_type          = WOICE_OGGV      ;
	p_vc->voice_flags   = mate.voice_flags;
	p_vc->basic_key     = mate.basic_key  ;
	p_vc->tuning        = mate.tuning     ;
	p_w->_x3x_basic_key = mate.basic_key  ;
	p_w->_x3x_tuning    =                0;

	b_ret = _true;
End:
	if( !b_ret ) pxtnWoice_Remove( p_w->_voice_num );

	return b_ret;
#else
	return _false;
#endif
}

b32 pxtnIO_effeOVER_Read( DDV *p_read, b32 *pbNew )
{
	_EFFECT_OVERDRIVE ovdrv = {0};

	u32 size;
	if( !ddv_Read( &size , sizeof(u32              ), 1, p_read ) ) return _false;
	if( !ddv_Read( &ovdrv, sizeof(_EFFECT_OVERDRIVE), 1, p_read ) ) return _false;
	if( ovdrv.xxx != 0 || ovdrv.yyy != 0                          ) return _false;

	if( ovdrv.cut > TUNEOVERDRIVE_CUT_MAX || ovdrv.cut < TUNEOVERDRIVE_CUT_MIN ) return _false;
	if( ovdrv.amp > TUNEOVERDRIVE_AMP_MAX || ovdrv.amp < TUNEOVERDRIVE_AMP_MIN ) return _false;

	s32 slot = pxtnOverDrive_Add( ovdrv.cut, ovdrv.amp, ovdrv.group );
	if( slot < 0 || slot > pxtnOverDrive_Get_NumMax() ){ *pbNew = _true; return _false; }
	return _true;
}

b32 pxtnIO_effeDELA_Read( DDV *p_read, b32 *pbNew )
{
	_EFFECT_DELAY dela = {0};

	u32 size;
	if( !ddv_Read( &size, sizeof(u32          ), 1, p_read )   ) return _false;
	if( !ddv_Read( &dela, sizeof(_EFFECT_DELAY), 1, p_read )   ) return _false;
	if( dela.unit >= DELAYUNIT_num            ){ *pbNew = _true; return _false; }
	



	s32 slot = pxtnDelay_Add( (DELAYUNIT)dela.unit, dela.freq, dela.rate, dela.group );
	if( slot < 0 || slot > pxtnDelay_Get_NumMax() ){ *pbNew = _true; return _false; }
	return _true;
}

static b32 _Text_Read( char **pp, DDV *p_read )
{
	u32 buf_size = 0;
	if( !ddv_Read( &buf_size, sizeof(u32), 1, p_read ) ) return _false;
	if( buf_size < 0                                   ) return _false;

	if( !(*pp = (char *)malloc( buf_size + 1 )) ) return _false;
	memset( *pp, 0, buf_size + 1 );

	b32 b_ret = _false;
	if( buf_size ){ if( !ddv_Read( *pp, sizeof(char), buf_size, p_read ) ) goto End; }

	b_ret = _true;
End:
	if( !b_ret ){ free( *pp ); *pp = NULL; }

	return b_ret;
}
b32 pxtnIO_textCOMM_Read( DDV *p_read )
{
	char *p_comment_buf = NULL;
	if( !_Text_Read( &p_comment_buf, p_read ) ) return _false;
	pxtnText_Set_Comment( p_comment_buf );
	return _true;
}
b32 pxtnIO_textNAME_Read( DDV *p_read )
{
	char *p_name_buf = NULL;
	if( !_Text_Read( &p_name_buf, p_read ) ) return _false;
	pxtnText_Set_Name( p_name_buf );
	return _true;
}

b32 pxtnIO_Event_V5_Read( DDV *p_read )
{
	s32 size, eve_num;
	if( !ddv_Read( &size   , sizeof(s32), 1, p_read ) ) return _false;
	if( !ddv_Read( &eve_num, sizeof(s32), 1, p_read ) ) return _false;

	s32 clock  ;
	u8  unit_no;
	u8  kind   ;
	s32 value  ;

	s32 absolute = 0;
	for( s32 e = 0; e < eve_num; e++ )
	{
		if( !ddv_Variable_Read( &clock        , p_read ) ) return _false;
		if( !ddv_Read( &unit_no, sizeof(s8), 1, p_read ) ) return _false;
		if( !ddv_Read( &kind   , sizeof(s8), 1, p_read ) ) return _false;
		if( !ddv_Variable_Read( &value        , p_read ) ) return _false;
		absolute += clock;
		clock     = absolute;
		pxtnEvelist_Linear_Add( clock, unit_no, kind, value );
	}

	return _true;
}

s32 pxtnIO_v5_EventNum_Read( DDV *p_read )
{
	s32 size, eve_num;
	if( !ddv_Read( &size   , sizeof(s32), 1, p_read ) ) return 0;
	if( !ddv_Read( &eve_num, sizeof(s32), 1, p_read ) ) return 0;

	s32 clock  ;
	u8  unit_no;
	u8  kind   ;
	s32 value  ;

	s32 count = 0;
	for( s32 e = 0; e < eve_num; e++ )
	{
		if( !ddv_Variable_Read( &clock        , p_read ) ) return 0;
		if( !ddv_Read( &unit_no, sizeof(s8), 1, p_read ) ) return 0;
		if( !ddv_Read( &kind   , sizeof(s8), 1, p_read ) ) return 0;
		if( !ddv_Variable_Read( &value        , p_read ) ) return 0;
		count++;
	}
	if( count != eve_num ) return 0;

	return eve_num;
}

b32 pxtnIO_Event_x4x_Read( DDV *p_read, b32 bTailAbsolute, b32 bCheckRRR, b32 *pbNew )
{
	_x4x_EVENTSTRUCT evnt = {0};
//	TUNEUNITTONESTRUCT *p_u;

	u32 size;
	if( !ddv_Read( &size, sizeof(u32             ), 1, p_read )   ) return _false;
	if( !ddv_Read( &evnt, sizeof(_x4x_EVENTSTRUCT), 1, p_read )   ) return _false;
	if( evnt.data_num != 2                       ){ *pbNew = _true; return _false; }
	if( evnt.event_kind >= EVENTKIND_NUM         ){ *pbNew = _true; return _false; }
	if( bCheckRRR && evnt.rrr != 0               ){ *pbNew = _true; return _false; }
//	if( !(p_u = pxtnUnit_GetVariable( evnt.unit_index ))          ) return _false;

	s32 clock;
	s32 value;
	s32 e    ;

	s32 absolute = 0;
	for( e = 0; e < evnt.event_num; e++ )
	{
		if( !ddv_Variable_Read( &clock, p_read ) ) break;
		if( !ddv_Variable_Read( &value, p_read ) ) break;
		absolute += clock;
		clock     = absolute;
		pxtnEvelist_x4x_Read_Add( clock, (u8)evnt.unit_index, (u8)evnt.event_kind, value );
		if( bTailAbsolute && pxtnEvelist_Kind_IsTail( evnt.event_kind ) ) absolute += value;
	}
	if( e != evnt.event_num ) return _false;

	pxtnEvelist_x4x_Read_NewKind();

	return _true;
}

s32 pxtnIO_x4x_EventNum_Read( DDV *p_read )
{
	_x4x_EVENTSTRUCT evnt = {0};

	u32 size;
	if( !ddv_Read( &size, sizeof(u32             ), 1, p_read ) ) return 0;
	if( !ddv_Read( &evnt, sizeof(_x4x_EVENTSTRUCT), 1, p_read ) ) return 0;

	// support only 2
	if( evnt.data_num != 2 ) return 0;

	s32 work, e;
	for( e = 0; e < evnt.event_num; e++ )
	{
		if( !ddv_Variable_Read( &work, p_read ) ) break;
		if( !ddv_Variable_Read( &work, p_read ) ) break;
	}
	if( e != evnt.event_num ) return 0;

	return evnt.event_num;
}

b32 pxtnIO_MasterV5_Read( DDV *p_read, b32 *pbNew )
{
	u32 size;
	if( !ddv_Read( &size, sizeof(u32), 1, p_read )  ) return _false;
	if( size != 15                 ){ *pbNew = _true; return _false; }
	
	s16 beat_clock  ;
	s8  beat_num    ;
	f32 beat_tempo  ;
	s32 clock_repeat;
	s32 clock_last  ;
	if( !ddv_Read( &beat_clock  , sizeof(s16), 1, p_read ) ) return _false;
	if( !ddv_Read( &beat_num    , sizeof(s8 ), 1, p_read ) ) return _false;
	if( !ddv_Read( &beat_tempo  , sizeof(f32), 1, p_read ) ) return _false;
	if( !ddv_Read( &clock_repeat, sizeof(s32), 1, p_read ) ) return _false;
	if( !ddv_Read( &clock_last  , sizeof(s32), 1, p_read ) ) return _false;

	pxtnMaster_Set           ( beat_num, beat_tempo, beat_clock );
	pxtnMaster_Set_RepeatMeas( clock_repeat / ( beat_num * beat_clock ) );
	pxtnMaster_Set_LastMeas  ( clock_last   / ( beat_num * beat_clock ) );
	return _true;
}

s32 pxtnIO_Master_v5_EventNum_Read( DDV *p_read )
{
	u32 size;
	s8  buf[ 15 ];
	if( !ddv_Read( &size, sizeof(u32),  1, p_read ) ) return 0;
	if( size != 15                                  ) return 0;
	if( !ddv_Read( buf  , sizeof(s8 ), 15, p_read ) ) return 0;
	return 5;
}

b32 pxtnIO_Master_x4x_Read( DDV *p_read, b32 *pbNew )
{
	_x4x_MASTER mast = {0};

	u32 size;
	if( !ddv_Read( &size, sizeof(u32        ), 1, p_read )   ) return _false;
	if( !ddv_Read( &mast, sizeof(_x4x_MASTER), 1, p_read )   ) return _false;

	// unknown format
	if( mast.data_num != 3 || mast.rrr != 0 ){ *pbNew = _true; return _false; }

	s32 beat_clock = EVENTDEFAULT_BEATCLOCK;
	s32 beat_num   = EVENTDEFAULT_BEATNUM  ;
	f32 beat_tempo = EVENTDEFAULT_BEATTEMPO;
	s32 repeat_clock = 0;
	s32 last_clock   = 0;

	s32 status, clock, volume;

	s32 absolute = 0, e;
	for( e = 0; e < mast.event_num; e++ )
	{
		if( !ddv_Variable_Read( &status, p_read ) ) break;
		if( !ddv_Variable_Read( &clock , p_read ) ) break;
		if( !ddv_Variable_Read( &volume, p_read ) ) break;
		absolute += clock;
		clock     = absolute;

		switch( status )
		{
		case EVENTKIND_BEATCLOCK: beat_clock   =  volume;                      if( clock  ) return _false; break;
		case EVENTKIND_BEATTEMPO: memcpy( &beat_tempo, &volume, sizeof(f32) ); if( clock  ) return _false; break;
		case EVENTKIND_BEATNUM  : beat_num     =  volume;                      if( clock  ) return _false; break;
		case EVENTKIND_REPEAT   : repeat_clock =  clock ;                      if( volume ) return _false; break;
		case EVENTKIND_LAST     : last_clock   =  clock ;                      if( volume ) return _false; break;
		default: *pbNew = _true; return _false;
		}
	}
	if( e != mast.event_num ) return _false;

	pxtnMaster_Set           ( beat_num, beat_tempo, beat_clock );
	pxtnMaster_Set_RepeatMeas( repeat_clock / ( beat_num * beat_clock ) );
	pxtnMaster_Set_LastMeas  ( last_clock   / ( beat_num * beat_clock ) );

	return _true;
}

s32 pxtnIO_Master_x4x_EventNum_Read( DDV *p_read )
{
	_x4x_MASTER mast = {0};

	u32 size;
	if( !ddv_Read( &size, sizeof(u32        ), 1, p_read ) ) return 0;
	if( !ddv_Read( &mast, sizeof(_x4x_MASTER), 1, p_read ) ) return 0;
	if( mast.data_num != 3 ) return 0;

	s32 work;
	for( s32 e = 0; e < mast.event_num; e++ )
	{
		if( !ddv_Variable_Read( &work, p_read ) ) return 0;
		if( !ddv_Variable_Read( &work, p_read ) ) return 0;
		if( !ddv_Variable_Read( &work, p_read ) ) return 0;
	}

	return mast.event_num;
}

// 読み込み(プロジェクト)
b32 pxtnIO_x1x_Project_Read( DDV *p_read )
{
	_x1x_PROJECT prjc;
	char  name[ _MAX_PROJECTNAME_x1x ];
	long  beat_note, beat_num, quarter_clock, meas_num;
	long  size;
	float beat_tempo;
	
	if( !p_read ) return _false;
	
	memset( &prjc,   0, sizeof( _x1x_PROJECT ) );
	if( !ddv_Read( &size, 4,                     1, p_read ) ) return _false;
	if( !ddv_Read( &prjc, sizeof(_x1x_PROJECT ), 1, p_read ) ) return _false;
	
	memset( name, 0, _MAX_PROJECTNAME_x1x );
	memcpy( name, prjc.x1x_name, _MAX_PROJECTNAME_x1x );
	meas_num      = prjc.x1x_meas_num;
	beat_note     = prjc.x1x_beat_note;
	beat_num      = prjc.x1x_beat_num;
	beat_tempo    = prjc.x1x_beat_tempo;
	quarter_clock = prjc.x1x_quarter_clock;

	pxtnText_Set_Name( name );
	pxtnMaster_Set( beat_num, beat_tempo, quarter_clock );

	return _true;
}
