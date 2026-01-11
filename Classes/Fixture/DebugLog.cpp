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


const char *print_descs[] =
{
	"pxtone_Ready();\n\n* hWnd: 0x%p\n* channel_num: %d\n* sps: %d\n* bps: %d\n* buffer_sec: %.2f\n* bDirectSound: %s\n* pProc: 0x%p",
	"pxtone_Reset();\n\n* hWnd: 0x%p\n* channel_num: %d\n* sps: %d\n* bps: %d\n* buffer_sec: %.2f\n* bDirectSound: %s\n* pProc: 0x%p",
	"pxtone_GetDirectSound( void );",
	"pxtone_GetLastError( void );",
	"pxtone_GetQuality();\n\n* p_channel_num: %d\n* p_sps: %d\n* p_bps: %d\n* p_sample_per_buf: %d",
	"pxtone_Release( void );",
	"pxtone_Tune_Load();\n\n* hModule: 0x%p\n* type_name: %s\n* file_name: %s",
	"pxtone_Tune_Read();\n\n* p: 0x%p\n* size: %d",
	"pxtone_Tune_Release( void );",
	"pxtone_Tune_Start();\n\n* start_sample: %d\n* fadein_msec: %d\n* volume: %.2f",
	"pxtone_Tune_Fadeout();\n\n* msec: %d",
	"pxtone_Tune_SetVolume();\n\n* v: %.2f",
	"pxtone_Tune_Stop( void );",
	"pxtone_Tune_IsStreaming( void );",
	"pxtone_Tune_SetLoop();\n\n* bLoop: %s",
	"pxtone_Tune_GetInformation();\n\n* p_beat_num: %d\n* p_beat_tempo: %.2f\n* p_beat_clock: %d\n* p_meas_num: %d",
	"pxtone_Tune_GetRepeatMeas( void );",
	"pxtone_Tune_GetPlayMeas( void );",
	"pxtone_Tune_GetName( void );",
	"pxtone_Tune_GetComment( void );",
	"pxtone_Tune_Vomit();\n\n* p: 0x%p\n* sample_num: %d",
	"pxtone_Noise_Initialize( void );",
	"pxtone_Noise_Release();\n\n* p_noise->p_buf: 0x%p\n* p_noise->size: %d",
	"pxtone_Noise_Create();\n\n* name: %s\n* type: %s\n* channel_num: %d\n* sps: %d\n* bps: %d",
	"pxtone_Noise_Create_FromMemory();\n\n* p_designdata: %s\n* designdata_size: %d\n* channel_num: %d\n* sps: %d\n* bps: %d",
};

void print_func( const char *fmt, ... )
{
	char str[ 0x80 + MAX_PATH ];
	va_list ap; va_start( ap, fmt ); vsprintf( str, fmt, ap ); va_end( ap );
	MessageBox( NULL, str, "pxtoneDLL", MB_OK );
}
const char *pTitle( int  num ){ return ( num >= 0 ) ? print_descs[ num ] : NULL; }
const char *pBool ( bool b   ){ return b ? "true" : "false"; }
