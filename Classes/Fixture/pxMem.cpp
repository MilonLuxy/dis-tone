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

void pxMem_cap( long   *val_l, long   max_l, long   min_l )
{
	if( *val_l > max_l ) *val_l = max_l;
	if( *val_l < min_l ) *val_l = min_l;
}
void pxMem_cap( char   *val_c, char   max_c, char   min_c )
{
	if( *val_c > max_c ) *val_c = max_c;
	if( *val_c < min_c ) *val_c = min_c;
}
void pxMem_cap( short  *val_s, short  max_s, short  min_s )
{
	if( *val_s > max_s ) *val_s = max_s;
	if( *val_s < min_s ) *val_s = min_s;
}
void pxMem_cap( float  *val_f, float  max_f, float  min_f )
{
	if( *val_f > max_f ) *val_f = max_f;
	if( *val_f < min_f ) *val_f = min_f;
}
void pxMem_cap( double *val_d, double max_d, double min_d )
{
	if( *val_d > max_d ) *val_d = max_d;
	if( *val_d < min_d ) *val_d = min_d;
}
