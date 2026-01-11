#include "pxStdDef.h"

#define EVENTDEFAULT_VOLUME       104
#define EVENTDEFAULT_VELOCITY     104
#define EVENTDEFAULT_PAN_VOLUME    64
#define EVENTDEFAULT_PAN_TIME       0
#define EVENTDEFAULT_PORTAMENT      0
#define EVENTDEFAULT_VOICENO        0
#define EVENTDEFAULT_GROUPNO        0
#define EVENTDEFAULT_KEY       0x6000
#define EVENTDEFAULT_BASICKEY  0x4500 // 4A(440Hz?)
#define EVENTDEFAULT_TUNING      1.0f

#define EVENTDEFAULT_BEATNUM        4
#define EVENTDEFAULT_BEATTEMPO    120
#define EVENTDEFAULT_BEATCLOCK    480

static enum EVENTKIND
{
	EVENTKIND_NULL   = 0, //  0
	EVENTKIND_ON        , //  1
	EVENTKIND_KEY       , //  2
	EVENTKIND_PAN_VOLUME, //  3
	EVENTKIND_VELOCITY  , //  4
	EVENTKIND_VOLUME    , //  5
	EVENTKIND_PORTAMENT , //  6
	EVENTKIND_BEATCLOCK , //  7
	EVENTKIND_BEATTEMPO , //  8
	EVENTKIND_BEATNUM   , //  9
	EVENTKIND_REPEAT    , // 10
	EVENTKIND_LAST      , // 11
	EVENTKIND_VOICENO   , // 12
	EVENTKIND_GROUPNO   , // 13
	EVENTKIND_TUNING    , // 14
	EVENTKIND_PAN_TIME  , // 15

	EVENTKIND_NUM, // 16
};

// (20byte) =================
typedef struct EVERECORD
{
	u8 kind    ;
	u8 unit_no ;
	u8 reserve1;
	u8 reserve2;
	s32          value   ;
	s32          clock   ;
	EVERECORD    *prev    ;
	EVERECORD    *next    ;
};

b32  pxtnEvelist_Initialize           ( s32  event_num );
void pxtnEvelist_Release              ( void );
void pxtnEvelist_Reset                ( void );
void pxtnEvelist_Linear_Start         ( void );
void pxtnEvelist_x4x_Read_Start       ( void );
void pxtnEvelist_Clear_AtExit         ( void );
s32  pxtnEvelist_Get_EventNum         ( void );
s32  pxtnEvelist_Get_ClockNum         ( void );
void pxtnEvelist_x4x_Read_NewKind     ( void );
void pxtnEvelist_Linear_End           ( b32  b_connect );
void pxtnEvelist_Record_Add           ( s32  clock      ,                u8 unit_no, u8 kind, s32 value   );
void pxtnEvelist_Record_Add_f         ( s32  clock      ,                u8 unit_no, u8 kind, f32 value_f );
void pxtnEvelist_Linear_Add           ( s32  clock      ,                u8 unit_no, u8 kind, s32 value   );
void pxtnEvelist_Linear_Add_f         ( s32  clock      ,                u8 unit_no, u8 kind, f32 value_f );
void pxtnEvelist_x4x_Read_Add         ( s32  clock      ,                u8 unit_no, u8 kind, s32 value   );
b32  pxtnEvelist_Kind_IsTail          (                                              u8 kind              );
const EVERECORD *pxtnEvelist_Get_Records( void );

// counters ----------------------
s32  pxtnEvelist_Count_TotalEvent     ( void                                                              );
s32  pxtnEvelist_Count_TotalUnit      (                                  u8 unit_no                       );
s32  pxtnEvelist_Count_Kind_Value     (                                              u8 kind, s32 value   );
s32  pxtnEvelist_Count_Unit_Kind      (                                  u8 unit_no, u8 kind              );
s32  pxtnEvelist_Count_ActiveNotes    ( s32  clock_start, s32 clock_end, u8 unit_no                       );
s32  pxtnEvelist_Get_LastValue        ( s32  clock      ,                u8 unit_no, u8 kind              );
s32  pxtnEvelist_Get_MaxClock         ( void                                                              );
s32  pxtnEvelist_Record_Delete        ( s32  clock_start, s32 clock_end, u8 unit_no, u8 kind              );
s32  pxtnEvelist_Record_Delete        ( s32  clock_start, s32 clock_end, u8 unit_no                       );
s32  pxtnEvelist_Record_Clock_Shift   ( s32  clock      , s32 shift    , u8 unit_no                       ); // can't be under 0.
s32  pxtnEvelist_Record_Value_Change  ( s32  clock_start, s32 clock_end, u8 unit_no, u8 kind, s32 value   );
s32  pxtnEvelist_Record_Value_Set     ( s32  clock_start, s32 clock_end, u8 unit_no, u8 kind, s32 value   );
s32  pxtnEvelist_Record_Value_Omit    (                                                       s32 value   );
s32  pxtnEvelist_Record_Value_Replace ( s32  old_value  , s32 new_value                                   );
s32  pxtnEvelist_Record_UnitNo_Miss   (                                  u8 unit_no                       ); // delete event has the unit-no
s32  pxtnEvelist_Record_UnitNo_Replace(                                  u8 old_u  , u8 new_u             ); // exchange unit