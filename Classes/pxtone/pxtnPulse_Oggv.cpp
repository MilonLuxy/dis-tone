#include <StdAfx.h>

#include "pxtnPulse_Oggv.h"

#ifdef  pxINCLUDE_OGGVORBIS

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

typedef struct
{
    char *p_buf; // ogg vorbis-data on memory.s
    s32   size ; //
	s32   pos  ; // reading position.
}
OVMEM;

void      (* ogg_free   )( oggvUNIT   *p_ogg                   );
oggvUNIT *(* ogg_load   )( const char *path                    );
b32       (* ogg_decode )( oggvUNIT   *p_ogg, pcmFORMAT *p_pcm );
s32       (* ogg_size   )( oggvUNIT   *p_ogg                   );
b32       (* ogg_write  )( oggvUNIT   *p_ogg, FILE      *fp    );
oggvUNIT *(* ogg_read   )( DDV        *p_read, oggvUNIT *p     );


////////////////////////////
// ローカル関数 ////////////
////////////////////////////

// 4 callbacks below:

static size_t _mread( void *ptr, size_t size, size_t nmemb, void *p_void )
{
	OVMEM *pom = (OVMEM*)p_void;

	if( !pom ) return -1;
	if( pom->pos >= pom->size || pom->pos == -1 ) return 0;
	
	s32 left = pom->size - pom->pos;

	if( (s32)(size * nmemb) >= left )
	{
		memcpy( ptr, &pom->p_buf[ pom->pos ], pom->size - pom->pos );
		pom->pos = pom->size;
		return left / size;
	}

	memcpy( ptr, &pom->p_buf[ pom->pos ], nmemb * size );
	pom->pos += (s32)( nmemb * size );

	return nmemb;
}

static int _mseek ( void *p_void, ogg_int64_t offset, int mode )
{
	OVMEM *pom = (OVMEM*)p_void;

	if( !pom || pom->pos < 0 )       return -1;
	if( offset < 0 ){ pom->pos = -1; return -1; }

	s32 newpos;
	switch( mode )
	{
		case SEEK_SET: newpos =             (s32)offset; break;
		case SEEK_CUR: newpos = pom->pos  + (s32)offset; break;
		case SEEK_END: newpos = pom->size + (s32)offset; break;
		default: return -1;
	}
	if( newpos < 0 ) return -1;

	pom->pos = newpos;

	return 0;
}

static long _mtell       ( void *p_void ){ OVMEM *pom = (OVMEM*)p_void; return ( pom ) ? pom->pos : -1; }
static int  _mclose_dummy( void *p_void ){ OVMEM *pom = (OVMEM*)p_void; return ( pom ) ?        0 : -1; }


static b32 ogg_SetInformation( oggvUNIT *p_ogg )
{
	b32 b_ret = _false;
	
	OVMEM ovmem;
	ovmem.p_buf = p_ogg->_p_data;
	ovmem.pos   =              0;
	ovmem.size  = p_ogg->_size  ;

	// set callback func.
	ov_callbacks   oc;
	oc.read_func  = _mread       ;
	oc.seek_func  = _mseek       ;
	oc.close_func = _mclose_dummy;
	oc.tell_func  = _mtell       ;
	
	OggVorbis_File vf;
	
	vorbis_info *vi;

	switch( ov_open_callbacks( &ovmem, &vf, NULL, 0, oc ) )
	{
	case OV_EREAD     : goto End; //{printf("A read from media returned an error.\n");exit(1);} 
	case OV_ENOTVORBIS: goto End; //{printf("Bitstream is not Vorbis data. \n");exit(1);}
	case OV_EVERSION  : goto End; //{printf("Vorbis version mismatch. \n");exit(1);}
	case OV_EBADHEADER: goto End; //{printf("Invalid Vorbis bitstream header. \n");exit(1);}
	case OV_EFAULT    : goto End; //{printf("Internal logic fault; indicates a bug or heap/stack corruption. \n");exit(1);}
	default: break;
	}
	
	vi = ov_info( &vf,-1 );
	
	p_ogg->_ch      = vi->channels;
	p_ogg->_sps2    = vi->rate    ;
	p_ogg->_smp_num = (s32)ov_pcm_total( &vf, -1 );
    
	b_ret = _true;
End:
    // end.
    ov_clear( &vf );
	
	return b_ret;
}

static b32 ogg_Write( oggvUNIT *p_ogg, const char *path )
{
	b32 b_ret = _false;

	FILE *fp = fopen( path, "wb" ); if( !fp ) goto End;
	if( fwrite( p_ogg->_p_data, 1, p_ogg->_size, fp ) != p_ogg->_size ) goto End;

	b_ret = _true;
End:
	if( fp ) fclose( fp );

	return b_ret;
}

static b32 ogg_GetInfo( oggvUNIT *p_ogg, s32 *p_ch, s32 *p_sps, s32 *p_smp_num )
{
	if( !p_ogg ) return _false;

	if( p_ch      ) *p_ch      = p_ogg->_ch;
	if( p_sps     ) *p_sps     = p_ogg->_sps2;
	if( p_smp_num ) *p_smp_num = p_ogg->_smp_num;

	return _true;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

void pxtnOggv_Initialize( void      (*pxtn_free  )(oggvUNIT   *p_ogg                  ),
						  oggvUNIT *(*pxtn_load  )(const char *path                   ),
						  b32       (*pxtn_decode)(oggvUNIT   *p_ogg, pcmFORMAT *p_pcm),
						  s32       (*pxtn_size  )(oggvUNIT   *p_ogg                  ),
						  b32       (*pxtn_write )(oggvUNIT   *p_ogg, FILE      *fp   ),
						  oggvUNIT *(*pxtn_read  )(DDV        *p_read, oggvUNIT *p )  )
{
	ogg_free   = pxtn_free  ;
	ogg_load   = pxtn_load  ;
	ogg_decode = pxtn_decode;
	ogg_size   = pxtn_size  ;
	ogg_write  = pxtn_write ;
	ogg_read   = pxtn_read  ;
}

b32 pxtnOggv_CopyFrom  ( oggvUNIT *p_dst, const oggvUNIT *p_src )
{
	if( !p_src->_p_data ) return _true;

	memcpy( p_dst, p_src, offsetof(oggvUNIT, _p_data) );

	if( !(p_dst->_p_data = (char*)malloc( p_src->_size )) ) return _false;
	memcpy( p_dst->_p_data, p_src->_p_data, p_src->_size );
	return _true;
}

void pxtn_free( oggvUNIT *p_ogg )
{
	if( p_ogg )
	{
		if( p_ogg->_p_data ){ free( p_ogg->_p_data ); p_ogg->_p_data = NULL; }
		free( p_ogg ); p_ogg = NULL;
	}
}

oggvUNIT *pxtn_load( const char *path )
{
	b32   b_ret = _false;
	FILE *fp = NULL;

	HANDLE hFile = CreateFile( path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ) return NULL;

	oggvUNIT *p_ogg = (oggvUNIT*)malloc( sizeof(oggvUNIT) );
	if( !p_ogg ) return NULL;
	memset( p_ogg, 0, sizeof(oggvUNIT) );

	p_ogg->_size = GetFileSize( hFile, NULL );
	CloseHandle( hFile );

	if( p_ogg->_size )
	{
		p_ogg->_p_data = (char*)malloc( p_ogg->_size );

		if( !p_ogg->_p_data ) goto End;
		if( !(fp = fopen( path, "rb" )) ) goto End;
		if( fread( p_ogg->_p_data, 1, p_ogg->_size, fp ) != p_ogg->_size ) goto End;
		if( !ogg_SetInformation( p_ogg ) ) goto End;
	}

	b_ret = _true;
End:
	if( fp ) fclose( fp );
	if( !b_ret && p_ogg )
	{
		if( p_ogg->_p_data ) free( p_ogg->_p_data );
		free( p_ogg ); p_ogg = NULL;
	}

	return p_ogg;
}

b32 pxtn_decode( oggvUNIT *p_ogg, pcmFORMAT *p_pcm )
{
	b32 b_ret = _false;

	OggVorbis_File vf;
	vorbis_info   *vi;
	ov_callbacks   oc;

	OVMEM ovmem;

	ovmem.p_buf = p_ogg->_p_data;
	ovmem.pos   =              0;
	ovmem.size  = p_ogg->_size  ;

	// set callback func.
	oc.read_func  = _mread       ;
	oc.seek_func  = _mseek       ;
	oc.close_func = _mclose_dummy;
	oc.tell_func  = _mtell       ;

	switch( ov_open_callbacks( &ovmem, &vf, NULL, 0, oc ) )
	{
	case OV_EREAD     : goto End; //{printf("A read from media returned an error.\n");exit(1);} 
	case OV_ENOTVORBIS: goto End; //{printf("Bitstream is not Vorbis data. \n");exit(1);}
	case OV_EVERSION  : goto End; //{printf("Vorbis version mismatch. \n");exit(1);}
	case OV_EBADHEADER: goto End; //{printf("Invalid Vorbis bitstream header. \n");exit(1);}
	case OV_EFAULT    : goto End; //{printf("Internal logic fault; indicates a bug or heap/stack corruption. \n");exit(1);}
	default: break;
	}

	vi = ov_info( &vf,-1 );

	int current_section;
	char pcmout[ 4096 ] = {0}; //take 4k out of the data segment, not the stack
	{
		s32 smp_num = (s32)ov_pcm_total( &vf, -1 );
		DWORD bytes;

		bytes = vi->channels * 2 * smp_num;

		if( !pxtnPCM_Create( p_pcm, vi->channels, vi->rate, 16, smp_num ) ) goto End;
	}
	// decode..
	{
		s32 ret = 0;
		u8 *p = (u8*)p_pcm->_p_smp;
		do
		{
			ret = ov_read( &vf, pcmout, sizeof(pcmout), 0, 2, 1, &current_section );
			if( ret > 0 ) memcpy( p, pcmout, ret ); //fwrite( pcmout, 1, ret, of );
			p += ret;
		}
		while( ret );
	}


	b_ret = _true;
End:
    // end.
    ov_clear( &vf );

	return b_ret;
}

s32 pxtn_size( oggvUNIT *p_ogg )
{
	if( !p_ogg ) return 0;
	return sizeof(p_ogg->_size)*4 + p_ogg->_size;
}

b32 pxtn_write( oggvUNIT *p_ogg, FILE *fp )
{
	if( !p_ogg || !fp ) return _false;

	if( fwrite( &p_ogg->_ch     , sizeof(s32 ),            1, fp ) !=            1 ) return _false;
	if( fwrite( &p_ogg->_sps2   , sizeof(s32 ),            1, fp ) !=            1 ) return _false;
	if( fwrite( &p_ogg->_smp_num, sizeof(s32 ),            1, fp ) !=            1 ) return _false;
	if( fwrite( &p_ogg->_size   , sizeof(s32 ),            1, fp ) !=            1 ) return _false;
	if( fwrite( &p_ogg->_p_data , sizeof(char), p_ogg->_size, fp ) != p_ogg->_size ) return _false;

	return _true;
}

oggvUNIT *pxtn_read( DDV *p_read, oggvUNIT *p )
{
	b32 b_ret = _false;

	oggvUNIT *p_ogg = NULL;
	if( !(p_ogg = (oggvUNIT*)malloc( sizeof(oggvUNIT) )) ) goto End;
	memset( p_ogg, 0, sizeof(oggvUNIT) );

	if( !ddv_Read( &p_ogg->_ch     , 4, 1, p_read ) ) goto End;
	if( !ddv_Read( &p_ogg->_sps2   , 4, 1, p_read ) ) goto End;
	if( !ddv_Read( &p_ogg->_smp_num, 4, 1, p_read ) ) goto End;
	if( !ddv_Read( &p_ogg->_size   , 4, 1, p_read ) ) goto End;
	if( !p_ogg->_size                               ) goto End;

	if( !(p_ogg->_p_data = (char*)malloc( p_ogg->_size )) ) goto End;
	if( !ddv_Read( p_ogg->_p_data, 1, p_ogg->_size, p_read ) ) goto End;

	memcpy( p, p_ogg, offsetof( oggvUNIT, _p_data ) );
	p->_p_data = p_ogg->_p_data;
	p_ogg->_p_data = NULL;

	b_ret = _true;
End:
	if( !b_ret && p_ogg )
	{
		if( p_ogg->_p_data ) free( p_ogg->_p_data );
		free( p_ogg ); p_ogg = NULL;
	}

	return p_ogg;
}

#endif
