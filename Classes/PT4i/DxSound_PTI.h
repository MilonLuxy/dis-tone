#ifdef pxINCLUDE_PT4i
bool DS_CreateBuffer( WAVEFORMATEX *p_format, int p_size, unsigned char *p_data, int no );
bool DS_Initialize  ( HWND hWnd );
void DS_Release     ( void );
void DS_Voice_Freq  ( int no, float rate   );
void DS_Voice_Volume( int no, float volume );
void DS_Voice_Stop  ( int no               );
void DS_Voice_Play  ( int no, bool b_loop  );
#endif