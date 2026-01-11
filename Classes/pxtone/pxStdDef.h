#ifndef pxStdDef_H
#define pxStdDef_H

#define pxINCLUDE_OGGVORBIS 1
// $(SolutionDir)libogg\include;$(SolutionDir)libvorbis\include;

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef unsigned char      u8 ;
typedef   signed char      s8 ;
typedef unsigned short     u16;
typedef   signed short     s16;
#ifdef _WIN32
typedef unsigned long      u32;
typedef   signed long      s32;
#else
typedef unsigned int       u32;
typedef   signed int       s32;
#endif
typedef unsigned long long u64;
typedef   signed long long s64;

typedef float              f32;
typedef double             f64;

typedef bool               b8 ; // 1 byte bool
typedef int                b32; // 4 byte bool (BOOL)

#define _false               0 // 'FALSE' instead of 'false'
#define _true                1 // 'TRUE' instead of 'true'
#define MAX_S8BIT          127
#define MAX_U8BIT          255
#define MAX_S16BIT       32767
#define MAX_U16BIT       65535


typedef struct
{
    s32  x;
    s32  y;
}
pxPOINT;


#endif
