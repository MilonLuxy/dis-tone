#include <StdAfx.h>


CRITICAL_SECTION _cs;
bool             _b = false;

void CriticalSection_Initialize( void ){           InitializeCriticalSection( &_cs ); _b = true ;   }
void CriticalSection_Release   ( void ){ if( _b ){ DeleteCriticalSection    ( &_cs ); _b = false; } }
bool CriticalSection_In        ( void ){ if( _b )  EnterCriticalSection     ( &_cs ); return _b ;   }
void CriticalSection_Out       ( void ){ if( _b )  LeaveCriticalSection     ( &_cs );               }
void CriticalSection_End       ( void ){ _b = false; }
