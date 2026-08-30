#include <StdAfx.h>

#include "pxtnWoice.h"



////////////////////////////////////////////////
// Resampling   ///////////////////////////////
////////////////////////////////////////////////

static f32 _calculate_Lanczos( f32 x )
{
	const f32 PI  = 3.14159265358979323846;
	const f32 A   = 4.0f;
	const f32 pix = PI * x;

	if( fabsf( x ) <  1e-6f ) return 1;
	if( fabsf( x ) >=     A ) return 0;
	return ( sinf( pix ) / pix ) * ( sinf( pix / A ) / ( pix / A ) );
}

static f32 _calculate_CatmullRom( const s16 *smp, f32 frac )
{
	if( !smp ) return 0;

	const f32 p0 = (f32)smp[ 0 ], p1 = (f32)smp[ 1 ], p2 = (f32)smp[ 2 ], p3 = (f32)smp[ 3 ];
	const f32 coef1 =   2.0f *             p1;
	const f32 coef2 = (       -p0                    + p2      ) * frac;
	const f32 coef3 = ( 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 ) * frac * frac;
	const f32 coef4 = (       -p0 + 3.0f * p1 - 3.0f * p2 + p3 ) * frac * frac * frac;

	return 0.5f * ( coef1 + coef2 + coef3 + coef4 );
}

static s16 _sample_get( const ptvINSTANCE *p_vi, s32 pos, s32 ch, b32 b_loop )
{
	if( !p_vi ) return 0;

	s32 smp_size = p_vi->smp_head_w + p_vi->smp_body_w + p_vi->smp_tail_w;
	if( smp_size <= 0 ) return 0;

	if( b_loop && p_vi->smp_body_w > 0 )
	{
		pos %= p_vi->smp_body_w;
		if( pos < 0 ) pos += p_vi->smp_body_w;
	}

	if( pos >= smp_size - 1 ) pos = smp_size - 1;
	if( pos <             0 ) pos =            0;

	return *(s16*)&p_vi->p_smp_w[ pos * 4 + ch * 2 ];
}

s32 pxtnSample_interpolate( s32 ch, f64 smp_pos, const ptvINSTANCE *p_vi, s8 inter, b32 b_loop )
{
	s32 index = (s32)floor( smp_pos );
	f32 frac  = (f32)     ( smp_pos - index );

	switch( inter )
	{
		// original - no interpolation
		case 0:
		default:
			return _sample_get( p_vi, index, ch, b_loop );

		// linear interpolation
		case 1:
		{
			s16 s0 = _sample_get( p_vi, index    , ch, b_loop );
			s16 s1 = _sample_get( p_vi, index + 1, ch, b_loop );

			return (s32)( s0 + frac * (s1 - s0) );
		}

		// cubic interpolation - Catmull-Rom
		case 2:
		{
			s16 smp[ 4 ] = {};
			for( s32 i = 0; i < 4; i++ ) smp[ i ] = _sample_get( p_vi, index + (i - 1), ch, b_loop );
			return (s32)_calculate_CatmullRom( smp, frac );
		}

		// sinc interpolation - 8-tap Lanczos
		case 3:
		{
			f32 result = 0;
			for( s32 i = -3; i < 5; i++ )
			{
				s32 sample_pos = index + i;
				result += _sample_get( p_vi, sample_pos, ch, b_loop ) *
						  _calculate_Lanczos( smp_pos - (f32)sample_pos );
			}
			return (s32)result;
		}
	}

	return 0;
}
