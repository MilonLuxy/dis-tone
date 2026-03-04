BOOL pxWASAPI_Initialize ( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread );
BOOL pxWASAPI_Release    ( void );
void pxWASAPI_GetPadding ( UINT *pSize );
void pxWASAPI_GetBufCount( UINT *p_bufCount             );
HANDLE pxWASAPI_GetHandle( void );
BOOL pxWASAPI_Fill       ( UINT buf_index, BYTE **lpBuf );
BOOL pxWASAPI_Unfill     ( UINT buf_index               );