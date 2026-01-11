#include <StdAfx.h>
#include "pxStdDef.h"


static char _err_msg[ 32 ] = {0}; // receives all error messages from 'pxtnService'

void pxtnError_Set( const char *fmt, ... )
{
	char str[ 64 ];
	memset( str, 0, sizeof(str) );

	va_list ap; va_start( ap, fmt ); vsprintf( str, fmt, ap ); va_end( ap );

	memset( _err_msg,   0, sizeof(_err_msg)     );
	memcpy( _err_msg, str, sizeof(_err_msg) - 1 );
}
char *pxtnError_Get( void ){ return _err_msg; }
