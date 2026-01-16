#include <StdAfx.h>


BOOL pxMem_zero_alloc( void **pp, long size )
{
	*pp = malloc( size ); if( !( *pp ) ) return FALSE;
	memset( *pp, 0, size );              return TRUE ;
}

void pxMem_free( void **pp )
{
	if( *pp ){ free( *pp ); *pp = NULL; }
}

void pxMem_zero( void *p, long byte_size )
{
	if( !p ) return;
	char *p_ = (char*)p;
	for( long i = 0; i < byte_size; i++, p_++ ) *p_ = 0;
}

void pxMem_cap( long   *val, long   max, long   min ){ *val = ( *val > max ? max : *val < min ? min : *val ); }
void pxMem_cap( double *val, double max, double min ){ *val = ( *val > max ? max : *val < min ? min : *val ); }
void pxMem_cap( char   *val, char   max, char   min ){ long   l = *val; pxMem_cap( &l, (long  )max, (long  )min ); *val = (char )l; }
void pxMem_cap( short  *val, short  max, short  min ){ long   l = *val; pxMem_cap( &l, (long  )max, (long  )min ); *val = (short)l; }
void pxMem_cap( int    *val, int    max, int    min ){ long   l = *val; pxMem_cap( &l, (long  )max, (long  )min ); *val = (int  )l; }
void pxMem_cap( float  *val, float  max, float  min ){ double d = *val; pxMem_cap( &d, (double)max, (double)min ); *val = (float)d; }
