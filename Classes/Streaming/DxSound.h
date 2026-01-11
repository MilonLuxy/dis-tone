#define DS_BUFFER_COUNT  3

typedef struct DS_STREAMING_THREAD
{
	HANDLE *events;
	DWORD   count;
	HANDLE  handle;
	DWORD   id;
};

void *DxSound_GetDirectSoundPointer( void );
BOOL  DxSound_Initialize( HWND hWnd, long ch_num, long sps, long bps, long smp_buf );
BOOL  DxSound_Release   ( void );
BOOL  DxSound_Lock      ( DWORD buf_index, LPVOID *lpbuf1, DWORD *dwbuf1, LPVOID *lpbuf2, DWORD *dwbuf2 );
BOOL  DxSound_Unlock    (                  LPVOID  lpbuf1, DWORD  dwbuf1, LPVOID  lpbuf2, DWORD  dwbuf2 );