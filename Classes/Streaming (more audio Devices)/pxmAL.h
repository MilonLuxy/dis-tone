BOOL pxmAL_Initialize  ( HWND hWnd, long ch_num, long sps, long bps, long smp_buf, void *pThread );
BOOL pxmAL_Release     ( void );
BOOL pxmAL_IsActive    ( void );
INT  pxmAL_GetProcessed( void );
BOOL pxmAL_Unregist    ( UINT *buf );
BOOL pxmAL_Regist      ( UINT *buf, void *p_smp, long p_size );