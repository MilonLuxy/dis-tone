#ifndef pxtnPulse_Oggv_H
#define pxtnPulse_Oggv_H

#include "pxStdDef.h"

#ifdef  pxINCLUDE_OGGVORBIS

#include "../Fixture/DataDevice.h"
#include "pxtnPulse_PCM.h"

typedef struct oggvUNIT
{
	s32   _ch     ;
	s32   _sps2   ;
	s32   _smp_num;
	s32   _size   ;
	char *_p_data ;
};

void pxtnOggv_Initialize( void      (*pxtn_free  )(oggvUNIT   *p_ogg                  ),
						  oggvUNIT *(*pxtn_load  )(const char *path                   ),
						  b32       (*pxtn_decode)(oggvUNIT   *p_ogg, pcmFORMAT *p_pcm),
						  s32       (*pxtn_size  )(oggvUNIT   *p_ogg                  ),
						  b32       (*pxtn_write )(oggvUNIT   *p_ogg, FILE      *fp   ),
						  oggvUNIT *(*pxtn_read  )(DDV        *p_read, oggvUNIT *p )  );
b32 pxtnOggv_CopyFrom   ( oggvUNIT *p_dst, const oggvUNIT *p_src );

void      pxtn_free  ( oggvUNIT   *p_ogg                   );
oggvUNIT *pxtn_load  ( const char *path                    );
b32       pxtn_decode( oggvUNIT   *p_ogg, pcmFORMAT *p_pcm );
s32       pxtn_size  ( oggvUNIT   *p_ogg                   );
b32       pxtn_write ( oggvUNIT   *p_ogg, FILE      *fp    );
oggvUNIT *pxtn_read  ( DDV        *p_read, oggvUNIT *p     );

#endif
#endif
