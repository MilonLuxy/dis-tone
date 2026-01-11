#ifndef pxtnPulse_Oscillator_H
#define pxtnPulse_Oscillator_H

#include "pxStdDef.h"

class pxtnOscillator
{
private:

	pxPOINT *_p_point;
	s32      _point_num;
	s32      _point_reso;
	s32      _volume;
	s32      _sample_num;

public :

	void ReadyGetSample( pxPOINT *p_point, s32 point_num, s32 volume, s32 sample_num, s32 point_reso );
	f64  GetOneSample_Overtone ( s32 index );
	f64  GetOneSample_Coodinate( s32 index );
};

#endif