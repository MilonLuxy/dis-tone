void *Streaming_GetDirectSound( void );
BOOL  Streaming_Release       ( void );
BOOL  Streaming_Initialize    ( DWORD *strm_cfg, long size );
BOOL  Streaming_Tune_Start    ( const void *p_prep );
void  Streaming_Tune_Stop     ( void );
void  Streaming_Tune_Fadeout  ( long msec );
BOOL  Streaming_Is            ( void );
void  Streaming_GetQuality    ( long *p_ch_num, long *p_sps, long *p_bps, long *p_smp_buf );
void  Streaming_Set_SampleInfo( long    ch_num, long    sps, long    bps );
void  Streaming_Get_SampleInfo( long *p_ch_num, long *p_sps, long *p_bps );

BOOL  DxSound_Proc            ( void *arg );
BOOL  pxMME_Proc              ( void *arg );