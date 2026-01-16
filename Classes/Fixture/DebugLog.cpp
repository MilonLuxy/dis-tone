#include <StdAfx.h>
#include <stdio.h>


static CRITICAL_SECTION _cs;

static char            *_path_dir  = NULL;
static int              _max_size  =    0;
static const char      *_file_name = "debug.txt";
static const char      *_old_ext   = "_old";

void DebugLog_Initialize( int max_size, const char *path_dir ) // default = 1024 * 100
{
	char str[ MAX_PATH ];
	memset( str, 0, MAX_PATH );

	va_list ap; va_start( ap, path_dir ); vsprintf( str, path_dir, ap ); va_end( ap );

	int size = strlen( str ); size++;
	_path_dir = (char*)malloc( size );

	if( _path_dir )
	{
		memcpy( _path_dir, str, size );
		_max_size = max_size;
		FILE *fp = fopen( _path_dir, "a+t" );
		if( fp )
		{
			SYSTEMTIME st;
			GetLocalTime( &st );
			fprintf( fp, "%04d%02d%02d\n", st.wYear, st.wMonth, st.wDay );
			fclose( fp );
		}
		InitializeCriticalSection( &_cs );
	}
}
void DebugLog_Release( void )
{
	if( _path_dir ){ free( _path_dir ); _path_dir = NULL; }
	DeleteCriticalSection( &_cs );
}

void dlog( const char *fmt, ... )
{
	if( !_path_dir ) return;
	
	EnterCriticalSection( &_cs );
	{
		SYSTEMTIME st;
		char       str[1024];

		memset( str, 0, sizeof(str) );

		HANDLE hFile = CreateFile( _path_dir, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE ) return;

		int size = GetFileSize( hFile, NULL );
		CloseHandle( hFile );

		if( size > _max_size )
		{
			char name[ 64 ];
			const char *ext = PathFindExtension( _path_dir );
			strcpy( name, _path_dir );
			PathRemoveExtension( name );
			strcat( name, _old_ext  ); // turns 'debug' into 'debug_old'
			strcat( name, ext       );
			SetFileAttributes( name, FILE_ATTRIBUTE_NORMAL );
			DeleteFile( name );
			MoveFile( _path_dir, name );
		}

		va_list ap; va_start( ap, fmt ); vsprintf( str, fmt, ap ); va_end( ap );
		FILE *fp = fopen( _path_dir, "a+t" );

		if( fp )
		{
			GetLocalTime( &st );
			fprintf( fp, ",%02d%02d%02d,%s\n", st.wHour, st.wMinute, st.wSecond, str );
			fclose( fp );
		}
	}
	LeaveCriticalSection( &_cs );
}
