void ActiveTone_Voice_Release   ( void );
BOOL ActiveTone_Voice_Initialize( long    ch_num, long    sps, long    bps, long size );
void ActiveTone_Voice_Sampling  ( void       *p1,                           long size );
void ActiveTone_Voice_Stop      ( long play_id, BOOL b_force );
long ActiveTone_Voice_Get_PlayId( void );
void ActiveTone_Voice_StopAll   ( void );
void ActiveTone_Voice_Freq      ( long play_id, float freq   );