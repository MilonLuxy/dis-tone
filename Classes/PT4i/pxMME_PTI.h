#ifdef pxINCLUDE_PT4i
bool MME_LoadBuffer  ( WAVEFORMATEX *p_format, int p_size, unsigned char *p_data, bool b_reset_freq );
bool MME_Initialize  ( HWND hWnd, int ch_num, int sps, int bps );
void MME_Release     ( void );
void MME_Voice_Freq  ( int no, float rate   );
void MME_Voice_Volume( int no, float volume );
void MME_Voice_Stop  ( int no               );
void MME_Voice_Play  ( int no, bool b_loop  );
#endif