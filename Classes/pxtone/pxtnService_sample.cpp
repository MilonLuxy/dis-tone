#include <StdAfx.h>

#include "pxtnWoice.h"



////////////////////////////////////////////////
// Resampling   ///////////////////////////////
////////////////////////////////////////////////

static f32 _calculate_Lanczos( f32 distance )
{
	const f32 A      = 4.0f;
	if( fabsf( distance ) <  1e-6f ) return 1;
	if( fabsf( distance ) >=     A ) return 0;
	
	const f32 PI     = 3.14159265358979323846f;
	const f32 pix    = PI * distance;
	const f32 sinc   = sinf( pix ) / pix;
	const f32 window = sinf( pix / A ) / ( pix / A );
	return sinc * window;
}

static f32 _calculate_Gaussian( f32 distance )
{
	const f32 sigma = 0.6f;
	return expf( -( distance * distance ) / ( 2.0f * sigma * sigma ) );
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

		// cubic interpolation - 4-tap Catmull-Rom
		case 2:
		{
			f32 result = 0;
			s16 smp[ 4 ] = {};
			for( s32 i = 0; i < 4; i++ )
			{
				smp[ i ] = _sample_get( p_vi, index + (i - 1), ch, b_loop );
			}
			result = _calculate_CatmullRom( smp, frac );
			return (s32)round( result );
		}
		
		// gaussian interpolation - 4-tap
		case 3:
		{
			f32 result = 0, weight_sum = 0;
			for( s32 i = -1; i < 3; i++ )
			{
				s32 sample_pos = index + i;
				f32 weight     = _calculate_Gaussian( smp_pos - (f32)sample_pos );

				result     += _sample_get( p_vi, sample_pos, ch, b_loop ) * weight;
				weight_sum += weight;
			}
			if( weight_sum > 1e-6f ) result /= weight_sum;

			return (s32)round( result );
		}

		// sinc interpolation - 8-tap Lanczos
		case 4:
		{
			f32 result = 0;
			for( s32 i = -3; i < 5; i++ )
			{
				s32 sample_pos = index + i;
				result += _sample_get( p_vi, sample_pos, ch, b_loop ) *
						  _calculate_Lanczos( smp_pos - (f32)sample_pos );
			}
			return (s32)round( result );
		}
	}

	return 0;
}
