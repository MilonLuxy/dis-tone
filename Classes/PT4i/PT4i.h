#ifndef PT4i_H
#define PT4i_H

#ifdef pxINCLUDE_PT4i

typedef struct
{
	unsigned char  ch  ;
	unsigned char  type;
	unsigned short val ;
} PT4iEVENT;

typedef struct
{
	unsigned char vo_id        ;
	unsigned char flags        ;
	unsigned char clock_count  ;
	float         target_volume; // new parameter
} PT4iCHANNEL;

bool PT4i_Initialize      ( HWND hWnd, int ch_num, int sps, int bps, int smp_buf, bool b_dxsound );
void PT4i_Release         ( void );
bool PT4i_Load            ( FILE *fp );
bool PT4i_Start           ( void );
void PT4i_Stop            ( void );
bool PT4i_Procedure       ( void );
void PT4i_SetVolume       ( float vol );
void PT4i_SetFade         ( int msec );
int  PT4i_Get_NowEve      ( void );
bool PT4i_IsFinised       ( void );
int  PT4i_Get_RepeatMeas  ( void );
int  PT4i_Get_PlayMeas    ( void );
void PT4i_Get_Information ( int *p_beat_divide, float *p_beat_tempo, int *p_wait, int *p_meas_num );
const char *PT4i_Get_Error( void );

#endif
#endif
