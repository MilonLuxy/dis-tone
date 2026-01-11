#include <StdAfx.h>


#include "pxtnPulse_Frequency.h"

#define _OCTAVE_NUM            16   // オクターブ数 
#define _KEY_PER_OCTAVE        12   // １オクターブ内のキー数
#define _FREQUENCY_PER_KEY     0x10 // １キーを構成する周波数の数

// 基準（x1）となる周波数テーブルインデックス
#define _BASIC_FREQUENCY_INDEX ( (_OCTAVE_NUM/2) * _KEY_PER_OCTAVE * _FREQUENCY_PER_KEY )
#define _TABLE_SIZE            (  _OCTAVE_NUM    * _KEY_PER_OCTAVE * _FREQUENCY_PER_KEY )

f32    *_freq_table = NULL;//[ _TABLE_SIZE ];


static f64 _GetDivideOctaveRate( s32 divi )
{
	f64 parameter = 1.0;
	f64 work;
	f64 result;
	f64 add;
	s32 i, j, k;

	// f64 は17桁
	for( i = 0; i < 17; i++ )
	{
		// addを作る
 		add = 1;
		for( j = 0; j < i; j++ ) add = add * 0.1;

		// 0 から 9 を調べる
		for( j = 0; j < 10; j++ )
		{
			work = parameter + add * j;

			// 分割回数かける
			result = 1.0;
			for( k = 0; k < divi; k++ )
			{
				result *= work;
				if( result >= 2.0 ) break;
			}

			// 分割回数かける前に 2.0 以上になったら終わり
			if( k != divi ) break;
		}
		// 2.0 超えなかった直前の値を採用
		parameter += add * ( j - 1 );
	}

	return parameter;
}

void pxtnFrequency_Initialize( void )
{
	f64 oct_table[ _OCTAVE_NUM ] =
	{
		0.00390625, //0  -8
		0.0078125,  //1  -7 
		0.015625,   //2  -6
		0.03125,    //3  -5
		0.0625,     //4  -4
		0.125,      //5  -3
		0.25,       //6  -2
		0.5,        //7  -1
		1,          //8  
		2,          //9   1
		4,          //a   2
		8,          //b   3
		16,         //c   4
		32,         //d   5
		64,         //e   6
		128,        //f   7
	};

	s32 key;
	s32 f;
	f64 oct_x24;
	f64 work;
	
	if( !(_freq_table = (f32*)malloc( sizeof(f32) * _TABLE_SIZE )) ) return;

	oct_x24 = _GetDivideOctaveRate( _KEY_PER_OCTAVE * _FREQUENCY_PER_KEY );

	for( f = 0; f < _OCTAVE_NUM * (_KEY_PER_OCTAVE * _FREQUENCY_PER_KEY); f++ )
	{
		work = oct_table[   f /  (_KEY_PER_OCTAVE * _FREQUENCY_PER_KEY) ];
		for( key = 0; key < f %  (_KEY_PER_OCTAVE * _FREQUENCY_PER_KEY); key++ )
		{
			work *= oct_x24;
		}
		_freq_table[ f ] = (f32)work;
	}
}

void pxtnFrequency_Release( void )
{
	if( _freq_table ) free( _freq_table ); _freq_table = NULL;
}

f32 pxtnFrequency_Get( s32 key )
{
	if( !_freq_table ) return 0;
	s32 i;

	i = (key + 0x6000) * _FREQUENCY_PER_KEY / 0x100;
	if( i < 0            ) i = 0;
	if( i >= _TABLE_SIZE ) i = _TABLE_SIZE -1;
	return _freq_table[ i ];
}

const f32 *pxtnFrequency_GetDirect( s32 *p_size )
{
	if( !_freq_table ) return 0;
	*p_size = _TABLE_SIZE;
	return _freq_table;
}
