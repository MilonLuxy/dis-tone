
static enum EVETYPE_PTI
{
	EVETYPE_PTI_NONE  = 0,
	EVETYPE_PTI_WAIT     ,
	EVETYPE_PTI_ON       ,
	EVETYPE_PTI_VOL      ,
	EVETYPE_PTI_KEY      ,
	EVETYPE_PTI_REPEAT   ,
	EVETYPE_PTI_LAST     ,
};

BOOL _Write_Events( unsigned char unit_no, unsigned char kind, unsigned char value, FILE *fp )
{
	if( fwrite( &unit_no, sizeof(unsigned char), 1, fp ) != 1 ) return FALSE;
	if( fwrite( &kind   , sizeof(unsigned char), 1, fp ) != 1 ) return FALSE;
	if( fwrite( &value  , sizeof(unsigned char), 1, fp ) != 1 ) return FALSE;
	return TRUE;
}

BOOL pxtnIO_PTI_Write( const char *path_pti, long beat_divide )
{
	BOOL b_ret        = FALSE;
	BOOL repeat_clock = FALSE;
	FILE *fp_pti      = NULL ;
	FILE *fp_txt      = NULL ;
	long clock;
	long absolute = 0;
	long eve_num  = 0;

	long aiStack_58[15];
	for( int i = 0; i < 5; i++ )
	{
		aiStack_58[ i * 3 + 0 ] = EVENTDEFAULT_VOLUME;
		aiStack_58[ i * 3 + 1 ] = EVENTDEFAULT_VELOCITY;
		aiStack_58[ i * 3 + 2 ] = EVENTDEFAULT_KEY + 0xA00;
	}

	long beat_num; float beat_tempo; long beat_clock;
	pxtnMaster_Get( &beat_num, &beat_tempo, &beat_clock );

	long clock_repeat = beat_clock * beat_num * pxtnMaster_Get_RepeatMeas();
	long clock_last   = beat_clock * beat_num * pxtnMaster_Get_LastMeas  ();
	long div_clock    = beat_clock / beat_divide;

	char path_txt[ 64 + 3 ];
	strcpy( path_txt, path_pti );
	PathRemoveExtension( path_txt );
	strcat( path_txt, ".txt" );

	fp_pti = fopen( path_pti, "wb" ); if( !fp_pti ) goto End;
	fp_txt = fopen( path_txt, "wt" ); if( !fp_txt ) goto End;

	unsigned char btempo  = (unsigned char)beat_tempo;
	unsigned char bdivide = beat_divide;

	if( fwrite( &btempo , sizeof(unsigned char), 1, fp_pti ) != 1 ) goto End;
	if( fwrite( &bdivide, sizeof(unsigned char), 1, fp_pti ) != 1 ) goto End;
	fprintf( fp_txt, "beat-tempo : %d\n", (long)beat_tempo );
	fprintf( fp_txt, "beat-divide: %d\n", beat_divide      );
	fprintf( fp_txt, "div-clock  : %d\n", div_clock        );


	for( const EVERECORD *p = pxtnEvelist_Get_Records(); p; p = p->next )
	{
		if( !repeat_clock && p->clock >= clock_repeat )
		{
			repeat_clock = TRUE;
			clock = ( clock_repeat - absolute ) / div_clock;
			if( !_Write_Events( 0, EVETYPE_PTI_REPEAT, (unsigned char)clock, fp_pti ) ) goto End;
			fprintf( fp_txt, "%02d repeat\n", clock );
			absolute = clock_repeat;
			eve_num++;
		}
		clock = ( p->clock - absolute ) / div_clock;

		if( clock )
		{
			if( !_Write_Events( 0, EVETYPE_PTI_WAIT, (unsigned char)clock, fp_pti ) ) goto End;
			fprintf( fp_txt, "%02d\n", clock );
			absolute = p->clock;
			eve_num++;
		}

		UINT uVar6 = (long)p->unit_no;
		long *piVar1 = aiStack_58 + uVar6 * 3;

		bool b_eve = false;
		unsigned char pti_unit = p->unit_no;
		unsigned char pti_kind;
		unsigned char pti_value;

		switch( p->kind )
		{
			case EVENTKIND_ON:
				{
					pti_kind  = EVETYPE_PTI_ON;
					pti_value = (unsigned char)( p->unit_no / div_clock );
					b_eve     = true;
				}
				break;
			case EVENTKIND_KEY:
				if( aiStack_58[uVar6 * 3 + 2] != p->unit_no )
				{
					aiStack_58[uVar6 * 3 + 2] = p->unit_no;
					pti_kind  = EVETYPE_PTI_KEY;
					pti_value = (char)(p->unit_no + (p->unit_no >> 0x1f & 0xffU) >> 8) + 0xbe;
					b_eve     = true;
				}
				break;
			case EVENTKIND_VELOCITY:
				if( *piVar1 != p->unit_no )
				{
					*piVar1   = p->unit_no;
					pti_kind  = EVETYPE_PTI_VOL;
					pti_value = (unsigned char)( (*piVar1 * aiStack_58[uVar6 * 3 + 1]) / 0x7f );
					b_eve     = true;
				}
				break;
			case EVENTKIND_VOLUME:
				if( aiStack_58[uVar6 * 3 + 1] != p->unit_no )
				{
					aiStack_58[uVar6 * 3 + 1] = p->unit_no;
					pti_kind  = EVETYPE_PTI_VOL;
					pti_value = (unsigned char)( (*piVar1 * aiStack_58[uVar6 * 3 + 1]) / 0x7f );
					b_eve     = true;
				}
				break;
		}

		if( b_eve )
		{
			if( !_Write_Events( pti_unit, pti_kind, pti_value, fp_pti ) ) goto End;
			fprintf( fp_txt, "\tunit: %02d, kind: %s v:%d\n", (long)p->unit_no, p->kind, (long)pti_value );
			eve_num++;
		}
	}

	if( clock_last )
	{
		clock = ( clock_last - absolute ) / div_clock;
		if( !_Write_Events( 0, EVETYPE_PTI_LAST, (unsigned char)clock, fp_pti ) ) goto End;
		fprintf( fp_txt, "%02d last\n", clock );
		absolute = clock_repeat;
		eve_num++;
	}
	fprintf( fp_txt, "event num: %d\n", eve_num );

	b_ret = TRUE;
End:
	if( fp_txt ) fclose( fp_txt );
	if( fp_pti ) fclose( fp_pti );

	return b_ret;
}
