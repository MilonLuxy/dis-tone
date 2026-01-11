#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "pxtnPulse_Noise.h"
#include "pxtnPulse_Oscillator.h"


//                           01234567
static const char *_code  = "PTNOISE-";
static const u32   _ver_0 =  20051028 ; // -v.0.9.2.3
static const u32   _ver_1 =  20120418 ; // 16 wave types.
static s16        *_p_tables[ WAVETYPE_Num ];
static s32         _rand_buf[ 2 ];

#define _MAX_EDITUNIT_NUM            4
#define _MAX_EDITENVE_NUM            3

#define _EDITFLAG_XX1           0x0001
#define _EDITFLAG_XX2           0x0002
#define _EDITFLAG_ENVELOPE      0x0004
#define _EDITFLAG_PAN           0x0008
#define _EDITFLAG_OSC_MAIN      0x0010
#define _EDITFLAG_OSC_FREQ      0x0020
#define _EDITFLAG_OSC_VOLU      0x0040
#define _EDITFLAG_OSC_PAN       0x0080
#define _EDITFLAG_UNCOVERED 0xffffff83

#define _BASIC_SPS             44100.0
#define _BASIC_FREQUENCY         100.0 // 100Hz
#define _KEY_TOP                0x3200 // 40

#define _smp_num_rand            44100
#define _smp_num            (s32)( _BASIC_SPS / _BASIC_FREQUENCY )

#define _LIMIT_SMPNUM (48000 * 10)
#define _LIMIT_ENVE_X ( 1000 * 10)
#define _LIMIT_ENVE_Y (  100     )
#define _LIMIT_OSC_FREQUENCY 44100.0f
#define _LIMIT_OSC_VOLUME      200.0f
#define _LIMIT_OSC_OFFSET      100.0f


enum _RANDOMTYPE
{
	_RANDOM_None = 0,
	_RANDOM_Saw     ,
	_RANDOM_Rect    ,
};

typedef struct
{
	f64         incriment;
	f64         offset;
	f64         volume;
	const s16  *p_smp;
	b8          b_rev;
	_RANDOMTYPE ran_type;
	s32         rdm_start;
	s32         rdm_margin;
	s32         rdm_index;
}
_OSCILLATOR;

typedef struct
{
	s32 smp;
	f64 mag;
}
_POINT;

typedef struct
{
	b8      b_use;
	f64     pan[ 2 ];
	s32     enve_index;
	f64     enve_mag_start;
	f64     enve_mag_margin;
	s32     enve_count;
	s32     enve_num;
	_POINT *enves;

	_OSCILLATOR main;
	_OSCILLATOR freq;
	_OSCILLATOR volu;
}
_UNIT;


////////////////////////////
// private functions ////
////////////////////////////

static b32 _WriteOscillator( const ptnOSCILLATOR *p_osc, FILE *fp, s32 *p_add )
{
	if( !ddv_Variable_Write( (s32) p_osc->type        , fp, p_add ) ) return _false;
	if( !ddv_Variable_Write( (s32) p_osc->b_rev       , fp, p_add ) ) return _false;
	if( !ddv_Variable_Write( (s32)(p_osc->freq   * 10), fp, p_add ) ) return _false;
	if( !ddv_Variable_Write( (s32)(p_osc->volume * 10), fp, p_add ) ) return _false;
	if( !ddv_Variable_Write( (s32)(p_osc->offset * 10), fp, p_add ) ) return _false;
	return _true;
}

static b32 _ReadOscillator( ptnOSCILLATOR *p_osc, DDV *p_read )
{
	s32 work;
	if( !ddv_Variable_Read( &work, p_read ) ) return _false; p_osc->type   = (WAVETYPE)work;
//	if( p_osc->type >= WAVETYPE_Num         ) return _false;
	if( !ddv_Variable_Read( &work, p_read ) ) return _false; p_osc->b_rev  = work ? _true : _false;
	if( !ddv_Variable_Read( &work, p_read ) ) return _false; p_osc->freq   = (f32)work / 10;
	if( !ddv_Variable_Read( &work, p_read ) ) return _false; p_osc->volume = (f32)work / 10;
	if( !ddv_Variable_Read( &work, p_read ) ) return _false; p_osc->offset = (f32)work / 10;
	return _true;
}

static u32 _MakeFlags( const ptnUNIT *pU )
{
	u32 flags = 0;
										 flags |= _EDITFLAG_ENVELOPE;
	if( pU->pan                        ) flags |= _EDITFLAG_PAN     ;
	if( pU->main.type != WAVETYPE_None ) flags |= _EDITFLAG_OSC_MAIN;
	if( pU->freq.type != WAVETYPE_None ) flags |= _EDITFLAG_OSC_FREQ;
	if( pU->volu.type != WAVETYPE_None ) flags |= _EDITFLAG_OSC_VOLU;
	return flags;
}

static void _random_reset( void )
{
	_rand_buf[ 0 ] = 0x4444;
	_rand_buf[ 1 ] = 0x8888;
}

static s16 _random_get( void )
{
	s32 w1, w2;
	s8 *p1;
	s8 *p2;

	w1 = (s16)_rand_buf[ 0 ] + _rand_buf[ 1 ];
	p1 = (s8*)&w1;
	p2 = (s8*)&w2;
	p2[ 0 ] = p1[ 1 ];
	p2[ 1 ] = p1[ 0 ];
	_rand_buf[ 1 ] = (s16)_rand_buf[ 0 ];
	_rand_buf[ 0 ] = (s16)w2;

	return (s16)w2;
}

static void _RevisionUnit( ptnOSCILLATOR *p_osc )
{
	if( p_osc->type >= WAVETYPE_Num ) p_osc->type = WAVETYPE_None;
	pxMem_cap( &p_osc->freq  , _LIMIT_OSC_FREQUENCY, 0 );
	pxMem_cap( &p_osc->volume, _LIMIT_OSC_VOLUME   , 0 );
	pxMem_cap( &p_osc->offset, _LIMIT_OSC_OFFSET   , 0 );
}

static void _RevisionDesign( ptnDESIGN *p_design )
{
	ptnUNIT *p_u;
	s32     i, e;

	if( p_design->smp_num_44k > _LIMIT_SMPNUM ) p_design->smp_num_44k = _LIMIT_SMPNUM;

	for( i = 0; i < p_design->unit_num; i++ )
	{
		p_u = &p_design->units[ i ];
		if( p_u->b_use )
		{
			for( e = 0; e < p_u->enve_num; e++ )
			{
				pxMem_cap( &p_u->enves[ e ].x, _LIMIT_ENVE_X, 0 );
				pxMem_cap( &p_u->enves[ e ].y, _LIMIT_ENVE_Y, 0 );
			}
			pxMem_cap( &p_u->pan, 100, -100 );

			_RevisionUnit( &p_u->main );
			_RevisionUnit( &p_u->freq );
			_RevisionUnit( &p_u->volu );
		}
	}
}

static void _set_ocsillator( _OSCILLATOR *p_to, ptnOSCILLATOR *p_from, s32 sps )
{
	s16 *p;
	
	switch( p_from->type )
	{
	case   WAVETYPE_Random : p_to->ran_type = _RANDOM_Saw ; break;
	case   WAVETYPE_Random2: p_to->ran_type = _RANDOM_Rect; break;
	default                : p_to->ran_type = _RANDOM_None; break;
	}

	p_to->incriment  = ( _BASIC_SPS / sps ) * ( p_from->freq   / _BASIC_FREQUENCY );
	
	// offset
	if( p_to->ran_type != _RANDOM_None ) p_to->offset = 0;
	else                                 p_to->offset = (f64)_smp_num * ( p_from->offset / 100 );

	p_to->volume     = ( p_from->volume  / 100 );
	p_to->p_smp      = _p_tables[ p_from->type ];
	p_to->b_rev      = p_from->b_rev;

	p_to->rdm_start  = 0;
	p_to->rdm_index  = (s32)( (f64)_smp_num_rand * ( p_from->offset / 100 ) );
	p                = _p_tables[ WAVETYPE_Random ];
	p_to->rdm_margin = p[ p_to->rdm_index ];
}

static void _incriment( _OSCILLATOR *p_osc, f64 incriment )
{
	p_osc->offset += incriment;
	if( p_osc->offset > _smp_num )
	{
		p_osc->offset     -= _smp_num;
		if( p_osc->offset >= _smp_num ) p_osc->offset = 0;
		if( p_osc->ran_type != _RANDOM_None )
		{
			s16 *p = _p_tables[ WAVETYPE_Random ];
			p_osc->rdm_start  = p[ p_osc->rdm_index ];
			p_osc->rdm_index++;
			if( p_osc->rdm_index >= _smp_num_rand ) p_osc->rdm_index = 0;
			p_osc->rdm_margin = p[ p_osc->rdm_index ] - p_osc->rdm_start;
		}
	}
}


///////////////////////////
// public functions ////
///////////////////////////

b32 pxtnNoise_Initialize( void )
{
	b32  b_ret = _false;
	s32  a, s;
	s16 *p, v;
	f64  work;

	pxtnOscillator osc;
	
	pxPOINT overtones_sine [ 1] = { { 1,128} };
	pxPOINT overtones_saw2 [16] = { { 1,128},{ 2,128},{ 3,128},{ 4,128},{ 5,128},{ 6,128},{ 7,128},{ 8,128},{ 9,128},{10,128},{11,128},{12,128},{13,128},{14,128},{15,128},{16,128}, };

	pxPOINT overtones_rect2[ 8] = { { 1,128},{ 3,128},{ 5,128},{ 7,128},{ 9,128},{11,128},{13,128},{15,128}, };

	pxPOINT coodi_tri      [ 4] = { { 0,  0},{ _smp_num/4,128},{_smp_num*3/4,-128},{ _smp_num,  0} };

	_random_reset();


	for( s = 0; s < WAVETYPE_Num; s++ ) _p_tables[ s ] = NULL;

	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_None      ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Sine      ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Saw       ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Rect      ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Random    ], sizeof(s16) * _smp_num_rand ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Saw2      ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Rect2     ], sizeof(s16) * _smp_num      ) ) goto End;

	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Tri       ], sizeof(s16) * _smp_num      ) ) goto End;
 //	if( !pxMem_zero_alloc( (void**)&_p_tables[ pxWAVETYPE_Random2 ], sizeof(s16) * _smp_num_rand ) ) goto End; x
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Rect3     ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Rect4     ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Rect8     ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Rect16    ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Saw3      ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Saw4      ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Saw6      ], sizeof(s16) * _smp_num      ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&_p_tables[ WAVETYPE_Saw8      ], sizeof(s16) * _smp_num      ) ) goto End;

	// none --

    // sine --
	osc.ReadyGetSample( overtones_sine, 1, 128, _smp_num, 0 );
	p = _p_tables[ WAVETYPE_Sine ];
	for( s = 0; s < _smp_num; s++ )
	{
		work = osc.GetOneSample_Overtone( s );
		pxMem_cap( &work, 1.0, -1.0 );
		*p = (s16)( work * MAX_S16BIT );
		p++;
	}

	// saw down --
	p = _p_tables[ WAVETYPE_Saw ];
	work = MAX_S16BIT + MAX_S16BIT;
	for( s = 0; s < _smp_num; s++ ){ *p = (s16)( MAX_S16BIT - work * s / _smp_num ); p++; }

	// rect --
	p = _p_tables[ WAVETYPE_Rect ];
	for( s = 0; s < _smp_num / 2; s++ ){ *p = (s16)(  MAX_S16BIT ); p++; }
	for( s    ; s < _smp_num    ; s++ ){ *p = (s16)( -MAX_S16BIT ); p++; }

	// random -- 
	p = _p_tables[ WAVETYPE_Random ];
	for( s = 0; s < _smp_num_rand; s++ ){ *p = _random_get(); p++; }

    // saw2 --
	osc.ReadyGetSample( overtones_saw2, 16, 128, _smp_num, 0 );
	p = _p_tables[ WAVETYPE_Saw2 ];
	for( s = 0; s < _smp_num; s++ )
	{
		work = osc.GetOneSample_Overtone( s );
		pxMem_cap( &work, 1.0, -1.0 );
		*p = (s16)( work * MAX_S16BIT );
		p++;
	}

    // rect2 --
	osc.ReadyGetSample( overtones_rect2, 8, 128, _smp_num, 0 );
	p = _p_tables[ WAVETYPE_Rect2 ];
	for( s = 0; s < _smp_num; s++ )
	{
		work = osc.GetOneSample_Overtone( s );
		pxMem_cap( &work, 1.0, -1.0 );
		*p = (s16)( work * MAX_S16BIT );
		p++;
	}
	
	// Triangle -- 
	osc.ReadyGetSample( coodi_tri, 4, 128, _smp_num, _smp_num );	
	p = _p_tables[ WAVETYPE_Tri ];
	for( s = 0; s < _smp_num; s++ )
	{
		work = osc.GetOneSample_Coodinate( s ); if( work > 1.0 ) work = 1.0; if( work < -1.0 ) work = -1.0;
		*p = (s16)( work * MAX_S16BIT );
		p++;
	}

	// Random2  -- x

	// Rect-3  -- 
	p = _p_tables[ WAVETYPE_Rect3 ];
	for( s = 0; s < _smp_num /  3; s++ ){ *p = (s16)(  MAX_S16BIT ); p++; }
	for( s    ; s < _smp_num     ; s++ ){ *p = (s16)( -MAX_S16BIT ); p++; }
	// Rect-4   -- 
	p = _p_tables[ WAVETYPE_Rect4 ];
	for( s = 0; s < _smp_num /  4; s++ ){ *p = (s16)(  MAX_S16BIT ); p++; }
	for( s    ; s < _smp_num     ; s++ ){ *p = (s16)( -MAX_S16BIT ); p++; }
	// Rect-8   -- 
	p = _p_tables[ WAVETYPE_Rect8 ];
	for( s = 0; s < _smp_num /  8; s++ ){ *p = (s16)(  MAX_S16BIT ); p++; }
	for( s    ; s < _smp_num     ; s++ ){ *p = (s16)( -MAX_S16BIT ); p++; }
	// Rect-16  -- 
	p = _p_tables[ WAVETYPE_Rect16 ];
	for( s = 0; s < _smp_num / 16; s++ ){ *p = (s16)(  MAX_S16BIT ); p++; }
	for( s    ; s < _smp_num     ; s++ ){ *p = (s16)( -MAX_S16BIT ); p++; }

	// Saw-3    -- 
	p = _p_tables[ WAVETYPE_Saw3 ];
	for( s = 0; s < _smp_num /  3; s++ ){ *p = (s16)(  MAX_S16BIT ); p++; }
	for( s    ; s < _smp_num*2/ 3; s++ ){ *p = (s16)(           0 ); p++; }
	for( s    ; s < _smp_num     ; s++ ){ *p = (s16)( -MAX_S16BIT ); p++; }

	// Saw-4    -- 
	p = _p_tables[ WAVETYPE_Saw4 ];
	for( s = 0; s < _smp_num  / 4; s++ ){ *p = (s16)(  MAX_S16BIT   ); p++; }
	for( s    ; s < _smp_num*2/ 4; s++ ){ *p = (s16)(  MAX_S16BIT/3 ); p++; }
	for( s    ; s < _smp_num*3/ 4; s++ ){ *p = (s16)( -MAX_S16BIT/3 ); p++; }
	for( s    ; s < _smp_num     ; s++ ){ *p = (s16)( -MAX_S16BIT   ); p++; }

	// Saw-6    -- 
	p = _p_tables[ WAVETYPE_Saw6 ];
	a = _smp_num *1 / 6; v =  MAX_S16BIT                 ; for( s = 0; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *2 / 6; v =  MAX_S16BIT - MAX_S16BIT*2/5; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *3 / 6; v =               MAX_S16BIT  /5; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *4 / 6; v =             - MAX_S16BIT  /5; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *5 / 6; v = -MAX_S16BIT + MAX_S16BIT*2/5; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num       ; v = -MAX_S16BIT                 ; for( s    ; s < a; s++ ){ *p = v; p++; }

	// Saw-8    -- 
	p = _p_tables[ WAVETYPE_Saw8 ];
	a = _smp_num *1 / 8; v =  MAX_S16BIT                 ; for( s = 0; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *2 / 8; v =  MAX_S16BIT - MAX_S16BIT*2/7; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *3 / 8; v =  MAX_S16BIT - MAX_S16BIT*4/7; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *4 / 8; v =               MAX_S16BIT  /7; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *5 / 8; v =             - MAX_S16BIT  /7; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *6 / 8; v = -MAX_S16BIT + MAX_S16BIT*4/7; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num *7 / 8; v = -MAX_S16BIT + MAX_S16BIT*2/7; for( s    ; s < a; s++ ){ *p = v; p++; }
	a = _smp_num       ; v = -MAX_S16BIT                 ; for( s    ; s < a; s++ ){ *p = v; p++; }


	b_ret = _true;
End:
	if( !b_ret ) pxtnNoise_Release();

	return b_ret;
}

void pxtnNoise_Release( void )
{
	for( s32 i = 0; i < WAVETYPE_Num; i++ ) pxMem_free( (void**)&_p_tables[ i ] );
}

b32 pxtnNoise_Design_Write( FILE *fp, s32 *p_add, ptnDESIGN *p_design )
{
	b32 b_ret = _false;

	s32 seek  = (p_add) ? *p_add : 0;

	if( fwrite(  _code , sizeof(char), 8, fp ) != 8 ) goto End;
	if( fwrite( &_ver_1, sizeof(s32 ), 1, fp ) != 1 ) goto End;
	seek += 12;
	if( !ddv_Variable_Write( p_design->smp_num_44k, fp, &seek ) ) goto End;

	s8 unit_num = 0;
	if( fwrite( &unit_num, sizeof(s8), 1, fp ) != 1 ) goto End;
	s32 num_seek = seek;
	seek += 1;

	for( s32 u = 0; u < p_design->unit_num; u++ )
	{
		const ptnUNIT *pU = &p_design->units[ u ];
		if( pU->b_use )
		{
			// ƒtƒ‰ƒO
			s32 flags = _MakeFlags( pU );
			if( !ddv_Variable_Write( flags, fp, &seek ) ) goto End;
			if( flags & _EDITFLAG_ENVELOPE )
			{
				if( !ddv_Variable_Write( pU->enve_num, fp, &seek ) ) goto End;
				for( s32 e = 0; e < pU->enve_num; e++ )
				{
					if( !ddv_Variable_Write( pU->enves[ e ].x, fp, &seek ) ) goto End;
					if( !ddv_Variable_Write( pU->enves[ e ].y, fp, &seek ) ) goto End;
				}
			}
			if( flags & _EDITFLAG_PAN      )
			{
				s8 byte = (s8)pU->pan;
				if( fwrite( &byte, sizeof(s8), 1, fp ) != 1 ) goto End;
				seek++;
			}
			if( flags & _EDITFLAG_OSC_MAIN ){ if( !_WriteOscillator( &pU->main, fp, &seek ) ) goto End; }
			if( flags & _EDITFLAG_OSC_FREQ ){ if( !_WriteOscillator( &pU->freq, fp, &seek ) ) goto End; }
			if( flags & _EDITFLAG_OSC_VOLU ){ if( !_WriteOscillator( &pU->volu, fp, &seek ) ) goto End; }
			unit_num++;
		}
	}

	// update unit_num.
	if( fseek ( fp, num_seek - seek        , SEEK_CUR ) != 0 ) goto End;
	if( fwrite(    &unit_num, sizeof(s8), 1,       fp ) != 1 ) goto End;
	if( fseek ( fp,     seek - num_seek - 1, SEEK_CUR ) != 0 ) goto End;
	if( p_add ) *p_add = seek;

	b_ret = _true;
End:

	return b_ret;
}

b32 pxtnNoise_OpenWrite( const char *file_name, ptnDESIGN *p_design )
{
	FILE *fp = NULL;
	if( !(fp = fopen( file_name, "wb" )) ) return _false;
	b32 b_ret = pxtnNoise_Design_Write( fp, NULL, p_design );
	fclose( fp );

	return b_ret;
}

b32 pxtnNoise_Design_Read( DDV *p_read, ptnDESIGN *p_design, b32 *pbNew )
{
	b32 b_ret = _false;
	s32 u, e, flags;
	s8  unit_num   ;
	s8  byte       ;
	u32 ver        ;

	ptnUNIT *pU    ;

	char code[8];
	
	if( !ddv_Read(  code    , 1, 8, p_read                    ) ) goto End;
	if( memcmp   (  code, _code, 8                            ) ) goto End;
	if( !ddv_Read( &ver     , 4, 1, p_read                    ) ) goto End;
	if( ver > _ver_0 && ver > _ver_1           ){ *pbNew = _true; goto End; }
	if( !ddv_Variable_Read( &p_design->smp_num_44k, p_read    ) ) goto End;
	if( !ddv_Read( &unit_num, 1, 1, p_read                    ) ) goto End;
	if( p_design->unit_num < 0                                  ) goto End;
	if( p_design->unit_num > _MAX_EDITUNIT_NUM ){ *pbNew = _true; goto End; }
	p_design->unit_num = unit_num;

	if( !pxMem_zero_alloc( (void**)&p_design->units, sizeof(ptnUNIT) * p_design->unit_num ) ) goto End;

	for( u = 0; u < p_design->unit_num; u++ )
	{
		pU = &p_design->units[ u ];
		pU->b_use = _true;

		if( !ddv_Variable_Read( &flags, p_read ) ) goto End;
		if( flags & _EDITFLAG_UNCOVERED ){ *pbNew = _true; goto End; }

		if( flags & _EDITFLAG_ENVELOPE )
		{
			if( !ddv_Variable_Read( &pU->enve_num, p_read ) ) goto End;
			if( pU->enve_num > _MAX_EDITENVE_NUM ){ *pbNew = _true; goto End; }
			if( !pxMem_zero_alloc( (void**)&pU->enves, sizeof(pxPOINT) * pU->enve_num ) ) goto End;
			for( e = 0; e < pU->enve_num; e++ )
			{
				if( !ddv_Variable_Read( &pU->enves[ e ].x, p_read ) ) goto End;
				if( !ddv_Variable_Read( &pU->enves[ e ].y, p_read ) ) goto End;
			}
		}

		if( flags & _EDITFLAG_PAN )
		{
			if( !ddv_Read( &byte, 1, 1, p_read ) ) goto End;
			pU->pan = byte;
		}
		
		if( flags & _EDITFLAG_OSC_MAIN ){ if( !_ReadOscillator( &pU->main, p_read ) ) goto End; }
		if( flags & _EDITFLAG_OSC_FREQ ){ if( !_ReadOscillator( &pU->freq, p_read ) ) goto End; }
		if( flags & _EDITFLAG_OSC_VOLU ){ if( !_ReadOscillator( &pU->volu, p_read ) ) goto End; }
	}

	b_ret = _true;
End:
	if( !b_ret ) pxtnNoise_Design_Release( p_design );

	return b_ret;
}

void pxtnNoise_Design_Release( ptnDESIGN *p )
{
	if( p->units )
	{
		for( s32 u = 0; u < p->unit_num; u++ )
		{
			if( p->units[ u ].enves ) pxMem_free( (void**)&p->units[ u ].enves );
		}
		pxMem_free( (void**)&p->units );
	}
	memset( p, 0, sizeof(ptnDESIGN ) );
}

b32 pxtnNoise_Allocate( ptnDESIGN *p_design, s32 unit_num, s32 envelope_num )
{
	b32 b_ret = _false;

	p_design->unit_num = unit_num;
	if( !pxMem_zero_alloc( (void**)&p_design->units, sizeof(ptnDESIGN) * unit_num ) ) goto End;

	for( s32 u = 0; u < unit_num; u++ )
	{
		ptnUNIT *p_unit = &p_design->units[ u ];
		p_unit->enve_num = envelope_num;
		if( !pxMem_zero_alloc( (void**)&p_unit->enves, sizeof(pxPOINT) * p_unit->enve_num ) ) goto End;
	}

	b_ret = _true;
End:
	if( !b_ret ) pxtnNoise_Design_Release( p_design );

	return b_ret;
}

b32 pxtnNoise_CopyFrom( ptnDESIGN *p_dst, const ptnDESIGN *p_src )
{
	b32 b_ret = _false;
	
	p_dst->smp_num_44k = p_src->smp_num_44k;
	p_dst->unit_num    = p_src->unit_num;
	p_dst->units       = p_src->units;
	p_dst->units       = NULL;

	if( p_src->unit_num )
	{
		s32 enve_num = p_src->units[ 0 ].enve_num;
		if( !pxMem_zero_alloc( (void**)&p_dst->units, sizeof(ptnDESIGN) * p_src->unit_num ) ) goto End;
		for( s32 u = 0; u < p_src->unit_num; u++ )
		{
			p_dst->units[ u ].b_use    = p_src->units[ u ].b_use   ;
			p_dst->units[ u ].enve_num = p_src->units[ u ].enve_num;
			p_dst->units[ u ].freq     = p_src->units[ u ].freq    ;
			p_dst->units[ u ].main     = p_src->units[ u ].main    ;
			p_dst->units[ u ].pan      = p_src->units[ u ].pan     ;
			p_dst->units[ u ].volu     = p_src->units[ u ].volu    ;
			if( !pxMem_zero_alloc( (void**)&p_dst->units[ u ].enves, sizeof(pxPOINT) * enve_num ) ) goto End;
			for( s32 e = 0; e < enve_num; e++ ) p_dst->units[ u ].enves[ e ] = p_src->units[ u ].enves[ e ];
		}
	}

	b_ret = _true;
End:
	if( !b_ret ) pxtnNoise_Design_Release( p_dst );

	return b_ret;
}

s32 pxtnNoise_SamplingSize( const ptnDESIGN *p_design, s32 channel_num, s32 sps, s32 bps )
{
	pxMem_cap( &sps, _BASIC_SPS, 1 );
	return (p_design->smp_num_44k / (s32)( _BASIC_SPS / sps )) * channel_num * bps / 8;
}

b32 pxtnNoise_Build( u8 **pp_smp, ptnDESIGN *p_design, s32 channel_num, s32 sps, s32 bps )
{
	b32  b_ret      = _false;
	s32  offset     =    0;
	f64  work       =    0;
	f64  vol        =    0;
	f64  fre        =    0;
	f64  store      =    0;
	s32  s, c, u, e =    0; // sample | channel | unit | envelope
	s32  unit_num   =    0;
	u8  *p = NULL;
	s32  smp_num    =    0;

	_UNIT *units    = NULL;
	_UNIT *pU;

	_RevisionDesign( p_design );

	unit_num = p_design->unit_num;

	if( !pxtnNoise_Initialize() ) goto End;
	if( !pxMem_zero_alloc( (void**)&units, sizeof(_UNIT) * unit_num ) ) goto End;
	pxMem_cap( &sps, _BASIC_SPS, 1 );

	//
	for( u = 0; u < unit_num; u++ )
	{
		pU = &units[ u ];

		pU->b_use    = p_design->units[ u ].b_use;
		pU->enve_num = p_design->units[ u ].enve_num;
		if(            p_design->units[ u ].pan == 0 )
		{
			pU->pan[ 0 ] = 1;
			pU->pan[ 1 ] = 1;
		}
		else if( p_design->units[ u ].pan < 0 )
		{
			pU->pan[ 0 ] = 1;
			pU->pan[ 1 ] = (f64)( 100.0f + p_design->units[ u ].pan ) / 100;
		}
		else
		{
			pU->pan[ 1 ] = 1;
			pU->pan[ 0 ] = (f64)( 100.0f - p_design->units[ u ].pan ) / 100;
		}

		if( !pxMem_zero_alloc( (void**)&pU->enves, sizeof(_POINT) * pU->enve_num ) ) goto End;

		// envelope
		for( e = 0; e < p_design->units[ u ].enve_num; e++ )
		{
			pU->enves[ e ].smp =   sps * p_design->units[ u ].enves[ e ].x / 1000;
			pU->enves[ e ].mag = (f64)p_design->units[ u ].enves[ e ].y /  100;
		}
		pU->enve_index      = 0;
		pU->enve_mag_start  = 0;
		pU->enve_mag_margin = 0;
		pU->enve_count      = 0;
		while( pU->enve_index < pU->enve_num )
		{
			pU->enve_mag_margin = pU->enves[ pU->enve_index ].mag - pU->enve_mag_start;
			if( pU->enves[ pU->enve_index ].smp ) break;
			pU->enve_mag_start = pU->enves[ pU->enve_index ].mag;
			pU->enve_index++;
		}

		_set_ocsillator( &pU->main, &p_design->units[ u ].main, sps );
		_set_ocsillator( &pU->freq, &p_design->units[ u ].freq, sps );
		_set_ocsillator( &pU->volu, &p_design->units[ u ].volu, sps );
	}

	smp_num = p_design->smp_num_44k / ( _BASIC_SPS / sps );
	
	if( *pp_smp ){ free( *pp_smp ); *pp_smp = NULL; }
	*pp_smp = (u8 *)malloc( smp_num * channel_num * bps / 8 );
	if( *pp_smp == NULL ) goto End;
	p = *pp_smp;


	for( s = 0; s < smp_num; s++ )
	{
		for( c = 0; c < channel_num; c++ )
		{
			store = 0;
			for( u = 0; u < unit_num; u++ )
			{
				pU = &units[ u ];
	
				if( pU->b_use )
				{
					// main
					switch( pU->main.ran_type )
					{
					case _RANDOM_None:
						offset =            (s32)pU->main.offset ;
						if( offset >= 0 ) work = pU->main.p_smp[ offset ];
						else              work = 0;
						break;
					case _RANDOM_Saw:
						if( pU->main.offset >= 0 ) work = pU->main.rdm_start + pU->main.rdm_margin * (s32)pU->main.offset / _smp_num;
						else                       work = 0;
						break;
					case _RANDOM_Rect:
						if( pU->main.offset >= 0 ) work = pU->main.rdm_start;
						else                       work = 0;
						break;
					}
					if( pU->main.b_rev ) work *= -1;
					work = work * pU->main.volume;
					
					// volu
					switch( pU->volu.ran_type )
					{
					case _RANDOM_None:
						offset = (s32)pU->volu.offset;
						vol    = (f64)pU->volu.p_smp[ offset ];
						break;
					case _RANDOM_Saw:
						vol = pU->volu.rdm_start + pU->volu.rdm_margin * (s32)pU->volu.offset / _smp_num;
						break;
					case _RANDOM_Rect:
						vol = pU->volu.rdm_start;
						break;
					}
					if( pU->volu.b_rev ) vol *= -1;
					vol  = vol * pU->volu.volume;

					work = work * ( vol + MAX_S16BIT ) / ( MAX_S16BIT * 2 );
					work = work * pU->pan[ c ];

					// envelope
					if( pU->enve_index < pU->enve_num )
						work *= pU->enve_mag_start + ( pU->enve_mag_margin * pU->enve_count / pU->enves[ pU->enve_index ].smp );
					else 
						work *= pU->enve_mag_start;
					store += work;
				}
			}


			s32 s32_ = (s32)store;
			pxMem_cap( &s32_, MAX_S16BIT, -MAX_S16BIT );
			if( bps ==  8 ){ *p   = (u8 )( ( s32_ >> 8 ) + 128 ); p += 1; } //  8bit
			else{    *( (s16*)p ) = (s16)    s32_;                p += 2; } // 16bit
		}

		// incriment
		for( u = 0; u < unit_num; u++ )
		{
			pU = &units[ u ];

			if( pU->b_use )
			{
				switch( pU->freq.ran_type )
				{
				case _RANDOM_None:
					offset = (s32)pU->freq.offset;
					fre = _KEY_TOP * pU->freq.p_smp[ offset ] / MAX_S16BIT;    // * freq
					break;
				case _RANDOM_Saw:
					fre = pU->freq.rdm_start + pU->freq.rdm_margin * (s32) pU->freq.offset / _smp_num;
					break;
				case _RANDOM_Rect:
					fre = pU->freq.rdm_start;
					break;
				}
				if( pU->freq.b_rev ) fre *= -1;
				fre = fre * pU->freq.volume;

				_incriment( &pU->main, pU->main.incriment * pxtnFrequency_Get( (s32)fre ) );
				_incriment( &pU->freq, pU->freq.incriment );
				_incriment( &pU->volu, pU->volu.incriment );

				// envelope
				if( pU->enve_index < pU->enve_num )
				{
					pU->enve_count++;
					if( pU->enve_count >= pU->enves[ pU->enve_index ].smp )
					{
						pU->enve_count      = 0;
						pU->enve_mag_start  = pU->enves[ pU->enve_index ].mag;
						pU->enve_mag_margin = 0;
						pU->enve_index++;
						while( pU->enve_index < pU->enve_num )
						{
							pU->enve_mag_margin = pU->enves[ pU->enve_index ].mag - pU->enve_mag_start;
							if( pU->enves[ pU->enve_index ].smp ) break;
							pU->enve_mag_start = pU->enves[ pU->enve_index ].mag;
							pU->enve_index++;
						}
					}
				}
			}
		}
	}

	b_ret = _true;
End:
	pxtnNoise_Release();
/*
	for( s32 i = 0; i < WAVETYPE_Num; i++ )
	{
		pxMem_free( (void**)&_p_tables[ i ] );
	}
*/
	return b_ret;
}
