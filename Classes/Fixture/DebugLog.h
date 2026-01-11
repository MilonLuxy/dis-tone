void DebugLog_Initialize( int max_size, const char *path_dir );
void DebugLog_Release   ( void );
void dlog               ( const char *fmt, ... );

void print_func         ( const char *fmt, ... );
const char *pTitle      ( int  num );
const char *pBool       ( bool b   );