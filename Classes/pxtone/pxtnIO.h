#include "pxStdDef.h"
#include "../Fixture/DataDevice.h"
#include "pxtnOverDrive.h"
#include "pxtnUnit.h"
#include "pxtnWoice.h"

u16 pxtnIO_GetCompileVersion( void );

// Write..
b32 pxtnIO_assiUNIT_Write  ( FILE *fp, TUNEUNITTONESTRUCT *p_u    , s32 idx   );
b32 pxtnIO_num_UNIT_Write  ( FILE *fp                             , s32 num   );
b32 pxtnIO_assiWOIC_Write  ( FILE *fp, WOICE_STRUCT       *p_w    , s32 idx   );
b32 pxtnIO_matePCM_Write   ( FILE *fp, WOICE_STRUCT       *p_voice            );
b32 pxtnIO_matePTN_Write   ( FILE *fp, WOICE_STRUCT       *p_voice            );
b32 pxtnIO_matePTV_Write   ( FILE *fp, WOICE_STRUCT       *p_voice            );
b32 pxtnIO_mateOGGV_Write  ( FILE *fp, WOICE_STRUCT       *p_voice            );
b32 pxtnIO_effeOVER_Write  ( FILE *fp, OVERDRIVE_STRUCT   *p_ovdrv            );
b32 pxtnIO_effeDELA_Write  ( FILE *fp, DELAYSTRUCT        *p_dela             );
b32 pxtnIO_textCOMM_Write  ( FILE *fp                                         );
b32 pxtnIO_textNAME_Write  ( FILE *fp                                         );
b32 pxtnIO_Event_V5_Write  ( FILE *fp                             , s32 rough );
b32 pxtnIO_MasterV5_Write  ( FILE *fp                             , s32 rough );
b32 pxtnIO_Master_x4x_Write( FILE *fp                             , s32 rough );

b32 pxtnIO_PTI_Write       ( const char *path_pti,     s32 beat_divide        );

// Read..
b32 pxtnIO_assiUNIT_Read           ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_num_UNIT_Read           ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_Unit_Read               ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_UnitOld_Read            ( DDV *p_read                                               );
b32 pxtnIO_assiWOIC_Read           ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_matePCM_Read            ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_matePTN_Read            ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_matePTV_Read            ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_mateOGGV_Read           ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_effeOVER_Read           ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_effeDELA_Read           ( DDV *p_read,                                   b32 *pbNew );
b32 pxtnIO_textCOMM_Read           ( DDV *p_read                                               );
b32 pxtnIO_textNAME_Read           ( DDV *p_read                                               );
b32 pxtnIO_Event_V5_Read           ( DDV *p_read                                               );
s32 pxtnIO_v5_EventNum_Read        ( DDV *p_read                                               );
b32 pxtnIO_Event_x4x_Read          ( DDV *p_read, b32 bTailAbsolute, b32 bCheckRRR, b32 *pbNew );
s32 pxtnIO_x4x_EventNum_Read       ( DDV *p_read                                               );
b32 pxtnIO_MasterV5_Read           ( DDV *p_read,                                   b32 *pbNew );
s32 pxtnIO_Master_v5_EventNum_Read ( DDV *p_read                                               );
b32 pxtnIO_Master_x4x_Read         ( DDV *p_read,                                   b32 *pbNew );
s32 pxtnIO_Master_x4x_EventNum_Read( DDV *p_read                                               );
b32 pxtnIO_x1x_Project_Read        ( DDV *p_read                                               );