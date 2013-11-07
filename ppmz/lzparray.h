#ifndef LZPARRAY_H
#define LZPARRAY_H

#include "ppmcoder.h"

extern long LZP_EncodeArray(ubyte * RawArray,
	ubyte * ArithArray,ulong RawArrayLen,
	PPM_WriteFunc PPM_Write,PPM_InitFunc PPM_Init);

extern bool LZP_DecodeArray(ubyte * RawArray,
	ubyte * ArithArray,ulong RawArrayLen,
	PPM_ReadFunc PPM_Read,PPM_InitFunc PPM_Init);

#endif
