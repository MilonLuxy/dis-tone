#include <StdAfx.h>

#include "pxtnGroup.h"

static s32 _group_num = 0;
void pxtnGroup_Set( s32 group ){ _group_num = group; }
s32  pxtnGroup_Get( void      ){ return _group_num;  }
