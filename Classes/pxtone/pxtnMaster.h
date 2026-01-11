#include "pxStdDef.h"
void pxtnMaster_Reset         ();
void pxtnMaster_Set           ( s32    beat_num, f32    beat_tempo, s32    beat_clock );
void pxtnMaster_Get           ( s32 *p_beat_num, f32 *p_beat_tempo, s32 *p_beat_clock );
s32  pxtnMaster_Get_BeatNum   ();
f32  pxtnMaster_Get_BeatTempo ();
s32  pxtnMaster_Get_BeatClock ();
s32  pxtnMaster_Get_MeasNum   ();
s32  pxtnMaster_Get_RepeatMeas();
s32  pxtnMaster_Get_LastMeas  ();
s32  pxtnMaster_Get_LastClock ();
s32  pxtnMaster_Get_PlayMeas  ();
s32  pxtnMaster_Get_ThisClock ( s32 meas, s32 beat, s32 clock );
void pxtnMaster_Adjust_MeasNum( s32 clock      );
void pxtnMaster_Set_MeasNum   ( s32 meas_num   );
void pxtnMaster_Set_RepeatMeas( s32 meas       );
void pxtnMaster_Set_LastMeas  ( s32 meas       );