#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "pxtnPulse_PCM.h"


////////////////////////////
// ローカル関数 ////////////
////////////////////////////

// stereo / mono
static b32 _Convert_ChannelNum( pcmFORMAT *p_pcm, s32 new_ch )
{
	if( p_pcm->_p_smp == NULL   ) return _false;
	if( p_pcm->_ch    == new_ch ) return _true ;
	
	u8  *p_work = NULL;
	s32  work_size;
	s32  a,b;
	s32  temp1;
	s32  temp2;

	s32  sample_size = ( p_pcm->_smp_head + p_pcm->_smp_body + p_pcm->_smp_tail ) *
						p_pcm->_ch       * p_pcm->_bps      / 8;


	// mono to stereo --------
	if( new_ch == 2 )
	{
		work_size = sample_size * 2;
		if( !(p_work = (u8 *)malloc( work_size )) ) return _false;

		if( p_pcm->_bps == 8 )
		{
			b = 0;
			for( a = 0; a < sample_size; a++ )
			{
				p_work[b  ] = p_pcm->_p_smp[a];
				p_work[b+1] = p_pcm->_p_smp[a];
				b += 2;
			}
		}
		else if( p_pcm->_bps == 16 )
		{
			b = 0;
			for( a = 0; a < sample_size; a += 2 )
			{
				p_work[b  ] = p_pcm->_p_smp[a  ];
				p_work[b+1] = p_pcm->_p_smp[a+1];
				p_work[b+2] = p_pcm->_p_smp[a  ];
				p_work[b+3] = p_pcm->_p_smp[a+1];
				b += 4;
			}
		}
	}
	// stereo to mono --------
	else
	{
		work_size = sample_size / 2;
		if( !(p_work = (u8 *)malloc( work_size )) ) return _false;

		if( p_pcm->_bps == 8 )
		{
			b = 0;
			for( a = 0; a < sample_size; a += 2 )
			{
				temp1       = (s32)p_pcm->_p_smp[a] + (s32)p_pcm->_p_smp[a+1];
				p_work[b  ] = (u8 )( temp1 / 2 );
				b++;
			}
		}
		else if( p_pcm->_bps == 16 )
		{
			b = 0;
			for( a = 0; a < sample_size; a += 4 )
			{
				temp1                 = *( (s16*)(&p_pcm->_p_smp[a  ]) );
				temp2                 = *( (s16*)(&p_pcm->_p_smp[a+2]) );
				*(s16*)( &p_work[b] ) =    (s16 )( ( temp1 + temp2 )/2 );
				b += 2;
			}
		}
	}

	// release once.
	pxMem_free( (void**)&p_pcm->_p_smp );

	if( !(p_pcm->_p_smp = (u8 *)malloc( work_size )) ){ free( p_work ); return _false; }
	memcpy( p_pcm->_p_smp, p_work, work_size );
	free( p_work );

	// update param.
	p_pcm->_ch = new_ch;

	return _true;
}

// change bps
static b32 _Convert_BitPerSample( pcmFORMAT *p_pcm, s32 new_bps )
{
	if( p_pcm->_p_smp == NULL    ) return _false;
	if( p_pcm->_bps   == new_bps ) return _true ;

	u8  *p_work;
	s32  work_size;
	s32  a,b;
	s32  temp1;

	s32  sample_size = ( p_pcm->_smp_head + p_pcm->_smp_body + p_pcm->_smp_tail ) *
						p_pcm->_ch       * p_pcm->_bps      / 8;


	// 16 to 8 --------
	if( new_bps == 8 )
	{
		work_size = sample_size / 2;
		if( !(p_work = (u8 *)malloc( work_size )) ) return _false;

		b = 0;
		for( a = 0; a < sample_size; a += 2 )
		{
			temp1 = *( (s16*)(&p_pcm->_p_smp[a]) );
			temp1 =  (temp1/0x100) + 128;
			p_work[b] = (u8 )temp1;
			b++;
		}
	}
	//  8 to 16 --------
	else if( new_bps == 16 )
	{
		work_size = sample_size * 2;
		if( !(p_work = (u8 *)malloc( work_size )) ) return _false;

		b = 0;
		for( a = 0; a < sample_size; a++ )
		{
			temp1 = p_pcm->_p_smp[a];
			temp1 = ( temp1 - 128 ) * 0x100;
			*((s16*)(&p_work[b])) = (s16)temp1;
			b += 2;
		}
	}
	else return _false;

	// release once.
	pxMem_free( (void**)&p_pcm->_p_smp );

	if( !(p_pcm->_p_smp = (u8 *)malloc( work_size )) ){ free( p_work ); return _false; }
	memcpy( p_pcm->_p_smp, p_work, work_size );
	free( p_work );

	// update param.
	p_pcm->_bps = new_bps;

	return _true;
}

// sps
static b32 _Convert_SamplePerSecond( pcmFORMAT *p_pcm, s32 new_sps )
{
	if( p_pcm->_p_smp == NULL    ) return _false;
	if( p_pcm->_sps   == new_sps ) return _true ;
	
	b32  b_ret = _false;
	s32  a, b, sample_num;

	u8  *p1byte_data;
	u16 *p2byte_data;
	u32 *p4byte_data;

	u8  *p1byte_work = NULL;
	u16 *p2byte_work = NULL;
	u32 *p4byte_work = NULL;


	s32  head_size = p_pcm->_smp_head * p_pcm->_ch * p_pcm->_bps / 8;
	s32  body_size = p_pcm->_smp_body * p_pcm->_ch * p_pcm->_bps / 8;
	s32  tail_size = p_pcm->_smp_tail * p_pcm->_ch * p_pcm->_bps / 8;

	head_size = (s32)( ( (f64)head_size * (f64)new_sps + (f64)(p_pcm->_sps) - 1 ) / p_pcm->_sps );
	body_size = (s32)( ( (f64)body_size * (f64)new_sps + (f64)(p_pcm->_sps) - 1 ) / p_pcm->_sps );
	tail_size = (s32)( ( (f64)tail_size * (f64)new_sps + (f64)(p_pcm->_sps) - 1 ) / p_pcm->_sps );

	s32  work_size = head_size + body_size + tail_size;

	// stereo 16bit ========
	if( p_pcm->_ch == 2 && p_pcm->_bps == 16 )
	{
		p_pcm->_smp_head = head_size  / 4;
		p_pcm->_smp_body = body_size  / 4;
		p_pcm->_smp_tail = tail_size  / 4;
		sample_num       = work_size  / 4;
		work_size        = sample_num * 4;
		p4byte_data      = (u32*)p_pcm->_p_smp;

		if( !pxMem_zero_alloc( (void**)&p4byte_work, work_size ) ) goto End;
		for( a = 0; a < sample_num; a++ )
		{
			b = (s32)( (f64)a * (f64)(p_pcm->_sps) / (f64)new_sps );
			p4byte_work[a] = p4byte_data[b];
		}
	}
	// mono 8bit ========
	else if( p_pcm->_ch == 1 && p_pcm->_bps == 8 )
	{
		p_pcm->_smp_head = head_size  / 1;
		p_pcm->_smp_body = body_size  / 1;
		p_pcm->_smp_tail = tail_size  / 1;
		sample_num       = work_size  / 1;
		work_size        = sample_num * 1;
		p1byte_data      = (u8*)p_pcm->_p_smp;

		if( !pxMem_zero_alloc( (void**)&p1byte_work, work_size ) ) goto End;
		for( a = 0; a < sample_num; a++ )
		{
			b = (s32)( (f64)a * (f64)(p_pcm->_sps) / (f64)(new_sps) );
			p1byte_work[a] = p1byte_data[b];
		}
	}
	// mono 16bit / stereo 8bit ========
	else
	{
		p_pcm->_smp_head = head_size  / 2;
		p_pcm->_smp_body = body_size  / 2;
		p_pcm->_smp_tail = tail_size  / 2;
		sample_num       = work_size  / 2;
		work_size        = sample_num * 2;
		p2byte_data      = (u16*)p_pcm->_p_smp;

		if( !pxMem_zero_alloc( (void**)&p2byte_work, work_size ) ) goto End;
		for( a = 0; a < sample_num; a++ )
		{
			b = (s32)( (f64)a * (f64)(p_pcm->_sps) / (f64)new_sps );
			p2byte_work[a] = p2byte_data[b];
		}
	}

	// release once.
	pxMem_free( (void**)&p_pcm->_p_smp );
	
	if( !(p_pcm->_p_smp = (u8 *)malloc( work_size )) ) goto End;
	
	if(      p4byte_work ) memcpy( p_pcm->_p_smp, p4byte_work, work_size );
	else if( p2byte_work ) memcpy( p_pcm->_p_smp, p2byte_work, work_size );
	else if( p1byte_work ) memcpy( p_pcm->_p_smp, p1byte_work, work_size );
	else goto End;

	// update.
	p_pcm->_sps = new_sps;

	b_ret = _true;
End:
	if( !b_ret )
	{
		pxMem_free( (void**)&p_pcm->_p_smp );
		p_pcm->_smp_head = 0;
		p_pcm->_smp_body = 0;
		p_pcm->_smp_tail = 0;
	}

	pxMem_free( (void**)&p2byte_work );
	pxMem_free( (void**)&p1byte_work );
	pxMem_free( (void**)&p4byte_work );

	return b_ret;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

b32 pxtnPCM_Read( const char *file_name, pcmFORMAT *p_pcm )
{
	b32           b_ret     = _false;
	char          buf[ 16 ] = { 0 };
	u32           size      =   0  ;
	WAVEFORMATEX  p_format  = { 0 };
	FILE         *fp        = NULL ;

	p_pcm->_p_smp = NULL;
	memset( &p_format, 0, sizeof(WAVEFORMATEX) );
	
	// 'RIFFxxxxWAVEfmt '
	if( !(fp = fopen(file_name, "rb")) ) goto End;
	if( fread( buf, sizeof(char), 16, fp ) != 16 ) goto End;
	
	if( buf[ 0] != 'R' || buf[ 1] != 'I' || buf[ 2] != 'F' || buf[ 3] != 'F' ||
		buf[ 8] != 'W' || buf[ 9] != 'A' || buf[10] != 'V' || buf[11] != 'E' ||
		buf[12] != 'f' || buf[13] != 'm' || buf[14] != 't' || buf[15] != ' ' ) goto End;

	// フォーマットを読み込む
	if( fread( &size    , sizeof(u32         ), 1, fp ) != 1 ) goto End;
	if( fread( &p_format, sizeof(WAVEFORMATEX), 1, fp ) != 1 ) goto End;

	if( p_format.wFormatTag     != WAVE_FORMAT_PCM                    ) goto End;
	if( p_format.nChannels      != 1 && p_format.nChannels      !=  2 ) goto End;
	if( p_format.wBitsPerSample != 8 && p_format.wBitsPerSample != 16 ) goto End;


	// 'fmt ' 以降に 'data' を探す
	p_pcm->_ch  = p_format.nChannels;
	p_pcm->_sps = p_format.nSamplesPerSec;
	p_pcm->_bps = p_format.wBitsPerSample;
	fseek( fp, 12, SEEK_SET ); // skip 'RIFFxxxxWAVE'

	while( 1 )
	{
		if( fread( &buf , sizeof(char), 4, fp ) != 4 ) goto End;
		if( fread( &size, sizeof(u32 ), 1, fp ) != 1 ) goto End;
		if( buf[0] == 'd' && buf[1] == 'a' && buf[2] == 't' && buf[3] == 'a' ) break;
		fseek( fp, size, SEEK_CUR );
	}

	p_pcm->_smp_head = 0;
	p_pcm->_smp_body = size * 8 / p_format.wBitsPerSample / p_format.nChannels;
	p_pcm->_smp_tail = 0;
	if( !(p_pcm->_p_smp = (u8 *)malloc( size )) ) goto End;

	if( fread( p_pcm->_p_smp, sizeof(u8), size, fp ) != size ) goto End;

	b_ret = _true;
End:
	if( fp ) fclose( fp );
	if( !b_ret ) pxMem_free( (void**)&p_pcm->_p_smp );
	return b_ret;
}

b32 pxtnPCM_Write( const char *file_name, pcmFORMAT *p_pcm, const char *pstrLIST )
{
	if( !p_pcm->_p_smp ) return _false;

	b32  b_ret = _false;
	b32  bText = _false; // for LIST

	u32  riff_size;
	u32  fact_size; // num sample.
	u32  list_size; // num list text.
	u32  isft_size;

	char tag_RIFF[4] = {'R','I','F','F'};
	char tag_WAVE[4] = {'W','A','V','E'};
	char tag_fmt_[8] = {'f','m','t',' ', 0x12,0,0,0};
	char tag_fact[8] = {'f','a','c','t', 0x04,0,0,0};
	char tag_data[4] = {'d','a','t','a'};
	char tag_LIST[4] = {'L','I','S','T'};
	char tag_INFO[8] = {'I','N','F','O','I','S','F','T'};


	if( pstrLIST && strlen( pstrLIST ) ) bText = _true ;
	else                                 bText = _false;
	
	u32 sample_size = ( p_pcm->_smp_head + p_pcm->_smp_body + p_pcm->_smp_tail ) * p_pcm->_ch * p_pcm->_bps / 8;

	WAVEFORMATEX format;
	format.cbSize          = 0;
	format.wFormatTag      = WAVE_FORMAT_PCM ;
	format.nChannels       = (u16)p_pcm->_ch ;
	format.nSamplesPerSec  =      p_pcm->_sps;
	format.wBitsPerSample  = (u16)p_pcm->_bps;
	format.nBlockAlign     = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;

	fact_size  = ( p_pcm->_smp_head + p_pcm->_smp_body + p_pcm->_smp_tail );
	riff_size  = sample_size;
	riff_size +=  4; // 'WAVE'
	riff_size += 26; // 'fmt '
	riff_size += 12; // 'fact'
	riff_size +=  8; // 'data'

	if( bText )
	{
		isft_size = strlen( pstrLIST );
		list_size = 4 + 4 + 4 + isft_size; // "INFO" + "ISFT" + size + ver_Text;
		riff_size +=  8 + list_size; // 'LIST'
	}
	else
	{
		isft_size = 0;
		list_size = 0;
	}

	// open file..
	FILE *fp = NULL;
	if( !(fp = fopen( file_name, "wb" )) ) goto End;

	if( fwrite( &tag_RIFF     , sizeof(char        ), 4, fp ) != 4 ) goto End;
	if( fwrite( &riff_size    , sizeof(u32         ), 1, fp ) != 1 ) goto End;
	if( fwrite( &tag_WAVE     , sizeof(char        ), 4, fp ) != 4 ) goto End;
	if( fwrite( &tag_fmt_     , sizeof(char        ), 8, fp ) != 8 ) goto End;
	if( fwrite( &format       , sizeof(WAVEFORMATEX), 1, fp ) != 1 ) goto End;

	if( bText )
	{
		if( fwrite( &tag_LIST , sizeof(char        ), 4, fp ) != 4 ) goto End;
		if( fwrite( &list_size, sizeof(u32         ), 1, fp ) != 1 ) goto End;
		if( fwrite( &tag_INFO , sizeof(char        ), 8, fp ) != 8 ) goto End;
		if( fwrite( &isft_size, sizeof(u32         ), 1, fp ) != 1 ) goto End;
		if( fwrite(  pstrLIST , sizeof(char        ), isft_size, fp ) != isft_size ) goto End;
	}

	if( fwrite( &tag_fact     , sizeof(char        ), 8, fp ) != 8 ) goto End;
	if( fwrite( &fact_size    , sizeof(u32         ), 1, fp ) != 1 ) goto End;
	if( fwrite( &tag_data     , sizeof(char        ), 4, fp ) != 4 ) goto End;
	if( fwrite( &sample_size  , sizeof(u32         ), 1, fp ) != 1 ) goto End;
	if( fwrite( p_pcm->_p_smp , sizeof(char        ), sample_size, fp ) != sample_size ) goto End;

	b_ret = _true;
End:
	if( fp ) fclose( fp );

	return b_ret;
}

b32 pxtnPCM_Create( pcmFORMAT *p_pcm, s32 ch, s32 sps, s32 bps, s32 sample_num )
{
	if( bps != 8 && bps != 16 ) return _false;
	
	pxMem_free( (void**)&p_pcm->_p_smp );
	p_pcm->_ch       = ch  ;
	p_pcm->_sps      = sps ;
	p_pcm->_bps      = bps ;
	p_pcm->_smp_head =    0;
	p_pcm->_smp_body = sample_num;
	p_pcm->_smp_tail =    0;

	// bit / sample is 8 or 16
	s32 size = p_pcm->_smp_body * p_pcm->_bps * p_pcm->_ch / 8;

	if( !(p_pcm->_p_smp = (u8 *)malloc( size )) ) return _false;
	memset( p_pcm->_p_smp, (p_pcm->_bps == 8) ? 128 : 0, size );

	return _true;
}

// convert..
b32 pxtnPCM_Convert( pcmFORMAT *p_pcm, s32 new_ch, s32 new_sps, s32 new_bps )
{
	if( !_Convert_ChannelNum     ( p_pcm, new_ch  ) ) return _false;
	if( !_Convert_BitPerSample   ( p_pcm, new_bps ) ) return _false;
	if( !_Convert_SamplePerSecond( p_pcm, new_sps ) ) return _false;

	return _true;
}

b32 pxtnPCM_CopyFrom( pcmFORMAT *p_dst, const pcmFORMAT *p_src )
{
	memcpy( p_dst, p_src, offsetof(pcmFORMAT, _p_smp) );

	s32 size = ( p_src->_smp_head + p_src->_smp_body + p_src->_smp_tail ) * p_src->_ch * p_src->_bps / 8;
	if( !pxMem_zero_alloc( (void**)&p_dst->_p_smp, size ) ) return _false;
	
	memcpy( p_dst->_p_smp, p_src->_p_smp, size );
	return _true;
}

b32 pxtnPCM_Copy( pcmFORMAT *p_src, pcmFORMAT *p_dst, s32 start, s32 end )
{
	if( p_src->_smp_head || p_src->_smp_tail || !p_src->_p_smp ) return _false;
	
	memcpy( p_dst, p_src, offsetof(pcmFORMAT, _p_smp) );
	s32 size   = ( end - start ) * p_src->_ch * p_src->_bps / 8;
	s32 offset =         start   * p_src->_ch * p_src->_bps / 8;

	if( !pxMem_zero_alloc( (void**)&p_dst->_p_smp, size ) ) return _false;
	
	memcpy( p_dst->_p_smp, &p_src->_p_smp[ offset ], size );
	p_dst->_smp_body = end - start;
	return _true;
}
