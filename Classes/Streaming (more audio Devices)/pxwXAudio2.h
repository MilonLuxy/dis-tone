#define XA_BUFFER_COUNT      3

BOOL  pxwXAudio2_Initialize  ( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread );
BOOL  pxwXAudio2_Release     ( void );
DWORD pxwXAudio2_GetState    ( void );
BOOL  pxwXAudio2_IsActive    ( void );
BOOL  pxwXAudio2_8bitAllocate( BYTE **p_buf8 );
void  pxwXAudio2_GetBufs     ( BYTE **p_data, DWORD *p_size, DWORD idx );
BOOL  pxwXAudio2_SubmitAudio ( BYTE  *p_data, DWORD  p_size );