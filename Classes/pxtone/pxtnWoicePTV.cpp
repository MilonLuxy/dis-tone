#include <StdAfx.h>


#include "../Fixture/DataDevice.h"
#include "../Fixture/pxMem.h"
#include "pxtnWoice.h"

//                           01234567
static const char *_code  = "PTVOICE-";
//                 _ver_0 =  20050826 ; // -v.0.2.3.x (?)
//                 _ver_1 =  20051101 ; // support coodinate
static u32         _ver_2 =  20060111 ; // support no-envelope


////////////////////////
// privates..
////////////////////////

static b32 _Write_Wave( FILE *fp, const ptvUNIT *p_vc, s32 *p_total )
{
	b32 b_ret = _false;
	s32 num, i, size;
	s8  sc = 0;
	u8  uc = 0;

	if( !ddv_Variable_Write( p_vc->type, fp, p_total ) ) goto term;

	switch( p_vc->type )
	{
	// Coodinate (3)
	case VOICETYPE_Coodinate:

		if( !ddv_Variable_Write( p_vc->wave.num , fp, p_total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->wave.reso, fp, p_total ) ) goto term;
		num = p_vc->wave.num;
		for( i = 0; i < num; i++ )
		{
			uc = (u8)p_vc->wave.points[ i ].x; if( fwrite( &uc, 1, 1, fp ) != 1 ) goto term; (*p_total)++;
			sc = (s8)p_vc->wave.points[ i ].y; if( fwrite( &sc, 1, 1, fp ) != 1 ) goto term; (*p_total)++;
		}
		break;

	// Overtone (2)
	case VOICETYPE_Overtone:

		if( !ddv_Variable_Write( p_vc->wave.num, fp, p_total ) ) goto term;
		num = p_vc->wave.num;
		for( i = 0; i < num; i++ )
		{
			if( !ddv_Variable_Write( p_vc->wave.points[ i ].x, fp, p_total ) ) goto term;
			if( !ddv_Variable_Write( p_vc->wave.points[ i ].y, fp, p_total ) ) goto term;
		}
		break;

	// sampling (7)
	case VOICETYPE_Sampling:
		if( !ddv_Variable_Write( p_vc->p_pcm->_ch      , fp, p_total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->p_pcm->_bps     , fp, p_total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->p_pcm->_sps     , fp, p_total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->p_pcm->_smp_head, fp, p_total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->p_pcm->_smp_body, fp, p_total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->p_pcm->_smp_tail, fp, p_total ) ) goto term;

		size = ( p_vc->p_pcm->_smp_head + p_vc->p_pcm->_smp_body + p_vc->p_pcm->_smp_tail ) *
				 p_vc->p_pcm->_ch       * p_vc->p_pcm->_bps      / 8;

		if( fwrite( p_vc->p_pcm->_p_smp, 1, size, fp ) != size ) goto term;
		*p_total += size;
		break;
			
		case VOICETYPE_OggVorbis: goto term; // not support.
	}

	b_ret = _true;
term:

	return b_ret;
}

static b32 _Write_Envelope( FILE *fp, const ptvUNIT *p_vc, s32 *p_total )
{
	b32 b_ret = _false;

	// envelope. (5)
	if( !ddv_Variable_Write( p_vc->envelope.fps     , fp, p_total ) ) goto term;
	if( !ddv_Variable_Write( p_vc->envelope.head_num, fp, p_total ) ) goto term;
	if( !ddv_Variable_Write( p_vc->envelope.body_num, fp, p_total ) ) goto term;
	if( !ddv_Variable_Write( p_vc->envelope.tail_num, fp, p_total ) ) goto term;

	s32 num = p_vc->envelope.head_num + p_vc->envelope.body_num + p_vc->envelope.tail_num;
	for( s32 i = 0; i < num; i++ )
	{
		if( !ddv_Variable_Write( p_vc->envelope.points[ i ].x, fp, p_total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->envelope.points[ i ].y, fp, p_total ) ) goto term;
	}

	b_ret = _true;
term:

	return b_ret;
}



static b32 _Read_Wave( DDV *p_read, ptvUNIT *p_vc )
{
	s32 num;
	s8  sc;
	u8  uc;

	if( !ddv_Variable_Read( (s32*)&p_vc->type, p_read ) ) return _false;

	switch( p_vc->type )
	{
	// coodinate (3)
	case VOICETYPE_Coodinate:
		if( !ddv_Variable_Read( &p_vc->wave.num , p_read ) ) return _false;
		if( !ddv_Variable_Read( &p_vc->wave.reso, p_read ) ) return _false;
		num = p_vc->wave.num;

		if( !pxMem_zero_alloc( (void**)&p_vc->wave.points, sizeof(pxPOINT) * num ) ) return _false;
		for( s32 i = 0; i < num; i++ )
		{
			if( !ddv_Read( &uc, 1, 1, p_read ) ) return _false; p_vc->wave.points[ i ].x = uc;
			if( !ddv_Read( &sc, 1, 1, p_read ) ) return _false; p_vc->wave.points[ i ].y = sc;
		}
		num = p_vc->wave.num;
		break;
	// overtone (2)
	case VOICETYPE_Overtone:

		if( !ddv_Variable_Read( &p_vc->wave.num, p_read ) ) return _false;
		num = p_vc->wave.num;

		if( !pxMem_zero_alloc( (void**)&p_vc->wave.points, sizeof(pxPOINT) * num ) ) return _false;
		for( s32 i = 0; i < num; i++ )
		{
			if( !ddv_Variable_Read( &p_vc->wave.points[ i ].x, p_read ) ) return _false;
			if( !ddv_Variable_Read( &p_vc->wave.points[ i ].y, p_read ) ) return _false;
		}
		break;

	// p_vc->sampring. (7)
	case VOICETYPE_Sampling: return _false; // un-support

		//if( !ddv_Variable_Read( &p_vc->p_pcm->_ch      , p_read ) ) goto term;
		//if( !ddv_Variable_Read( &p_vc->p_pcm->_bps     , p_read ) ) goto term;
		//if( !ddv_Variable_Read( &p_vc->p_pcm->_sps     , p_read ) ) goto term;
		//if( !ddv_Variable_Read( &p_vc->p_pcm->_smp_head, p_read ) ) goto term;
		//if( !ddv_Variable_Read( &p_vc->p_pcm->_smp_body, p_read ) ) goto term;
		//if( !ddv_Variable_Read( &p_vc->p_pcm->_smp_tail, p_read ) ) goto term;
		//size = ( p_vc->p_pcm->_smp_head + p_vc->p_pcm->_smp_body + p_vc->p_pcm->_smp_tail ) * p_vc->p_pcm->_ch * p_vc->p_pcm->_bps / 8;
		//if( !pxMem_zero_alloc( (void**)&p_vc->p_pcm->_p_smp,    size )         ) goto term;
		//if( !ddv_Read_File(             p_vc->p_pcm->_p_smp, 1, size, p_read ) ) goto term;
		//break;

	default: return _false; // un-support
	}

	return _true;
}

static b32 _Read_Envelope( DDV *p_read, ptvUNIT *p_vc )
{
	b32 b_ret = _false;
	s32 num, i;

	//p_vc->envelope. (5)
	if( !ddv_Variable_Read( &p_vc->envelope.fps     , p_read ) ) goto term;
	if( !ddv_Variable_Read( &p_vc->envelope.head_num, p_read ) ) goto term;
	if( !ddv_Variable_Read( &p_vc->envelope.body_num, p_read ) ) goto term;
	if( !ddv_Variable_Read( &p_vc->envelope.tail_num, p_read ) ) goto term;
	if( p_vc->envelope.body_num                                ) goto term;
	if( p_vc->envelope.tail_num != 1                           ) goto term;

	num = p_vc->envelope.head_num + p_vc->envelope.body_num + p_vc->envelope.tail_num;
	if( !pxMem_zero_alloc( (void**)&p_vc->envelope.points, sizeof(pxPOINT) * num ) ) goto term;
	for( i = 0; i < num; i++ )
	{
		if( !ddv_Variable_Read( &p_vc->envelope.points[ i ].x, p_read ) ) goto term;
		if( !ddv_Variable_Read( &p_vc->envelope.points[ i ].y, p_read ) ) goto term;
	}

	b_ret = _true;
term:
	if( !b_ret ) pxMem_free( (void**)&p_vc->envelope.points );

	return b_ret;
}



////////////////////////
// publics..
////////////////////////

b32 pxtnWoicePTV_Write( FILE *fp, s32 *p_total, WOICE_STRUCT *p_voice )
{
	if( !fp || !p_voice ) return _false;

	b32      b_ret = _false;
	ptvUNIT *p_vc  = NULL;
	u32      work  =    0;
	s32      v     =    0;
	s32      total =    0;

	if( fwrite( _code  ,     1, 8, fp    ) != 8 ) goto term;
	if( fwrite( &_ver_2,     4, 1, fp    ) != 1 ) goto term;
	if( fwrite( &total ,     4, 1, fp    ) != 1 ) goto term;

	work = 0;

	// p_ptv-> (5)
	if( !ddv_Variable_Write( work, fp, &total ) ) goto term; // basic_key (no use)
	if( !ddv_Variable_Write( work, fp, &total ) ) goto term;
	if( !ddv_Variable_Write( work, fp, &total ) ) goto term;
	if( !ddv_Variable_Write( p_voice->_voice_num, fp, &total ) ) goto term;

	for( v = 0; v < p_voice->_voice_num; v++ )
	{
		// p_ptvv-> (9)
		if( !(          p_vc = &p_voice->_voices[ v ]          ) ) goto term;
		if( !ddv_Variable_Write( p_vc->basic_key  , fp, &total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->volume     , fp, &total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->pan        , fp, &total ) ) goto term;
		memcpy( &work, &p_vc->tuning, 4                          );
		if( !ddv_Variable_Write( work             , fp, &total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->voice_flags, fp, &total ) ) goto term;
		if( !ddv_Variable_Write( p_vc->data_flags , fp, &total ) ) goto term;

		if( p_vc->data_flags & PTV_DATAFLAG_WAVE     && !_Write_Wave    ( fp, p_vc, &total ) ) goto term;
		if( p_vc->data_flags & PTV_DATAFLAG_ENVELOPE && !_Write_Envelope( fp, p_vc, &total ) ) goto term;
	}

	// total size
	if( fseek ( fp, -(total + 4), SEEK_CUR  ) != 0 ) goto term;
	if( fwrite( &total, sizeof(s32), 1, fp ) != 1 ) goto term;
	if( fseek ( fp,  (total    ), SEEK_CUR  ) != 0 ) goto term;

	if( p_total ) *p_total = 16 + total;
	b_ret  = _true;
term:
	
	return b_ret;
}

b32 pxtnWoicePTV_OpenWrite( const char *file_name, WOICE_STRUCT *p_voice )
{
	FILE *fp = NULL;
	if( !(fp = fopen( file_name, "wb" )) ) return _false;
	b32 b_ret = pxtnWoicePTV_Write( fp, NULL, p_voice );
	fclose( fp );

	return b_ret;
}

b32 pxtnWoicePTV_Read( DDV *p_read, WOICE_STRUCT *p_voice, b32 *pbNew )
{
	if( !p_read || !p_voice ) return _false;

	b32 b_ret = _false;
	s32 work1, work2;
	s32 unit_num    ;
	s32 total       ;
	u32 ver         ;
	
	ptvUNIT *p_vc   ;

	char code[8];

	if( !ddv_Read(  code ,     1, 8, p_read              ) ) goto term;
	if( !ddv_Read( &ver  ,     4, 1, p_read              ) ) goto term;
	if( memcmp   (  code , _code, 8                      ) ) goto term;
	if( !ddv_Read( &total,     4, 1, p_read              ) ) goto term;
	if( ver > _ver_2                      ){ *pbNew = _true; goto term; }

	// p_ptv-> (5)
	if( !ddv_Variable_Read( &p_voice->_x3x_basic_key, p_read ) ) goto term;
	if( !ddv_Variable_Read( &work1                  , p_read ) ) goto term;
	if( !ddv_Variable_Read( &work2                  , p_read ) ) goto term;
	if(  work1 || work2                       ){ *pbNew = _true; goto term; }
	if( !ddv_Variable_Read( &unit_num               , p_read ) ) goto term;
	if( !pxtnWoice_Voice_Allocate( p_voice,         unit_num ) ) goto term;

	for( s32 v = 0; v < p_voice->_voice_num; v++ )
	{
		// p_ptvv-> (8)
		if( !(   p_vc = &p_voice->_voices[ v ]                   ) ) goto term;
		if( !ddv_Variable_Read(       &p_vc->basic_key  , p_read ) ) goto term;
		if( !ddv_Variable_Read(       &p_vc->volume     , p_read ) ) goto term;
		if( !ddv_Variable_Read(       &p_vc->pan        , p_read ) ) goto term;
		if( !ddv_Variable_Read(       &work1            , p_read ) ) goto term;
		memcpy( &p_vc->tuning, &work1, 4                            );
		if( !ddv_Variable_Read( (s32*)&p_vc->voice_flags, p_read ) ) goto term;
		if( !ddv_Variable_Read( (s32*)&p_vc->data_flags , p_read ) ) goto term;

		// no support.
		if( p_vc->voice_flags & PTV_VOICEFLAG_UNCOVERED ){ *pbNew = _true; goto term; }
		if( p_vc->data_flags  & PTV_DATAFLAG_UNCOVERED  ){ *pbNew = _true; goto term; }
		if( p_vc->data_flags  & PTV_DATAFLAG_WAVE       ){ if( !_Read_Wave    ( p_read, p_vc ) ) goto term; }
		if( p_vc->data_flags  & PTV_DATAFLAG_ENVELOPE   ){ if( !_Read_Envelope( p_read, p_vc ) ) goto term; }
	}
	p_voice->_type = WOICE_PTV;

	b_ret = _true;
term:

	return b_ret;
}
