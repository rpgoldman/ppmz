#ifndef PPMARRAY_H
#define PPMARRAY_H

#include "ppmcoder.h"

extern long PPMcoder_EncodeArray(ubyte * RawArray,ubyte * ArithArray,ulong RawArrayLen,
  PPM_WriteFunc PPM_Write,PPM_InitFunc PPM_Init,
  ubyte * PreArray,ulong PreLen,bool DoLRU);

extern bool PPMcoder_DecodeArray(ubyte * RawArray,ubyte * ArithArray,ulong RawArrayLen,
  PPM_ReadFunc PPM_Read,PPM_InitFunc PPM_Init,
  ubyte * PreArray,ulong PreLen,bool DoLRU,ubyte *DebugCompare);

#endif
