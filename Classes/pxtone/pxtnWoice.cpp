#include <StdAfx.h>


#include "../Fixture/DataDevice.h"
#include "../Fixture/pxMem.h"
#include "../Streaming/Streaming.h"
#include "pxtnPulse_Oscillator.h"
#include "pxtnEvelist.h"
#include "pxtnWoice.h"

#define _DEFAULT_SMP_SIZE             400
#define _DEFAULT_PAN                   64
#define _DEFAULT_CHANNEL_NUM            2
#define _DEFAULT_SPS                44100
#define _DEFAULT_BPS                   16

static s32            _woice_num  =   0 ;
static WOICE_STRUCT **_woice_main = NULL;



////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _Voice_Release( WOICE_STRUCT *p_voice )
{
	if( !p_voice ) return;

	for( s32 v = 0; v < p_voice->_voice_num; v++ )
	{
		ptvUNIT     *p_vc = &p_voice->_voices [ v ];
		ptvINSTANCE *p_vi = &p_voice->_voinsts[ v ];

		if( p_vc )
		{
			pxMem_free     ( (void**)&p_vc->p_pcm  );
			if( p_vc->p_ptn ) pxtnNoise_Design_Release( p_vc->p_ptn );
#ifdef  pxINCLUDE_OGGVORBIS
			extern void (* ogg_free)(oggvUNIT *p_ogg);
			if( ogg_free ){ ogg_free( p_vc->p_oggv ); p_vc->p_oggv = NULL; }
#endif
			pxMem_free( (void**)&p_vc->envelope.points );
			pxMem_free( (void**)&p_vc->wave.points     );
		}
		if( p_vi )
		{
			pxMem_free( (void**)&p_vi->p_env   );
			pxMem_free( (void**)&p_vi->p_smp_w );
		}
	}
	pxMem_free( (void**)&p_voice->_voices  );
	pxMem_free( (void**)&p_voice->_voinsts );
	p_voice->_voice_num = 0;
}

static s32 _Get_Index( WOICE_STRUCT *p_voice )
{
	for( s32 index = 0; index < _woice_num; index++ ){ if( _woice_main[ index ] == p_voice ) return index; }
	return -1;
}

static WOICETYPE _Get_Type( const char *p_ext )
{
	const char *ext = PathFindExtension( p_ext );

	if( !strcmp( ext, ".wav"     ) ) return WOICE_PCM ;
	if( !strcmp( ext, ".ptvoice" ) ) return WOICE_PTV ;
	if( !strcmp( ext, ".ptnoise" ) ) return WOICE_PTN ;
	if( !strcmp( ext, ".ogg"     ) ) return WOICE_OGGV;
	return WOICE_None;
}

static s32 _Get_EmptySlot( void )
{
	for( s32 index = 0; index < _woice_num; index++ ){ if( !_woice_main[ index ] ) return index; }
	return -1;
}

static b32 _Copy( const WOICE_STRUCT *p_src, WOICE_STRUCT *p_dst )
{
	b32      b_ret = _false;
	ptvUNIT *p_vc1 = NULL ;
	ptvUNIT *p_vc2 = NULL ;

	if( !pxtnWoice_Voice_Allocate( p_dst, p_src->_voice_num ) ) goto End;

	p_dst->_type = p_src->_type;

	memcpy( p_dst->_name, p_src->_name, sizeof(p_src->_name) );
	p_dst->_name_size = p_src->_name_size;
	
	for( s32 v = 0; v < p_src->_voice_num; v++ )
	{
		p_vc1 = &p_src->_voices[ v ];
		p_vc2 = &p_dst->_voices[ v ];

		p_vc2->tuning      = p_vc1->tuning     ;
		p_vc2->data_flags  = p_vc1->data_flags ;
		p_vc2->basic_key   = p_vc1->basic_key  ;
		p_vc2->pan         = p_vc1->pan        ;
		p_vc2->type        = p_vc1->type       ;
		p_vc2->voice_flags = p_vc1->voice_flags;
		p_vc2->volume      = p_vc1->volume     ;

		// envelope
		p_vc2->envelope.body_num = p_vc1->envelope.body_num;
		p_vc2->envelope.fps      = p_vc1->envelope.fps     ;
		p_vc2->envelope.head_num = p_vc1->envelope.head_num;
		p_vc2->envelope.tail_num = p_vc1->envelope.tail_num;
		s32 num  = p_vc2->envelope.head_num + p_vc2->envelope.body_num + p_vc2->envelope.tail_num;
		s32 size = sizeof(pxPOINT) * num;
		if( !pxMem_zero_alloc( (void**)&p_vc2->envelope.points, size ) ) goto End;
		memcpy(                         p_vc2->envelope.points, p_vc1->envelope.points, size );

		// wave
		p_vc2->wave.num      = p_vc1->wave.num ;
		p_vc2->wave.reso     = p_vc1->wave.reso;
		size = sizeof(pxPOINT) * p_vc2->wave.num ;
		if( !pxMem_zero_alloc( (void**)&p_vc2->wave.points, size ) ) goto End;
		memcpy(                         p_vc2->wave.points, p_vc1->wave.points, size );

		if( !pxtnPCM_CopyFrom  ( p_vc2->p_pcm, p_vc1->p_pcm   ) ) goto End;
		if( !pxtnNoise_CopyFrom( p_vc2->p_ptn, p_vc1->p_ptn   ) ) goto End;
#ifdef  pxINCLUDE_OGGVORBIS
		if( !pxtnOggv_CopyFrom ( p_vc2->p_oggv, p_vc1->p_oggv ) ) goto End;
#endif
	}

	b_ret = _true;
End:
	if( !b_ret ) _Voice_Release( p_dst );

	return b_ret;
}

static void _UpdateWavePTV( ptvUNIT *p_vc, ptvINSTANCE *p_vi, s32 ch_num, s32 sps, s32 bps )
{
	f64 work, osc;
	s32 s32_;
	s32 pan_volume[ 2 ] = {_DEFAULT_PAN, _DEFAULT_PAN};
	b32 b_ovt;

	pxtnOscillator osci;

	if( ch_num == 2 )
	{
		if( p_vc->pan > _DEFAULT_PAN ) pan_volume[ 0 ] = ( 128 - p_vc->pan );
		if( p_vc->pan < _DEFAULT_PAN ) pan_volume[ 1 ] = (       p_vc->pan );
	}

	osci.ReadyGetSample( p_vc->wave.points, p_vc->wave.num, p_vc->volume, p_vi->smp_body_w, p_vc->wave.reso );
	
	b_ovt = (p_vc->type == VOICETYPE_Overtone) ? _true : _false;

	//  8bit
	if( bps == 8 )
	{
		u8 *p = (u8 *)p_vi->p_smp_w;
		for( s32 s = 0; s < p_vi->smp_body_w; s++ )
		{
			if( b_ovt ) osc = osci.GetOneSample_Overtone ( s );
			else        osc = osci.GetOneSample_Coodinate( s );
			for( s32 c = 0; c < ch_num; c++ )
			{
				work = osc * pan_volume[ c ] / _DEFAULT_PAN;
				pxMem_cap( &work, 1, -1 );
				s32_  = (s32 )( work * MAX_S8BIT );
				p[ s * ch_num + c ] = (u8)(s32_ + 128);
			}
		}
	}
	// 16bit
	else
	{
		s16 *p = (s16*)p_vi->p_smp_w;
		for( s32 s = 0; s < p_vi->smp_body_w; s++ )
		{
			if( b_ovt ) osc = osci.GetOneSample_Overtone ( s );
			else        osc = osci.GetOneSample_Coodinate( s );
			for( s32 c = 0; c < ch_num; c++ )
			{
				work = osc * pan_volume[ c ] / _DEFAULT_PAN;
				pxMem_cap( &work, 1, -1 );
				s32_  = (s32 )( work * MAX_S16BIT );
				p[ s * ch_num + c ] = (s16)s32_;
			}
		}
	}
}

static b32 _Tone_ReadySample( WOICE_STRUCT *p_voice )
{
	b32          b_ret    = _false;
	ptvINSTANCE *p_vi     = NULL ;
	ptvUNIT     *p_vc     = NULL ;
	pcmFORMAT    pcm_work = { 0 };

	for( s32 v = 0; v < p_voice->_voice_num; v++ )
	{
		p_vi = &p_voice->_voinsts[ v ];
		pxMem_free( (void**)&p_vi->p_smp_w );
		p_vi->smp_head_w = 0;
		p_vi->smp_body_w = 0;
		p_vi->smp_tail_w = 0;
	}

	for( s32 i = 0; i < p_voice->_voice_num; i++ )
	{
		p_vc = &p_voice->_voices [ i ];
		p_vi = &p_voice->_voinsts[ i ];

		switch( p_vc->type )
		{
			case VOICETYPE_Coodinate:
			case VOICETYPE_Overtone :
				{
					p_vi->smp_body_w = _DEFAULT_SMP_SIZE;
					s32 size = p_vi->smp_body_w * _DEFAULT_CHANNEL_NUM * _DEFAULT_BPS / 8;
					if( !pxMem_zero_alloc( (void**)&p_vi->p_smp_w, size ) ) goto End;
					_UpdateWavePTV( p_vc, p_vi, _DEFAULT_CHANNEL_NUM, _DEFAULT_SPS, _DEFAULT_BPS );
					break;
				}

			case VOICETYPE_Noise:
				if( !pxtnNoise_Build( &p_vi->p_smp_w, p_vc->p_ptn, _DEFAULT_CHANNEL_NUM, _DEFAULT_SPS, _DEFAULT_BPS ) ) goto End;
				p_vi->smp_body_w = p_vc->p_ptn->smp_num_44k;
				break;

			case VOICETYPE_Sampling:
				if( !pxtnPCM_CopyFrom( &pcm_work, p_vc->p_pcm ) ) goto End;
				if( !pxtnPCM_Convert ( &pcm_work, _DEFAULT_CHANNEL_NUM, _DEFAULT_SPS, _DEFAULT_BPS ) ){ pxMem_free( (void**)&pcm_work._p_smp ); goto End; }

				p_vi->smp_head_w = pcm_work._smp_head;
				p_vi->smp_body_w = pcm_work._smp_body;
				p_vi->smp_tail_w = pcm_work._smp_tail;
				p_vi->p_smp_w    = pcm_work._p_smp;
				pcm_work._p_smp  = NULL;
				break;

			case VOICETYPE_OggVorbis:

#ifdef pxINCLUDE_OGGVORBIS
				extern b32 (* ogg_decode)(oggvUNIT *p_ogg, pcmFORMAT *p_pcm);
				if( !ogg_decode ) goto End;
				if( !ogg_decode( p_vc->p_oggv, &pcm_work ) ) goto End;
				if( !pxtnPCM_Convert( &pcm_work, _DEFAULT_CHANNEL_NUM, _DEFAULT_SPS, _DEFAULT_BPS ) ){ pxMem_free( (void**)&pcm_work._p_smp ); goto End; }

				p_vi->smp_head_w = pcm_work._smp_head;
				p_vi->smp_body_w = pcm_work._smp_body;
				p_vi->smp_tail_w = pcm_work._smp_tail;
				p_vi->p_smp_w    = pcm_work._p_smp;
				pcm_work._p_smp  = NULL;
#else
				goto End; // pxtnERR_ogg_no_supported
#endif
				break;

			default: goto End;
		}
	}

	b_ret = _true;
End:
	if( !b_ret )
	{
		for( s32 v = 0; v < p_voice->_voice_num; v++ )
		{
			p_vi = &p_voice->_voinsts[ v ];
			pxMem_free( (void**)&p_vi->p_smp_w );
			p_vi->smp_head_w = 0;
			p_vi->smp_body_w = 0;
			p_vi->smp_tail_w = 0;
		}
	}

	return b_ret;
}

static b32 _Tone_ReadyEnvelope( WOICE_STRUCT *p_voice )
{
	b32      b_ret   = _false;
	s32      e       =    0 ;
	pxPOINT *p_point = NULL ;

	s32 sps; Streaming_Get_SampleInfo( NULL, (long*)&sps, NULL );

	for( s32 v = 0; v < p_voice->_voice_num; v++ )
	{
		ptvINSTANCE *p_vi   = &p_voice->_voinsts[ v ];
		ptvUNIT     *p_vc   = &p_voice->_voices [ v ];
		ptvENVELOPE *p_enve = &p_vc->envelope;
		s32         size   =               0;
		
		pxMem_free( (void**)&p_vi->p_env );

		if( p_enve->head_num )
		{
			for( e = 0; e < p_enve->head_num; e++ ) size += p_enve->points[ e ].x;
			p_vi->env_size = (s32)( (f64)size * sps / p_enve->fps );
			if( !p_vi->env_size ) p_vi->env_size = 1;

			if( !pxMem_zero_alloc( (void**)&p_vi->p_env, p_vi->env_size                     ) ) goto End;
			if( !pxMem_zero_alloc( (void**)&p_point    , sizeof(pxPOINT) * p_enve->head_num ) ) goto End;

			// convert points.
			s32 offset   = 0;
			s32 head_num = 0;
			for( e = 0; e < p_enve->head_num; e++ )
			{
				if( !e || p_enve->points[ e ].x || p_enve->points[ e ].y )
				{
					offset        += (s32)( (f64)p_enve->points[ e ].x * sps / p_enve->fps );
					p_point[ e ].x = offset;
					p_point[ e ].y =                 p_enve->points[ e ].y;
					head_num++;
				}
			}

			pxPOINT start = {0};
			e = start.x = start.y = 0;
			for( s32 s = 0; s < p_vi->env_size; s++ )
			{
				for( ; e < head_num && s >= p_point[ e ].x; e++ )
				{
					start.x = p_point[ e ].x;
					start.y = p_point[ e ].y;
				}

				if( e < head_num )
				{
					p_vi->p_env[ s ] = (u8)( start.y + ( p_point[ e ].y - start.y ) *
													  (                          s - start.x ) /
													  (             p_point[ e ].x - start.x ) );
				}
				else p_vi->p_env[ s ] = (u8)start.y;
			}

			pxMem_free( (void**)&p_point );
		}

		if( p_enve->tail_num ) p_vi->env_release = (s32)( (f64)p_enve->points[ p_enve->head_num ].x * sps / p_enve->fps );
		else                   p_vi->env_release = 0;
	}

	b_ret = _true;
End:

	pxMem_free( (void**)&p_point );

	if( !b_ret ){ for( s32 v = 0; v < p_voice->_voice_num; v++ ) pxMem_free( (void**)&p_voice->_voinsts[ v ].p_env ); }

	return b_ret;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

b32 pxtnWoice_Initialize( s32 woice_num )
{
	if( !pxMem_zero_alloc( (void**)&_woice_main, woice_num * sizeof(WOICE_STRUCT*) ) ) return _false;
	_woice_num = woice_num;
	return _true;
}

b32 pxtnWoice_Remove( s32 index )
{
	if( index < 0 || index >= _woice_num ) return _false;

	_Voice_Release(      _woice_main[ index ] );
	pxMem_free( (void**)&_woice_main[ index ] );
	_woice_main[ index ] = NULL;
	return _true;
}

void pxtnWoice_RemoveAll( void ){ for( s32 i = 0; i < _woice_num; i++ ) pxtnWoice_Remove( i ); }

void pxtnWoice_Release( void )
{
	pxtnWoice_RemoveAll();
	pxMem_free( (void**)&_woice_main );
	_woice_num = 0;
}

b32 pxtnWoice_Voice_Allocate( WOICE_STRUCT *p_voice, s32 voice_num )
{
	b32 b_ret = _false;

	_Voice_Release( p_voice );

	if( !pxMem_zero_alloc( (void**)&p_voice->_voices , sizeof(ptvUNIT    ) * voice_num ) ) goto End;
	if( !pxMem_zero_alloc( (void**)&p_voice->_voinsts, sizeof(ptvINSTANCE) * voice_num ) ) goto End;
	p_voice->_voice_num = voice_num;

	b_ret = _true;
End:
	if( !b_ret ) _Voice_Release( p_voice );

	return b_ret;
}

void pxtnWoice_Voice_Init( ptvUNIT *p_vc, VOICETYPE type )
{
	p_vc->basic_key   = EVENTDEFAULT_BASICKEY;
	p_vc->volume      = 128;
	p_vc->pan         = _DEFAULT_PAN;
	p_vc->tuning      = 1.0f;
	p_vc->voice_flags = PTV_VOICEFLAG_SMOOTH;
	p_vc->data_flags  = PTV_DATAFLAG_WAVE;
	p_vc->type        = type;
	
	if( !pxMem_zero_alloc( (void**)&p_vc->p_pcm , sizeof(pcmFORMAT) ) ) return;
	if( !pxMem_zero_alloc( (void**)&p_vc->p_ptn , sizeof(ptnDESIGN) ) ) return;
#ifdef  pxINCLUDE_OGGVORBIS
	if( !pxMem_zero_alloc( (void**)&p_vc->p_oggv, sizeof(oggvUNIT ) ) ) return;
#endif

	memset( &p_vc->envelope, 0, sizeof(ptvENVELOPE) );
	memset( &p_vc->wave    , 0, sizeof(ptvWAVE    ) );
}

s32 pxtnWoice_Add( void )
{
	b32 b_ret = _false;

	s32 index = _Get_EmptySlot();
	if( index < 0 ) goto End;
	if( !pxMem_zero_alloc( (void**)&_woice_main[ index ], sizeof(WOICE_STRUCT) ) ) goto End;

	sprintf( _woice_main[ index ]->_name, "voice_%02d", index );
	_woice_main[ index ]->_name_size = strlen( _woice_main[ index ]->_name );
	b_ret = _true;
End:
	if( !b_ret ){ pxtnWoice_Remove( index ); return -1; }

	return index;
}

s32 pxtnWoice_Read( const char *path, b32 pbNew, WOICE_STRUCT *p_voice )
{
	b32           b_ret = _false;
	
	s32           index;
	WOICE_STRUCT *p_new = NULL;

	DDV           read ;
	memset( &read, 0, sizeof(DDV) );

	if( !p_voice )
	{
		index = _Get_EmptySlot();
		if( index < 0 ) goto End;
		if( !pxMem_zero_alloc( (void**)&_woice_main[ index ], sizeof(WOICE_STRUCT) ) ) goto End;

		p_new = _woice_main[ index ];
		sprintf( p_new->_name, "voice_%02d", index );
		p_new->_name_size = strlen( p_new->_name );
	}
	else
	{
		p_new = p_voice;
		index = _Get_Index( p_voice );
		if( index < 0 ) goto End;
	}

	switch( _Get_Type( path ) )
	{
		// PCM
		case WOICE_PCM:
			{
				if( !pxtnWoice_Voice_Allocate( p_new, 1 ) ) goto End;
				pxtnWoice_Voice_Init( p_new->_voices, VOICETYPE_Sampling );

				ptvUNIT *p_vc = &p_new->_voices[ 0 ];
				if( !pxtnPCM_Read( path, p_vc->p_pcm ) ) goto End;

				// if under 0.005 sec, set LOOP.
				f32 sec =   (f32)( p_vc->p_pcm->_smp_body +
								   p_vc->p_pcm->_smp_head +
								   p_vc->p_pcm->_smp_tail ) /
							(f32)  p_vc->p_pcm->_sps;
				if( sec < 0.005f ) p_vc->voice_flags |=  PTV_VOICEFLAG_WAVELOOP;
				else               p_vc->voice_flags &= ~PTV_VOICEFLAG_WAVELOOP;

				p_new->_type = WOICE_PCM;
			}
			break;

		// PTV
		case WOICE_PTV:
			if( !ddv_Open         (  NULL, path , NULL, &read ) ) goto End;
			if( !pxtnWoicePTV_Read( &read, p_new, &pbNew      ) ) goto End;
			break;


		// PTN
		case WOICE_PTN:
			if( !ddv_Open ( NULL, path, NULL, &read ) ) goto End;
			if( !pxtnWoice_Voice_Allocate( p_new, 1 ) ) goto End;
			pxtnWoice_Voice_Init( p_new->_voices, VOICETYPE_Noise );

			if( !pxtnNoise_Design_Read( &read, p_new->_voices->p_ptn, &pbNew ) ) goto End;
			p_new->_type = WOICE_PTN;
			break;

		// OGGV
		case WOICE_OGGV:
#ifdef  pxINCLUDE_OGGVORBIS
			if( !pxtnWoice_Voice_Allocate( p_new, 1 ) ) goto End;
			pxtnWoice_Voice_Init( p_new->_voices, VOICETYPE_OggVorbis );

			extern oggvUNIT *(* ogg_load )( const char *path);
			if( !ogg_load ) goto End;
			if( !(p_new->_voices->p_oggv = ogg_load( path )) ) goto End;
			p_new->_type = WOICE_OGGV;
#else
			goto End; // pxtnERR_ogg_no_supported
#endif
			break;

		default: goto End;
	}

	b_ret = _true;
End:
	ddv_Close( &read );
	if( !b_ret ){ pxtnWoice_Remove( index ); index = -1; }

	return index;
}

s32 pxtnWoice_Duplicate( const WOICE_STRUCT *p_voice )
{
	b32 b_ret = _false;

	s32 index = _Get_EmptySlot();
	if( index < 0 ) return -1;
	if( !pxMem_zero_alloc( (void**)&_woice_main[ index ], sizeof(WOICE_STRUCT) ) ) goto End;
	
	WOICE_STRUCT *voice_dst = _woice_main[ index ];
	sprintf( voice_dst->_name, "voice_%02d", index );
	voice_dst->_name_size = strlen( voice_dst->_name );

	if( !_Copy( p_voice, voice_dst ) ) goto End;

	b_ret = _true;
End:
	if( !b_ret ){ pxtnWoice_Remove( index ); index = -1; }

	return index;
}

void pxtnWoice_BuildPTV( WOICE_STRUCT *p_voice )
{
	for( s32 v = 0; v < p_voice->_voice_num; v++ )
	{
		ptvINSTANCE *p_vi = &p_voice->_voinsts[ v ];
		ptvUNIT     *p_vc = &p_voice->_voices [ v ];

		if( p_vc->type == VOICETYPE_Overtone || p_vc->type == VOICETYPE_Coodinate )
		{
			p_vi->smp_body_w = _DEFAULT_SMP_SIZE;
			s32 size = p_vi->smp_body_w * _DEFAULT_CHANNEL_NUM * _DEFAULT_BPS;
			memset( p_vi->p_smp_w, 0, size / 8 );
			_UpdateWavePTV( p_vc, p_vi, _DEFAULT_CHANNEL_NUM, _DEFAULT_SPS, _DEFAULT_BPS );
		}
	}
}

b32 pxtnWoice_Tone_Ready( s32 index )
{
	if( index < 0 || index >= _woice_num ) return _false;

	WOICE_STRUCT *p_w = _woice_main[ index ];
	if( !p_w ) return _false;

	if( !_Tone_ReadySample  ( p_w ) ) return _false;
	if( !_Tone_ReadyEnvelope( p_w ) ) return _false;
	return _true;
}

s32 pxtnWoice_Count( void )
{
	s32 count = 0;
	for( s32 i = 0; i < _woice_num; i++ ){ if( _woice_main[ i ] ) count++; }
	return count;
}

b32 pxtnWoice_ReadyTone( void )
{
	for( s32 i = 0; i < _woice_num; i++ )
	{
		if( _woice_main[ i ] )
		{
			if( !_Tone_ReadySample  ( _woice_main[ i ] ) ) return _false;
			if( !_Tone_ReadyEnvelope( _woice_main[ i ] ) ) return _false;
		}
	}
	return _true;
}

s32 pxtnWoice_Get_NumMax( void ){ return _woice_num; }

void pxtnWoice_Replace( s32 old_place, s32 new_place )
{
	WOICE_STRUCT *p_w = _woice_main[ old_place ];
	s32 max_place = pxtnWoice_Count() - 1;

	if( new_place >  max_place ) new_place = max_place;
	if( new_place == old_place ) return;

	if( old_place < new_place )
	{
		for( s32 w = old_place; w < new_place; w++ ){ if( _woice_main[ w ] ) _woice_main[ w ] = _woice_main[ w + 1 ]; }
	}
	else
	{
		for( s32 w = old_place; w > new_place; w-- ){ if( _woice_main[ w ] ) _woice_main[ w ] = _woice_main[ w - 1 ]; }
	}
	_woice_main[ new_place ] = p_w;
}

WOICE_STRUCT *pxtnWoice_GetVariable( s32 index )
{
	if( index < 0 || index >= _woice_num ) return NULL;
	return _woice_main[ index ];
}

WOICE_STRUCT *pxtnWoice_Create( void )
{
	s32 index = _Get_EmptySlot();

	if( index < 0 ) return NULL;
	if( !pxMem_zero_alloc( (void**)&_woice_main[ index ], sizeof(WOICE_STRUCT) ) ) return NULL;

	sprintf( _woice_main[ index ]->_name, "voice_%02d", index );
	_woice_main[ index ]->_name_size = strlen( _woice_main[ index ]->_name );
	return _woice_main[ index ];
}
