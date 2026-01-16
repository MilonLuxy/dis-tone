BOOL pxMem_zero_alloc( void **pp, long byte_size );
void pxMem_free      ( void **pp );
void pxMem_zero      ( void  *p , long byte_size );
void pxMem_cap       ( long   *val, long   max, long   min );
void pxMem_cap       ( double *val, double max, double min );
void pxMem_cap       ( char   *val, char   max, char   min );
void pxMem_cap       ( short  *val, short  max, short  min );
void pxMem_cap       ( int    *val, int    max, int    min );
void pxMem_cap       ( float  *val, float  max, float  min );