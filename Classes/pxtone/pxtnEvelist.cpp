#include <StdAfx.h>


#include "../Fixture/pxMem.h"
#include "pxtnEvelist.h"


// (20byte) =================
typedef struct EVELIST_STRUCT
{
	s32       _event_num;
	EVERECORD *_eves     ;
	EVERECORD *_start    ;
	s32       _linear   ;
	EVERECORD *_p_x4x_rec;
};
static EVELIST_STRUCT _evelist_main;



////////////////////////////
// ローカル関数 ////////////
////////////////////////////

static void _Clear_All             ( EVELIST_STRUCT *evelist )
{
	pxMem_free( (void**)&evelist->_eves );
	evelist->_start     = NULL;
	evelist->_event_num =    0;
	evelist->_linear    =    0;
	evelist->_p_x4x_rec =    0;
}
static void _Clear                 ( EVELIST_STRUCT *evelist )
{
	if( evelist->_eves ) memset( evelist->_eves, 0, sizeof(EVERECORD) * evelist->_event_num );
	evelist->_start = NULL;
}
static void _Clear_w_Linear        ( EVELIST_STRUCT *evelist )
{
	if( evelist->_eves ){ _Clear( evelist ); evelist->_linear = 0; }
}
static void _Clear_w_Linear_x4xRec ( EVELIST_STRUCT *evelist )
{
	if( evelist->_eves ){ _Clear( evelist ); evelist->_linear = 0; evelist->_p_x4x_rec = NULL; }
}
static EVELIST_STRUCT *_ClearHelper( EVELIST_STRUCT *evelist )
{
	evelist->_eves = NULL; evelist->_start = NULL; evelist->_event_num = 0;
	return evelist;
}
static void _To_Clear_All ( EVELIST_STRUCT *evelist ){    _Clear_All(   evelist      ); }
static void _Alt_Release  ( void )                   { _To_Clear_All( &_evelist_main ); }

static s32  _Get_EventNum ( EVELIST_STRUCT *evelist ){ return ( evelist->_eves  ) ? evelist->_event_num    : 0; }
static s32  _Get_StartNum ( EVELIST_STRUCT *evelist ){ return ( evelist->_start ) ? evelist->_start->clock : 0; }

static void _Rec_Set( EVERECORD *p_rec, EVERECORD *prev, EVERECORD *next, s32 clock, u8 unit_no, u8 kind, s32 value )
{
	if( !prev ) _evelist_main._start = p_rec;
	else        prev->next           = p_rec;

	if(  next ) next->prev           = p_rec;

	p_rec->next    = next   ;
	p_rec->prev    = prev   ;
	p_rec->clock   = clock  ;
	p_rec->unit_no = unit_no;
	p_rec->kind    = kind   ;
	p_rec->value   = value  ;
}

static s32 _ComparePriority( u8 kind1, u8 kind2 )
{
	static const s32 priority_table[ EVENTKIND_NUM ] =
	{
		  0, // EVENTKIND_NULL
		 50, // EVENTKIND_ON       
		 40, // EVENTKIND_KEY      
		 60, // EVENTKIND_PAN_VOLUME  
		 70, // EVENTKIND_VELOCITY 
		 80, // EVENTKIND_VOLUME   
		 30, // EVENTKIND_PORTAMENT
		  0, // EVENTKIND_BEATCLOCK
		  0, // EVENTKIND_BEATTEMPO
		  0, // EVENTKIND_BEATNUM  
		  0, // EVENTKIND_REPEAT   
		255, // EVENTKIND_LAST     
		 10, // EVENTKIND_VOICENO  
		 20, // EVENTKIND_GROUPNO  
		 90, // EVENTKIND_TUNING 
		100, // EVENTKIND_PAN_TIME  
	};

	return priority_table[ kind1 ] - priority_table[ kind2 ];
}

static void _Rec_Cut( EVELIST_STRUCT *evelist, EVERECORD *p_rec )
{
	if( !p_rec->prev ) evelist->_start   = p_rec->next;
	else               p_rec->prev->next = p_rec->next;

	if(  p_rec->next ) p_rec->next->prev = p_rec->prev;

	p_rec->kind = EVENTKIND_NULL;
}

static b32 _Record_Add_i( EVELIST_STRUCT *evelist, s32 clock, u8 unit_no, u8 kind, s32 value )
{
	if( !evelist->_eves ) return _false;

	EVERECORD *p_new  = NULL;
	EVERECORD *p_prev = NULL;
	EVERECORD *p_next = NULL;

	// 空き検索
	for( s32 r = 0; r < evelist->_event_num; r++ )
	{
		if( evelist->_eves[ r ].kind == EVENTKIND_NULL )
		{
			p_new = &evelist->_eves[ r ];
			r = clock;
			break;
		}
	}
	if( !p_new ) return _false;

	// first.
	if( evelist->_start )
	{
		// top.
		if( clock < evelist->_start->clock )
		{
			p_next = evelist->_start;
		}
		else
		{
			for( EVERECORD *p = evelist->_start; p; p = p->next )
			{
				if( p->clock == clock ) // 同時
				{
					for( ; 1; p = p->next )
					{
						if( p->clock != clock                        ){ p_prev = p->prev; p_next = p;                           break; }
						if( unit_no == p->unit_no && kind == p->kind ){ p_prev = p->prev; p_next = p; p->kind = EVENTKIND_NULL; break; } // 置き換え
						if( _ComparePriority( kind, p->kind ) < 0    ){ p_prev = p->prev; p_next = p;                           break; } // プライオリティを検査
						if( !p->next                                 ){ p_prev = p;                                             break; } // 末端
					}
					break;
				}
				else if(  p->clock > clock ){ p_prev = p->prev; p_next = p; break; } // 追い越した
				else if( !p->next          ){ p_prev = p;                   break; } // 末端
			}
		}
	}

	_Rec_Set( p_new, p_prev, p_next, clock, unit_no, kind, value );

	// cut prev tail
	if( pxtnEvelist_Kind_IsTail( kind ) )
	{
		for( EVERECORD *p = p_new->prev; p; p = p->prev )
		{
			if( p->unit_no == unit_no && p->kind == kind )
			{
				if( clock < p->clock + p->value ) p->value = clock - p->clock;
				break;
			}
		}
	}

	// delete next
	if( pxtnEvelist_Kind_IsTail( kind ) )
	{
		for( EVERECORD *p = p_new->next; p && p->clock < clock + value; p = p->next )
		{
			if( p->unit_no == unit_no && p->kind == kind ) _Rec_Cut( &_evelist_main, p );
		}
	}

	return _true;
}

static void _x4x_Read_Add( EVELIST_STRUCT *evelist, s32 clock, u8 unit_no, u8 kind, s32 value )
{
	EVERECORD *p_new  = NULL;
	EVERECORD *p_prev = NULL;
	EVERECORD *p_next = NULL;

	p_new = &evelist->_eves[ evelist->_linear ];
	evelist->_linear++;

	// first.
	if( evelist->_start )
	{
		// top
		if( clock < evelist->_start->clock )
		{
			p_next = evelist->_start;
		}
		else
		{
			EVERECORD *p;
			
			if( !evelist->_p_x4x_rec ) p = evelist->_start    ;
			else                       p = evelist->_p_x4x_rec;

			for( ; p; p = p->next )
			{
				if( p->clock == clock ) // 同時
				{
					for( ; 1; p = p->next )
					{
						if( p->clock != clock                        ){ p_prev = p->prev; p_next = p;                                 break; }
						if( unit_no == p->unit_no && kind == p->kind ){ p_prev = p->prev; p_next = p->next; p->kind = EVENTKIND_NULL; break; } // 置き換え
						if( _ComparePriority( kind, p->kind ) < 0    ){ p_prev = p->prev; p_next = p;                                 break; } // プライオリティを検査
						if( !p->next                                 ){ p_prev = p;                                                   break; } // 末端
					}
					break;
				}
				else if( p->clock > clock ){ p_prev = p->prev; p_next = p; break; } // 追い越した
				else if( !p->next         ){ p_prev = p;                   break; } // 末端
			}
		}
	}
	_Rec_Set( p_new, p_prev, p_next, clock, unit_no, kind, value );

	evelist->_p_x4x_rec = p_new;
}

static void _Linear_Add_i( EVELIST_STRUCT *evelist, s32 clock, u8 unit_no, u8 kind, s32 value )
{
	EVERECORD *p = &evelist->_eves[ evelist->_linear ];

	p->clock   = clock;
	p->unit_no = unit_no;
	p->kind    = kind;
	p->value   = value;

	evelist->_linear++;
}

static void _Linear_End( EVELIST_STRUCT *evelist, b32 b_connect )
{
	if( evelist->_eves[ 0 ].kind != EVENTKIND_NULL ) evelist->_start = &evelist->_eves[ 0 ];

	if( b_connect )
	{
		for( s32 r = 1; r < evelist->_event_num && evelist->_eves[ 0 ].kind != EVENTKIND_NULL; r++ )
		{
			evelist->_eves[ r     ].prev = &evelist->_eves[ r - 1 ];
			evelist->_eves[ r - 1 ].next = &evelist->_eves[ r     ];
		}
	}
}

static void _x4x_Read_NewKind( EVELIST_STRUCT *evelist ){ evelist->_p_x4x_rec = NULL; }

// counters ----------------------
static s32 _Get_Count( EVELIST_STRUCT *evelist )
{
	if( !evelist->_eves || !evelist->_start ) return 0;

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next ) count++;
	return count;
}
static s32 _Get_Count( EVELIST_STRUCT *evelist, u8 unit_no )
{
	if( !evelist->_eves ) return 0;

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next ){ if( p->unit_no == unit_no ) count++; }
	return count;
}
static s32 _Get_Count( EVELIST_STRUCT *evelist, u8 kind, s32 value )
{
	if( !evelist->_eves ) return 0;

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next ){ if( p->kind == kind && p->value == value ) count++; }
	return count;
}
static s32 _Get_Count( EVELIST_STRUCT *evelist, u8 unit_no, u8 kind )
{
	if( !evelist->_eves ) return 0;

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next ){ if( p->unit_no == unit_no && p->kind == kind ) count++; }
	return count;
}
static s32 _Get_Count( EVELIST_STRUCT *evelist, s32 clock_start, s32 clock_end, u8 unit_no )
{
	if( !evelist->_eves ) return 0;

	EVERECORD *p;
	for( p = evelist->_start; p; p = p->next )
	{
		if( p->unit_no == unit_no )
		{
			if(                                       p->clock            >= clock_start ) break;
			if( pxtnEvelist_Kind_IsTail( p->kind ) && p->clock + p->value >  clock_start ) break;
		}
	}

	s32 count = 0;
	for( ; p && p->clock < clock_end; p = p->next ){ if( p->unit_no == unit_no ) count++; }
	return count;
}


static s32 _DefaultKindValue( u8 kind )
{
	switch( kind )
	{
		case EVENTKIND_KEY       : return EVENTDEFAULT_KEY       ;
		case EVENTKIND_PAN_VOLUME: return EVENTDEFAULT_PAN_VOLUME;
		case EVENTKIND_VELOCITY  : return EVENTDEFAULT_VELOCITY  ;
		case EVENTKIND_VOLUME    : return EVENTDEFAULT_VOLUME    ;
		case EVENTKIND_PORTAMENT : return EVENTDEFAULT_PORTAMENT ;
		case EVENTKIND_BEATCLOCK : return EVENTDEFAULT_BEATCLOCK ;
		case EVENTKIND_BEATTEMPO : return EVENTDEFAULT_BEATTEMPO ;
		case EVENTKIND_BEATNUM   : return EVENTDEFAULT_BEATNUM   ;
		case EVENTKIND_VOICENO   : return EVENTDEFAULT_VOICENO   ;
		case EVENTKIND_GROUPNO   : return EVENTDEFAULT_GROUPNO   ;
		case EVENTKIND_TUNING    : { f32 tuning = EVENTDEFAULT_TUNING; return *( (s32*)&tuning ); }
		case EVENTKIND_PAN_TIME  : return EVENTDEFAULT_PAN_TIME  ;
	}
	return 0;
}
static s32 _Get_Value( EVELIST_STRUCT *evelist, s32 clock, u8 unit_no, u8 kind )
{
	if( !evelist->_eves ) return 0;

	EVERECORD *p;
	s32 value = _DefaultKindValue( kind );

	for( p = evelist->_start; p && p->clock <= clock; p = p->next )
	{
		if( p->unit_no == unit_no && p->kind == kind ) value = p->value;
	}

	return value;
}

static s32 _Get_Max_Clock( EVELIST_STRUCT *evelist )
{
	s32 max_clock = 0;
	s32 clock;

	for( EVERECORD *p = evelist->_start; p; p = p->next )
	{
		if( !pxtnEvelist_Kind_IsTail( p->kind ) ) clock = p->clock;
		else                                      clock = p->clock + p->value;

		if( max_clock < clock ) max_clock = clock;
	}

	return max_clock;
}


static s32 _Record_Delete( EVELIST_STRUCT *evelist, s32 clock_start, s32 clock_end, u8 unit_no, u8 kind )
{
	if( !evelist->_eves ) return 0;
	
	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p && (p->clock == clock_start || p->clock < clock_end); p = p->next )
	{
		if( p->clock >= clock_start && p->unit_no == unit_no && p->kind == kind ){ _Rec_Cut( &_evelist_main, p ); count++; }
	}

	if( pxtnEvelist_Kind_IsTail( kind ) )
	{
		for( EVERECORD *p = evelist->_start; p && p->clock < clock_start; p = p->next )
		{
			if( p->unit_no == unit_no && p->kind == kind && clock_start < p->clock + p->value )
			{
				p->value = clock_start - p->clock;
				count++;
			}
		}
	}

	return count;
}
static s32 _Record_Delete( EVELIST_STRUCT *evelist, s32 clock_start, s32 clock_end, u8 unit_no )
{
	if( !evelist->_eves ) return 0;
	
	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p && (p->clock == clock_start || p->clock < clock_end); p = p->next )
	{
		if( p->clock >= clock_start && p->unit_no == unit_no ){ _Rec_Cut( evelist, p ); count++; }
	}

	for( EVERECORD *p = evelist->_start; p && p->clock < clock_start; p = p->next )
	{
		if( p->unit_no == unit_no && pxtnEvelist_Kind_IsTail( p->kind ) && p->clock + p->value > clock_start )
		{
			p->value = clock_start - p->clock;
			count++;
		}
	}

	return count;
}

static s32 _Record_Clock_Shift( EVELIST_STRUCT *evelist, s32 clock, s32 shift, u8 unit_no )
{
	if( !evelist->_eves || !evelist->_start || shift == 0 ) return 0;

	s32        count = 0;
	s32        c,   v;
	u8         k     ;
	EVERECORD *p_next;
	EVERECORD *p_prev;
	EVERECORD *p = evelist->_start;


	if( shift < 0 )
	{
		for( ; p && p->clock < clock; p = p->next ){}

		while( p )
		{
			if( p->unit_no == unit_no )
			{
				c      = p->clock;
				k      = p->kind ;
				v      = p->value;
				p_next = p->next ;

				_Rec_Cut( evelist, p );
				if( c + shift >= 0 ) _Record_Add_i( evelist, c + shift, unit_no, k, v );
				count++;

				p = p_next;
			}
			else p = p->next;
		}
	}
	else if( shift > 0 )
	{
		for( ; p; p = p->next ){}

		while( p && p->clock >= clock )
		{
			if( p->unit_no == unit_no )
			{
				c      = p->clock;
				k      = p->kind ;
				v      = p->value;
				p_prev = p->prev ;

				_Rec_Cut( evelist, p );
				_Record_Add_i( evelist, c + shift, unit_no, k, v );
				count++;

				p = p_prev;
			}
			else p = p->prev;
		}
	}

	return count;
}

static s32 _Record_Value_Change( EVELIST_STRUCT *evelist, s32 clock_start, s32 clock_end, u8 unit_no, u8 kind, s32 value )
{
	if( !evelist->_eves ) return 0;

	s32 max, min;
	switch( kind )
	{
		case EVENTKIND_NULL      : max =      0; min =   0; break;
		case EVENTKIND_ON        : max =    120; min = 120; break;
		case EVENTKIND_KEY       : max = 0xbfff; min =   0; break;
		case EVENTKIND_PAN_VOLUME: max =   0x80; min =   0; break;
		case EVENTKIND_PAN_TIME  : max =   0x80; min =   0; break;
		case EVENTKIND_VELOCITY  : max =   0x80; min =   0; break;
		case EVENTKIND_VOLUME    : max =   0x80; min =   0; break;
		default: max = 0; min = 0; break;
	}

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next )
	{
		if( p->unit_no == unit_no && p->kind == kind && p->clock >= clock_start && (clock_end == -1 || p->clock < clock_end) )
		{
			p->value += value;
			if( p->value < min ) p->value = min;
			if( p->value > max ) p->value = max;
			count++;
		}
	}

	return count;
}
static s32 _Record_Value_Set( EVELIST_STRUCT *evelist, s32 clock_start, s32 clock_end, u8 unit_no, u8 kind, s32 value )
{
	if( !evelist->_eves ) return 0;

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next )
	{
		if( p->unit_no == unit_no && p->kind == kind && p->clock >= clock_start && p->clock < clock_end )
		{
			p->value = value;
			count++;
		}
	}

	return count;
}
static s32 _Record_Value_Omit( EVELIST_STRUCT *evelist, u8 kind, s32 value )
{
	if( !evelist->_eves ) return 0;

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next )
	{
		if( p->kind == kind )
		{
			if     ( p->value == value ){ _Rec_Cut( evelist, p ); count++; }
			else if( p->value  > value ){ p->value--;             count++; }
		}
	}
	return count;
}
static s32 _Record_Value_Replace( EVELIST_STRUCT *evelist, u8 kind, s32 old_value, s32 new_value )
{
	if( !evelist->_eves || old_value == new_value ) return 0;

	s32 count = 0;
	if( old_value < new_value )
	{
		for( EVERECORD *p = evelist->_start; p; p = p->next )
		{
			if( p->kind == kind )
			{
				if(      p->value == old_value                          ){ p->value = new_value; count++; }
				else if( p->value >  old_value && p->value <= new_value ){ p->value--;           count++; }
			}
		}
	}
	else
	{
		for( EVERECORD *p = evelist->_start; p; p = p->next )
		{
			if( p->kind == kind )
			{
				if(      p->value == old_value                          ){ p->value = new_value; count++; }
				else if( p->value <  old_value && p->value >= new_value ){ p->value++;           count++; }
			}
		}
	}

	return count;
}

static s32 _Record_UnitNo_Miss( EVELIST_STRUCT *evelist, u8 unit_no )
{
	if( !evelist->_eves ) return 0;

	s32 count = 0;
	for( EVERECORD *p = evelist->_start; p; p = p->next )
	{
		if(      p->unit_no == unit_no ){ _Rec_Cut( evelist, p ); count++; }
		else if( p->unit_no >  unit_no ){ p->unit_no--;           count++; }
	}
	return count;
}
static s32 _Record_UnitNo_Replace( EVELIST_STRUCT *evelist, u8 old_u, u8 new_u )
{
	if( !evelist->_eves || old_u == new_u ) return 0;

	s32 count = 0;
	if( old_u < new_u )
	{
		for( EVERECORD *p = evelist->_start; p; p = p->next )
		{
			if(      p->unit_no == old_u                        ){ p->unit_no = new_u; count++; }
			else if( p->unit_no >  old_u && p->unit_no <= new_u ){ p->unit_no--;       count++; }
		}
	}
	else
	{
		for( EVERECORD *p = evelist->_start; p; p = p->next )
		{
			if(      p->unit_no == old_u                        ){ p->unit_no = new_u; count++; }
			else if( p->unit_no <  old_u && new_u <= p->unit_no ){ p->unit_no++;       count++; }
		}
	}

	return count;
}



////////////////////////////
// グローバル関数 //////////
////////////////////////////

b32  pxtnEvelist_Initialize( s32 event_num )
{
	if( event_num )
	{
		_Clear_All( &_evelist_main );
		if( !pxMem_zero_alloc( (void**)&_evelist_main._eves, sizeof(EVERECORD) * event_num ) ) return _false;
		_evelist_main._event_num = event_num;
	}
	return _true;
}

void pxtnEvelist_Release         ( void          ){        _Clear_All            ( &_evelist_main ); }
void pxtnEvelist_Reset           ( void          ){        _Clear                ( &_evelist_main ); }
void pxtnEvelist_Linear_Start    ( void          ){        _Clear_w_Linear       ( &_evelist_main ); }
void pxtnEvelist_x4x_Read_Start  ( void          ){        _Clear_w_Linear_x4xRec( &_evelist_main ); }
void pxtnEvelist_Clear_AtExit    ( void          ){        _ClearHelper          ( &_evelist_main ); atexit( _Alt_Release ); }
s32  pxtnEvelist_Get_EventNum    ( void          ){ return _Get_EventNum         ( &_evelist_main ); }
s32  pxtnEvelist_Get_ClockNum    ( void          ){ return _Get_StartNum         ( &_evelist_main ); }
void pxtnEvelist_x4x_Read_NewKind( void          ){        _x4x_Read_NewKind     ( &_evelist_main ); }
void pxtnEvelist_Linear_End      ( b32 b_connect ){        _Linear_End           ( &_evelist_main, b_connect ); }

void pxtnEvelist_Record_Add  ( s32 clock, u8 unit_no, u8 kind, s32 value   ){ _Record_Add_i( &_evelist_main, clock, unit_no, kind, value ); }
void pxtnEvelist_Record_Add_f( s32 clock, u8 unit_no, u8 kind, f32 value_f ){ _Record_Add_i( &_evelist_main, clock, unit_no, kind, *( (s32*)(&value_f) ) ); }
void pxtnEvelist_Linear_Add  ( s32 clock, u8 unit_no, u8 kind, s32 value   ){ _Linear_Add_i( &_evelist_main, clock, unit_no, kind, value ); }
void pxtnEvelist_Linear_Add_f( s32 clock, u8 unit_no, u8 kind, f32 value_f ){ _Linear_Add_i( &_evelist_main, clock, unit_no, kind, *( (s32*)(&value_f) ) ); }
void pxtnEvelist_x4x_Read_Add( s32 clock, u8 unit_no, u8 kind, s32 value   ){ _x4x_Read_Add( &_evelist_main, clock, unit_no, kind, value ); }
b32  pxtnEvelist_Kind_IsTail (                        u8 kind              ){ return ( kind == EVENTKIND_ON || kind == EVENTKIND_PORTAMENT ); }

const EVERECORD *pxtnEvelist_Get_Records( void ){ return (_evelist_main._eves) ? _evelist_main._start : NULL; }


// counters ----------------------
s32  pxtnEvelist_Count_TotalEvent     ( void                                                            ){ return _Get_Count            ( &_evelist_main                                               ); }
s32  pxtnEvelist_Count_TotalUnit      (                                  u8 unit_no                     ){ return _Get_Count            ( &_evelist_main,                         unit_no              ); }
s32  pxtnEvelist_Count_Kind_Value     (                                              u8 kind, s32 value ){ return _Get_Count            ( &_evelist_main,                                  kind, value ); }
s32  pxtnEvelist_Count_Unit_Kind      (                                  u8 unit_no, u8 kind            ){ return _Get_Count            ( &_evelist_main,                         unit_no, kind        ); }
s32  pxtnEvelist_Count_ActiveNotes    ( s32  clock_start, s32 clock_end, u8 unit_no                     ){ return _Get_Count            ( &_evelist_main, clock_start, clock_end, unit_no              ); }
s32  pxtnEvelist_Get_LastValue        ( s32  clock      ,                u8 unit_no, u8 kind            ){ return _Get_Value            ( &_evelist_main, clock      ,            unit_no, kind        ); }
s32  pxtnEvelist_Get_MaxClock         ( void                                                            ){ return _Get_Max_Clock        ( &_evelist_main                                               ); }
s32  pxtnEvelist_Record_Delete        ( s32  clock_start, s32 clock_end, u8 unit_no, u8 kind            ){ return _Record_Delete        ( &_evelist_main, clock_start, clock_end, unit_no, kind        ); }
s32  pxtnEvelist_Record_Delete        ( s32  clock_start, s32 clock_end, u8 unit_no                     ){ return _Record_Delete        ( &_evelist_main, clock_start, clock_end, unit_no              ); }
s32  pxtnEvelist_Record_Clock_Shift   ( s32  clock      , s32 shift    , u8 unit_no                     ){ return _Record_Clock_Shift   ( &_evelist_main, clock      , shift    , unit_no              ); }
s32  pxtnEvelist_Record_Value_Change  ( s32  clock_start, s32 clock_end, u8 unit_no, u8 kind, s32 value ){ return _Record_Value_Change  ( &_evelist_main, clock_start, clock_end, unit_no, kind, value ); }
s32  pxtnEvelist_Record_Value_Set     ( s32  clock_start, s32 clock_end, u8 unit_no, u8 kind, s32 value ){ return _Record_Value_Set     ( &_evelist_main, clock_start, clock_end, unit_no, kind, value ); }
s32  pxtnEvelist_Record_Value_Omit    (                                                       s32 value ){ return _Record_Value_Omit    ( &_evelist_main, EVENTKIND_VOICENO     ,                value ); }
s32  pxtnEvelist_Record_Value_Replace ( s32  old_value  , s32 new_value                                 ){ return _Record_Value_Replace ( &_evelist_main, EVENTKIND_VOICENO     , old_value, new_value ); }
s32  pxtnEvelist_Record_UnitNo_Miss   (                                  u8 unit_no                     ){ return _Record_UnitNo_Miss   ( &_evelist_main,                         unit_no              ); }
s32  pxtnEvelist_Record_UnitNo_Replace(                                  u8 old_u  , u8 new_u           ){ return _Record_UnitNo_Replace( &_evelist_main,                         old_u  , new_u       ); }
