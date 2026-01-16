//
// x1x : v.0.1.2.8 (-2005/06/03) project-info has quality, tempo, clock.
// x2x : v.0.6.N.N (-2006/01/15) no exe version.
// x3x : v.0.7.N.N (-2006/09/30) unit includes voice / basic-key use for only view
//                               no-support event: voice_no, group_no, tuning.
// x4x : v.0.8.3.4 (-2007/10/20) unit has event-list.

#include <StdAfx.h>


#include "pxtnDelay.h"
#include "pxtnError.h"
#include "pxtnEvelist.h"
#include "pxtnIO.h"
#include "pxtnMaster.h"
#include "pxtnOverDrive.h"
#include "pxtnText.h"
#include "pxtnUnit.h"
#include "pxtnWoice.h"
#include "pxtnService.h"


#define _VERSIONSIZE                16
#define _CODESIZE                    8
#define _MAX_FMTVER_x1x_EVENTNUM 10000

//                                       0123456789012345
static const char *_code_tune_x2x     = "PTTUNE--20050608";
static const char *_code_tune_x3x     = "PTTUNE--20060115";
static const char *_code_tune_x4x     = "PTTUNE--20060930";
static const char *_code_tune_v5      = "PTTUNE--20071119";

static const char *_code_proj_x1x     = "PTCOLLAGE-050227";
static const char *_code_proj_x2x     = "PTCOLLAGE-050608";
static const char *_code_proj_x3x     = "PTCOLLAGE-060115";
static const char *_code_proj_x4x     = "PTCOLLAGE-060930";
static const char *_code_proj_v5      = "PTCOLLAGE-071119";


static const char *_code_x1x_PROJ     = "PROJECT="; // プロジェクトヘッダ
static const char *_code_x1x_EVEN     = "EVENT==="; // イベント情報
static const char *_code_x1x_UNIT     = "UNIT===="; // ユニット情報
static const char *_code_x1x_END      = "END====="; // End Of Project File
static const char *_code_x1x_PCM      = "matePCM="; // 音源情報
static const char *_code_x1x_mateENVE = "mateENVE"; // 音源情報
static const char *_code_x1x_mateHEAD = "mateHEAD"; // 音源情報
static const char *_code_x1x_mateLFO  = "mateLFO="; // 音源情報

static const char *_code_x3x_pxtnUNIT = "pxtnUNIT";
static const char *_code_x4x_evenMAST = "evenMAST";
static const char *_code_x4x_evenUNIT = "evenUNIT";

static const char *_code_antiOPER     = "antiOPER"; // anti operation (edit)

static const char *_code_num_UNIT     = "num UNIT";
static const char *_code_MasterV5     = "MasterV5";
static const char *_code_Event_V5     = "Event V5";
static const char *_code_matePCM      = "matePCM ";
static const char *_code_matePTV      = "matePTV ";
static const char *_code_matePTN      = "matePTN ";
static const char *_code_mateOGGV     = "mateOGGV";
static const char *_code_effeDELA     = "effeDELA"; // 効果情報
static const char *_code_effeOVER     = "effeOVER";
static const char *_code_textNAME     = "textNAME"; // 名前
static const char *_code_textCOMM     = "textCOMM"; // コメント
static const char *_code_assiUNIT     = "assiUNIT";
static const char *_code_assiWOIC     = "assiWOIC";
static const char *_code_pxtoneND     = "pxtoneND";

enum enum_TagCode
{
	enum_TagCode_Unknown  = 0,
	enum_TagCode_antiOPER    ,

	enum_TagCode_x1x_PROJ    ,
	enum_TagCode_x1x_UNIT    ,
	enum_TagCode_x1x_PCM     ,
	enum_TagCode_x1x_EVEN    ,
	enum_TagCode_x1x_END     ,
	enum_TagCode_x3x_pxtnUNIT,
	enum_TagCode_x4x_evenMAST,
	enum_TagCode_x4x_evenUNIT,

	enum_TagCode_num_UNIT    ,
	enum_TagCode_MasterV5    ,
	enum_TagCode_Event_V5    ,
	enum_TagCode_matePCM     ,
	enum_TagCode_matePTV     ,
	enum_TagCode_matePTN     ,
	enum_TagCode_mateOGGV    ,
	enum_TagCode_effeDELA    ,
	enum_TagCode_effeOVER    ,
	enum_TagCode_textNAME    ,
	enum_TagCode_textCOMM    ,
	enum_TagCode_assiUNIT    ,
	enum_TagCode_assiWOIC    ,
	enum_TagCode_pxtoneND    ,
};

enum enum_FMTVER
{
	enum_FMTVER_unknown = 0,
	enum_FMTVER_x1x, // fix event num = 10000
	enum_FMTVER_x2x, // no version of exe
	enum_FMTVER_x3x, // unit has voice / basic-key for only view
	enum_FMTVER_x4x, // unit has event
	enum_FMTVER_v5 ,
};



////////////////////////////
// ローカル関数 ////////////
////////////////////////////

// コードの識別
static enum_TagCode _CheckTagCode( const char *p_code )
{
	if(      !memcmp( p_code, _code_antiOPER    , _CODESIZE ) ) return enum_TagCode_antiOPER;
	else if( !memcmp( p_code, _code_x1x_PROJ    , _CODESIZE ) ) return enum_TagCode_x1x_PROJ;
	else if( !memcmp( p_code, _code_x1x_UNIT    , _CODESIZE ) ) return enum_TagCode_x1x_UNIT;
	else if( !memcmp( p_code, _code_x1x_PCM     , _CODESIZE ) ) return enum_TagCode_x1x_PCM ;
	else if( !memcmp( p_code, _code_x1x_EVEN    , _CODESIZE ) ) return enum_TagCode_x1x_EVEN;
	else if( !memcmp( p_code, _code_x1x_END     , _CODESIZE ) ) return enum_TagCode_x1x_END ;
	else if( !memcmp( p_code, _code_x3x_pxtnUNIT, _CODESIZE ) ) return enum_TagCode_x3x_pxtnUNIT;
	else if( !memcmp( p_code, _code_x4x_evenMAST, _CODESIZE ) ) return enum_TagCode_x4x_evenMAST;
	else if( !memcmp( p_code, _code_x4x_evenUNIT, _CODESIZE ) ) return enum_TagCode_x4x_evenUNIT;
	else if( !memcmp( p_code, _code_num_UNIT    , _CODESIZE ) ) return enum_TagCode_num_UNIT;
	else if( !memcmp( p_code, _code_Event_V5    , _CODESIZE ) ) return enum_TagCode_Event_V5;
	else if( !memcmp( p_code, _code_MasterV5    , _CODESIZE ) ) return enum_TagCode_MasterV5;
	else if( !memcmp( p_code, _code_matePCM     , _CODESIZE ) ) return enum_TagCode_matePCM ;
	else if( !memcmp( p_code, _code_matePTV     , _CODESIZE ) ) return enum_TagCode_matePTV ;
	else if( !memcmp( p_code, _code_matePTN     , _CODESIZE ) ) return enum_TagCode_matePTN ;
	else if( !memcmp( p_code, _code_mateOGGV    , _CODESIZE ) ) return enum_TagCode_mateOGGV;
	else if( !memcmp( p_code, _code_effeDELA    , _CODESIZE ) ) return enum_TagCode_effeDELA;
	else if( !memcmp( p_code, _code_effeOVER    , _CODESIZE ) ) return enum_TagCode_effeOVER;
	else if( !memcmp( p_code, _code_textNAME    , _CODESIZE ) ) return enum_TagCode_textNAME;
	else if( !memcmp( p_code, _code_textCOMM    , _CODESIZE ) ) return enum_TagCode_textCOMM;
	else if( !memcmp( p_code, _code_assiUNIT    , _CODESIZE ) ) return enum_TagCode_assiUNIT;
	else if( !memcmp( p_code, _code_assiWOIC    , _CODESIZE ) ) return enum_TagCode_assiWOIC;
	else if( !memcmp( p_code, _code_pxtoneND    , _CODESIZE ) ) return enum_TagCode_pxtoneND;
	return enum_TagCode_Unknown;
}

static b32 _ReadVersion( DDV *p_read, enum_FMTVER *p_fmt_ver, u16 *p_exe_ver )
{
	char version[ _VERSIONSIZE ] = {0};
	u16 dummy;

	if( !ddv_Read( version, 1, _VERSIONSIZE, p_read ) ){ pxtnError_Set( "read." ); return _false; }

	// fmt version
	if(      !memcmp( version, _code_proj_x1x , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_x1x; *p_exe_ver = 0; return _true; }
	else if( !memcmp( version, _code_proj_x2x , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_x2x; *p_exe_ver = 0; return _true; }
	else if( !memcmp( version, _code_proj_x3x , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_x3x;								  }
	else if( !memcmp( version, _code_proj_x4x , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_x4x;								  }
	else if( !memcmp( version, _code_proj_v5  , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_v5 ;								  }
	else if( !memcmp( version, _code_tune_x2x , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_x2x; *p_exe_ver = 0; return _true; }
	else if( !memcmp( version, _code_tune_x3x , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_x3x;								  }
	else if( !memcmp( version, _code_tune_x4x , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_x4x;								  }
	else if( !memcmp( version, _code_tune_v5  , _VERSIONSIZE ) ){ *p_fmt_ver = enum_FMTVER_v5 ;								  }
	else { pxtnError_Set( "unknown version." ); return _false; }

	// exe version
	if( !ddv_Read( p_exe_ver, sizeof(u16), 1, p_read ) ){ pxtnError_Set( "read." ); return _false; }
	if( !ddv_Read( &dummy   , sizeof(u16), 1, p_read ) ){ pxtnError_Set( "read." ); return _false; }

	return _true;
}

// プロジェクトの読み込み(失敗したら自動的に初期化)
static b32 _ReadTuneItems( DDV *p_read )
{
	b32 b_ret = _false;
	b32 b_end = _false;
	char code[ _CODESIZE + 1 ] = {0};
	b32 pbNew = _false;


	/// 音源より先にそれに対応するユニットがなければならない ///
	while( !b_end )
	{
		if( !ddv_Read( code, 1, _CODESIZE, p_read ) ){ pxtnError_Set( "read file." ); goto term; }

		enum_TagCode tag = _CheckTagCode( code );
		switch( tag )
		{
		case enum_TagCode_antiOPER    : pxtnError_Set( "anti operation." ); goto term;

		// new -------
		case enum_TagCode_num_UNIT    : if( !pxtnIO_num_UNIT_Read( p_read, &pbNew ) ){ pxtnError_Set( "unit num."    ); goto term; } break;
		case enum_TagCode_MasterV5    : if( !pxtnIO_MasterV5_Read( p_read, &pbNew ) ){ pxtnError_Set( "read master." ); goto term; } break;
		case enum_TagCode_Event_V5    : if( !pxtnIO_Event_V5_Read( p_read         ) ){ pxtnError_Set( "read event."  ); goto term; } break;

		case enum_TagCode_matePCM     : if( !pxtnIO_matePCM_Read ( p_read, &pbNew ) ){ pxtnError_Set( "read pcm."    ); goto term; } break;
		case enum_TagCode_matePTV     : if( !pxtnIO_matePTV_Read ( p_read, &pbNew ) ){ pxtnError_Set( "read ptv."    ); goto term; } break;
		case enum_TagCode_matePTN     : if( !pxtnIO_matePTN_Read ( p_read, &pbNew ) ){ pxtnError_Set( "read ptn."    ); goto term; } break;

		case enum_TagCode_mateOGGV    :

#ifdef pxINCLUDE_OGGVORBIS
			if( !pxtnIO_mateOGGV_Read( p_read, &pbNew ) ){ pxtnError_Set( "read oggv." ); goto term; }
#else
			pxtnError_Set( "ogg no supported" ); goto term;
#endif
			break;

		case enum_TagCode_effeDELA    : if( !pxtnIO_effeDELA_Read( p_read, &pbNew ) ){ pxtnError_Set( "read delay."         ); goto term; } break;
		case enum_TagCode_effeOVER    : if( !pxtnIO_effeOVER_Read( p_read, &pbNew ) ){ pxtnError_Set( "read overdrive."     ); goto term; } break;
		case enum_TagCode_textNAME    : if( !pxtnIO_textNAME_Read( p_read         ) ){ pxtnError_Set( "read name."          ); goto term; } break;
		case enum_TagCode_textCOMM    : if( !pxtnIO_textCOMM_Read( p_read         ) ){ pxtnError_Set( "read comment."       ); goto term; } break;
		case enum_TagCode_assiWOIC    : if( !pxtnIO_assiWOIC_Read( p_read, &pbNew ) ){ pxtnError_Set( "read assist(voice)." ); goto term; } break;
		case enum_TagCode_assiUNIT    : if( !pxtnIO_assiUNIT_Read( p_read, &pbNew ) ){ pxtnError_Set( "read assist(unit)."  ); goto term; } break;
		case enum_TagCode_pxtoneND    : b_end = _true; break;

		// old -------
		case enum_TagCode_x4x_evenMAST: if( !pxtnIO_Master_x4x_Read ( p_read, &pbNew                ) ){ pxtnError_Set( "read master."       ); goto term; } break;
		case enum_TagCode_x4x_evenUNIT: if( !pxtnIO_Event_x4x_Read  ( p_read, _false, _true, &pbNew ) ){ pxtnError_Set( "read event."        ); goto term; } break;
		case enum_TagCode_x3x_pxtnUNIT: if( !pxtnIO_Unit_Read       ( p_read, &pbNew                ) ){ pxtnError_Set( "read unit."         ); goto term; } break;
		case enum_TagCode_x1x_PROJ    : if( !pxtnIO_x1x_Project_Read( p_read                        ) ){ pxtnError_Set( "read project(old)." ); goto term; } break;
		case enum_TagCode_x1x_UNIT    : if( !pxtnIO_UnitOld_Read    ( p_read                        ) ){ pxtnError_Set( "read unit(old)."    ); goto term; } break;
		case enum_TagCode_x1x_PCM     : if( !pxtnIO_matePCM_Read    ( p_read, &pbNew                ) ){ pxtnError_Set( "read pcm(old)."     ); goto term; } break;
		case enum_TagCode_x1x_EVEN    : if( !pxtnIO_Event_x4x_Read  ( p_read, _true, _false, &pbNew ) ){ pxtnError_Set( "read event(old)."   ); goto term; } break;
		case enum_TagCode_x1x_END     : b_end = _true; break;

		default: pxtnError_Set( "unknown [%s]", code ); goto term;
		}
	}

	b_ret = _true;
term:
	if( pbNew ) pxtnError_Set( "it\'s new format [%s]", code );
	
	return b_ret;
}

// fix old key event
static b32 _x3x_TuningKeyEvent()
{
	s32 unit_num  = pxtnUnit_Count ();
	s32 woice_num = pxtnWoice_Count();
	if( unit_num > woice_num ) return _false;

	for( s32 u = 0; u < unit_num; u++ )
	{
		if( u >= woice_num ) return _false;

		s32 change_value = pxtnWoice_GetVariable( u )->_x3x_basic_key - EVENTDEFAULT_BASICKEY;

		if( !pxtnEvelist_Count_Unit_Kind( u, EVENTKIND_KEY ) )
		{
			pxtnEvelist_Record_Add( 0, u, EVENTKIND_KEY, EVENTDEFAULT_KEY );
		}
		pxtnEvelist_Record_Value_Change( 0, -1, u, EVENTKIND_KEY, change_value );
	}
	return _true;
}

// fix old tuning (1.0)
static b32 _x3x_AddTuningEvent()
{
	s32 unit_num = pxtnUnit_Count();
	if( unit_num > pxtnWoice_Count() ) return _false;

	for( s32 u = 0; u < unit_num; u++ )
	{
		float tuning = pxtnWoice_GetVariable( u )->_x3x_tuning;
		if( tuning ) pxtnEvelist_Record_Add_f( 0, u, EVENTKIND_TUNING, tuning );
	}
	return _true;
}

static void _x3x_SetVoiceNames()
{
	for( s32 i = 0; i < pxtnWoice_Count(); i++ )
	{
		WOICE_STRUCT *_woices = pxtnWoice_GetVariable( i );
		sprintf( _woices->_name, "voice_%02d", i );
		_woices->_name_size = 8;
	}
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

void pxtnService_Clear( void )
{
	pxtnText_Set_Name      ( "no name" );
	pxtnText_Set_Comment   (    NULL   );
	pxtnUnit_RemoveAll     (           );
	pxtnEvelist_Reset      (           );
	pxtnWoice_RemoveAll    (           );
	pxtnDelay_RemoveAll    (           );
	pxtnOverDrive_RemoveAll(           );
	pxtnMaster_Reset       (           );
}


////////////////////////////////////////
// プロジェクトを保存 //////////////////
////////////////////////////////////////

b32 pxtnService_Write( const char *path, b32 b_tune )
{
	b32   b_ret   = _false;
	FILE *fp      = NULL ;
	s32   rough   = b_tune ? 10 : 1;
	u16   exe_ver = pxtnIO_GetCompileVersion();
	s32   dummy   =     0;


	// open file -------
	if( !(fp = fopen( path, "wb" )) ){ pxtnError_Set( "open file" ); goto End; }

	// format version
	if( fwrite( ( (b_tune) ? _code_tune_v5 : _code_proj_v5 ), 1, _VERSIONSIZE, fp ) != _VERSIONSIZE ){ pxtnError_Set( "write fmt ver" ); goto End; }

	// exe version
	if( fwrite( &exe_ver, 2,         1, fp ) !=         1 ){ pxtnError_Set( "write exe ver" ); goto End; }
	if( fwrite( &dummy  , 2,         1, fp ) !=         1 ){ pxtnError_Set( "write dummy"   ); goto End; }

	// master
	if( fwrite( _code_MasterV5, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code"   ); goto End; }
	if( !pxtnIO_MasterV5_Write( fp, rough )                     ){ pxtnError_Set( "write master" ); goto End; }

	// event
	if( fwrite( _code_Event_V5, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code"   ); goto End; }
	if( !pxtnIO_Event_V5_Write ( fp, rough )                    ){ pxtnError_Set( "write master" ); goto End; }

	// name
	if( pxtnText_Get_Name() )
	{
		if( fwrite( _code_textNAME, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code" ); goto End; }
		if( !pxtnIO_textNAME_Write( fp )                            ){ pxtnError_Set( "write name" ); goto End; }
	}

	// comment
	if( pxtnText_Get_Comment() )
	{
		if( fwrite( _code_textCOMM, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code"    ); goto End; }
		if( !pxtnIO_textCOMM_Write( fp )                            ){ pxtnError_Set( "write comment" ); goto End; }
	}

	// delay
	for( s32 d = 0; d < pxtnDelay_Count(); d++ )
	{
		if( fwrite( _code_effeDELA, 1, _CODESIZE, fp ) != _CODESIZE  ){ pxtnError_Set( "write code"  ); goto End; }
		if( !pxtnIO_effeDELA_Write( fp, pxtnDelay_GetVariable( d ) ) ){ pxtnError_Set( "write delay" ); goto End; }
	}

	// overdrive
	for( s32 o = 0; o < pxtnOverDrive_Count(); o++ )
	{
		if( fwrite( _code_effeOVER, 1, _CODESIZE, fp ) != _CODESIZE      ){ pxtnError_Set( "write code"      ); goto End; }
		if( !pxtnIO_effeOVER_Write( fp, pxtnOverDrive_GetVariable( o ) ) ){ pxtnError_Set( "write overdrive" ); goto End; }
	}

	// woice
	for( s32 w = 0; w < pxtnWoice_Count(); w++ )
	{
		WOICE_STRUCT *p_w = pxtnWoice_GetVariable( w );

		switch( p_w->_type )
		{
		case WOICE_PCM:
			if( fwrite( _code_matePCM , 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code" ); goto End; }
			if( !pxtnIO_matePCM_Write( fp, p_w )                        ){ pxtnError_Set( "write pcm"  ); goto End; }
			break;
		case WOICE_PTV:
			if( fwrite( _code_matePTV , 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code" ); goto End; }
			if( !pxtnIO_matePTV_Write( fp, p_w )                        ){ pxtnError_Set( "write ptv"  ); goto End; }
			break;
		case WOICE_PTN:
			if( fwrite( _code_matePTN , 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code" ); goto End; }
			if( !pxtnIO_matePTN_Write( fp, p_w )                        ){ pxtnError_Set( "write ptv"  ); goto End; }
			break;
		case WOICE_OGGV:

#ifdef pxINCLUDE_OGGVORBIS
			if( fwrite( _code_mateOGGV, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code" ); goto End; }
			if( !pxtnIO_mateOGGV_Write( fp, p_w )                       ){ pxtnError_Set( "write ogg"  ); goto End; }
#else
			pxtnError_Set( "ogg no supported" ); goto End;
#endif
			break;
        default: goto End;
		}

        if( !b_tune && p_w->_name_size > 0 )
		{
			if( fwrite( _code_assiWOIC, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code"       ); goto End; }
			if( !pxtnIO_assiWOIC_Write( fp, p_w, w )                    ){ pxtnError_Set( "write voice name" ); goto End; }
		}
	}

	// unit
	if( fwrite( _code_num_UNIT, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write unit num code" ); goto End; }
	if( !pxtnIO_num_UNIT_Write( fp, pxtnUnit_Count() )          ){ pxtnError_Set( "write unit num"      ); goto End; }

	for( s32 u = 0; u < pxtnUnit_Count(); u++ )
	{
		TUNEUNITTONESTRUCT *p_u = pxtnUnit_GetVariable( u );
		if( !b_tune && p_u->_name_size > 0 )
		{
			if( fwrite( _code_assiUNIT, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write code"      ); goto End; }
			if( !pxtnIO_assiUNIT_Write( fp, p_u, u )                    ){ pxtnError_Set( "write unit name" ); goto End; }
		}
	}

	if( fwrite( _code_pxtoneND, 1, _CODESIZE, fp ) != _CODESIZE ){ pxtnError_Set( "write end"      ); goto End; }
	if( fwrite( &dummy        , 4,         1, fp ) !=         1 ){ pxtnError_Set( "write end size" ); goto End; }
	
	b_ret = _true;
End:
	if( fp ) fclose( fp );

	return b_ret;
}

////////////////////////////////////////
// プロジェクトを読み込み //////////////
////////////////////////////////////////

s32 pxtnService_Pre_Count_Event( DDV *p_read, s32 p_count )
{
	b32  b_ret = _false;
	b32  b_end = _false;

	s32  count = 0;
	s32  size  = 0;
	char code[ _CODESIZE + 1 ] = {0};

	u16  exe_ver = 0;
	enum_FMTVER fmt_ver = enum_FMTVER_unknown;

	if( !_ReadVersion( p_read, &fmt_ver, &exe_ver ) ) goto End;

	if( fmt_ver == enum_FMTVER_x1x ){ count = _MAX_FMTVER_x1x_EVENTNUM; b_ret = _true; goto End; }

	while( !b_end )
	{
		if( !ddv_Read( code, 1, _CODESIZE, p_read ) ){ pxtnError_Set( "read." ); goto End; }

		switch( _CheckTagCode( code ) )
		{
		case enum_TagCode_Event_V5    : count += pxtnIO_v5_EventNum_Read        ( p_read ); break;
		case enum_TagCode_MasterV5    : count += pxtnIO_Master_v5_EventNum_Read ( p_read ); break;
		case enum_TagCode_x4x_evenMAST: count += pxtnIO_Master_x4x_EventNum_Read( p_read ); break;
		case enum_TagCode_x4x_evenUNIT: count += pxtnIO_x4x_EventNum_Read       ( p_read ); break;
		case enum_TagCode_pxtoneND    : b_end = _true; break;

		// skip
		case enum_TagCode_antiOPER    :
		case enum_TagCode_num_UNIT    :
		case enum_TagCode_x3x_pxtnUNIT:
		case enum_TagCode_matePCM     :
		case enum_TagCode_matePTV     :
		case enum_TagCode_matePTN     :
		case enum_TagCode_mateOGGV    :
		case enum_TagCode_effeDELA    :
		case enum_TagCode_effeOVER    :
		case enum_TagCode_textNAME    :
		case enum_TagCode_textCOMM    :
		case enum_TagCode_assiUNIT    :
		case enum_TagCode_assiWOIC    :

			if( !ddv_Read( &size, sizeof(s32), 1, p_read ) ){ pxtnError_Set( "read." ); goto End; }
			if( !ddv_Seek( p_read, size, SEEK_CUR )        ){ pxtnError_Set( "seek." ); goto End; }
			break;

		// ignore
		case enum_TagCode_x1x_PROJ:
		case enum_TagCode_x1x_UNIT:
		case enum_TagCode_x1x_PCM :
		case enum_TagCode_x1x_EVEN:
		case enum_TagCode_x1x_END :
		default                   : goto End;
		}
	}

	if( fmt_ver <= enum_FMTVER_x3x ) count += p_count * 4; // voice_no, group_no, key tuning, key event x3x

	b_ret = _true;
End:
	if( !b_ret ) count = 0;
	
	return count;
}

b32 pxtnService_Read( DDV *p_read, b32 b_tune )
{
	b32 b_ret = _false;
	u16 exe_ver = 0;
	enum_FMTVER fmt_ver = enum_FMTVER_unknown;

	pxtnService_Clear();

	if( !_ReadVersion( p_read, &fmt_ver, &exe_ver ) ) goto term;

	if( fmt_ver >= enum_FMTVER_v5 ) pxtnEvelist_Linear_Start  ();
	else                            pxtnEvelist_x4x_Read_Start();

	if( !_ReadTuneItems( p_read ) ) goto term;

	if( fmt_ver >= enum_FMTVER_v5  ) pxtnEvelist_Linear_End( _true );

	if( fmt_ver <= enum_FMTVER_x3x )
	{
		if( !_x3x_TuningKeyEvent() ) goto term;
		if( !_x3x_AddTuningEvent() ) goto term;
//		_x3x_SetVoiceNames();
	}

	if( !b_tune && pxtnMaster_Get_BeatClock() != EVENTDEFAULT_BEATCLOCK ){ pxtnError_Set( "edit prohibition" ); goto term; }

	s32 clock1 = pxtnEvelist_Get_MaxClock();
	s32 clock2 = pxtnMaster_Get_LastClock();
	pxtnMaster_Adjust_MeasNum( (clock1 > clock2) ? clock1 : clock2 );
	
	b_ret = _true;
term:
	if( !b_ret ) pxtnService_Clear();

	return b_ret;
}

b32 pxtnService_CheckPTIFormat( b8 *b_pti, DDV *p_read, const char *path )
{
	// Header check
	*b_pti = _false;
	enum_FMTVER dummy1; u16 dummy2;
	if( _ReadVersion( p_read, &dummy1, &dummy2 ) ) return _true; // File is 'ptcop' or 'pttune'
	ddv_Seek( p_read, 0, SEEK_SET );

	// PTI info check
	u8 beat_tempo, beat_divide;
	if( !ddv_Read( &beat_tempo , sizeof(u8), 1, p_read ) ){ pxtnError_Set( "read."          ); return _false; }
	if( !ddv_Read( &beat_divide, sizeof(u8), 1, p_read ) ){ pxtnError_Set( "read."          ); return _false; }
	if( !beat_tempo || !beat_divide                      ){ pxtnError_Set( "beat divide 0"  ); return _false; }

	// PTI beat tempo check
	s32 result_wait  =       60  / ( (f32)  beat_tempo  * (s32)beat_divide ) * 60;
	f32 result_tempo = (60 * 60) /   (f32)( result_wait *      beat_divide );
	if( isinf( result_tempo ) || isnan( result_tempo )   ){ pxtnError_Set( "inv-beat tempo" ); return _false; }
	*b_pti = _true;

	pxtnService_Clear();

	char name[ 16 ];
	strcpy( name, PathFindFileName( path ) );
	PathRemoveExtension( name );
	pxtnText_Set_Name  ( name );
	return _true;
}
