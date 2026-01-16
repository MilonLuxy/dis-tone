#include "pxStdDef.h"
#include "../Fixture/DataDevice.h"

// (24byte) =================
typedef struct
{
	s32 start_pos_meas  ;
	s32 start_pos_sample;

	s32 meas_end        ;
	s32 meas_repeat     ;
	f32 fadein_sec      ;

	f32 master_volume   ;
}
pxtnVOMITPREPARATION;


void pxtnService_Clear          ( void );
b32  pxtnService_Write          ( const char *path, b32 b_tune  );
s32  pxtnService_Pre_Count_Event( DDV *p_read,      s32 p_count );
b32  pxtnService_Read           ( DDV *p_read,      b32 b_tune  );
b32  pxtnService_CheckPTIFormat ( b8  *b_pti,       DDV *p_read, const char *path );

// pxtnServiceMoo -------------------------------
b32  pxtnServiceMoo_Is_Finished       ( void  );
b32  pxtnServiceMoo_Is_Prepared       ( void  );
s32  pxtnServiceMoo_Get_NowClock      ( void  );
s32  pxtnServiceMoo_Get_EndClock      ( void  );
void pxtnServiceMoo_SetMute_By_Unit   ( b32 b );
void pxtnServiceMoo_SetLoop           ( b32 b );
b32  pxtnServiceMoo_SetFade           ( s32 fade, s32 msec );
b32  pxtnServiceMoo_Preparation       ( const pxtnVOMITPREPARATION *p_prep );
void pxtnServiceMoo_Release           ( void  );
s32  pxtnServiceMoo_Get_SamplingOffset( void  );
void pxtnServiceMoo_Set_Master_Volume ( f32 v );
b32  pxtnServiceMoo_Proc              ( void *p_buf, s32 size );
