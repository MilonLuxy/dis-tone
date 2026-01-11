#include <StdAfx.h>

#include "pxStdDef.h"
#include "../Fixture/pxMem.h"

static char *_p_comment_buf = NULL;
static char *_p_name_buf    = NULL;

static b32 _CopyText( char **p_dst, const char *p_src )
{
	pxMem_free( (void**)& *p_dst );
	
	if( !p_src ) return _false;
	s32 len = strlen( p_src );

	if( !pxMem_zero_alloc( (void**)& *p_dst, len + 1 ) ) return _false;
	memcpy( *p_dst, p_src, len );
	return _true;
}

void pxtnText_Release( void ){ pxMem_free( (void**)&_p_comment_buf ); pxMem_free( (void**)&_p_name_buf ); }
void        pxtnText_Set_Comment( const char *comment ){ _CopyText( &_p_comment_buf, comment ); }
const char *pxtnText_Get_Comment( void                ){ return      _p_comment_buf;            }
void        pxtnText_Set_Name   ( const char *name    ){ _CopyText( &_p_name_buf   , name    ); }
const char *pxtnText_Get_Name   ( void                ){ return      _p_name_buf   ;            }
