BOOL pxMem_zero_alloc( void **pp, long byte_size );
void pxMem_free      ( void **pp );
void pxMem_zero      ( void  *p , long byte_size );
void pxMem_cap       ( char   *val_c, char   max_c, char   min_c );
void pxMem_cap       ( short  *val_s, short  max_s, short  min_s );
void pxMem_cap       ( long   *val_l, long   max_l, long   min_l );
void pxMem_cap       ( float  *val_f, float  max_f, float  min_f );
void pxMem_cap       ( double *val_d, double max_d, double min_d );