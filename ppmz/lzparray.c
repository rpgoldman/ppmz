
/***

doing _update hurts a lot.

--> after changing settings this may not be true anymore

coder 17 better than 9 ?

*****/

// #define UPDATE_ALL /* update literals in a match */

#define DO_LAM_DEF 0  // doesn't seem to matter

#define LZPO12      // else LZP_DET

#define PROGRESSQUANTUM 1000

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/arithc.h>
#include "ppmcoder.h"

#ifdef LZPO12
#include "lzpo12.h"
#else
#include "lzpdet.h"
#endif // LZPO12

#define MIN_INPUT_LEN 8

static const bool DoLRU=0, DoLAM = DO_LAM_DEF;

long LZP_EncodeArray(ubyte * RawArray,
  ubyte * ArithArray,ulong RawArrayLen,
  PPM_WriteFunc PPM_Write,PPM_InitFunc PPM_Init)
{
ubyte *CurRawArrayPtr;
long CurRawFileLen,OutFileLen;
ulong Context,Context2;
struct PPMcoderInfo * PPMCI;
arithInfo * ari;
#ifdef LZPO12
struct LZPo12Info * L12I;
#else
struct LZPdetInfo * LZPDI;
#endif
long MatchLen,Exclude,LastProgress=0;

if ( RawArrayLen < MIN_INPUT_LEN )
  {
  MemCpy(ArithArray,RawArray,RawArrayLen);
  return((long)RawArrayLen);
  }

CurRawArrayPtr = RawArray;
CurRawFileLen = 0;

if ( (ari = arithInit()) == NULL )
  {errputs("\narithInit failed!!\n\n"); return(0); }

arithEncodeInit(ari,ArithArray);

if ( (PPMCI = PPM_WriteInit(ari,256,DoLRU)) == NULL )
  { errputs("\nPPM Allocation failed!!\n\n"); arithFree(ari); return(0); }

if ( ! PPM_Init(PPMCI) )
  { errputs("\nPPM_Init failed!!\n\n"); arithFree(ari); return(0); }

#ifdef LZPO12

if ( (L12I = LZPo12_Init(ari)) == NULL )
  { errputs("\nLZPo12_Init failed!!\n\n"); PPMcoder_CleanUp(PPMCI); arithFree(ari); return(0); }

#else

if ( (LZPDI = LZPdet_Init(ari)) == NULL )
  { errputs("\nLZPdet_Init failed!!\n\n"); PPMcoder_CleanUp(PPMCI); arithFree(ari); return(0); }

#endif

while( CurRawFileLen < RawArrayLen )
  {
#ifdef LZPO12
  MatchLen = LZPo12_Write(L12I ,CurRawArrayPtr,&Exclude);
#else
  MatchLen = LZPdet_Write(LZPDI,CurRawArrayPtr,&Exclude);
#endif

  CurRawArrayPtr += MatchLen;
  CurRawFileLen  += MatchLen;

  if ( MatchLen == 0 || DoLAM )
    {
    Context2 = getulong(CurRawArrayPtr - 8);
    Context  = getulong(CurRawArrayPtr - 4);
    PPM_Write(PPMCI,*CurRawArrayPtr,Context,Context2,CurRawArrayPtr);
    CurRawArrayPtr++;
    CurRawFileLen ++;
    }

  if ( (CurRawFileLen-LastProgress) >= PROGRESSQUANTUM ) 
    {
    fprintf(stderr,"%ld/%ld\r",CurRawFileLen,RawArrayLen); fflush(stderr);
    if ( !PPM_CheckErr(PPMCI) ) { getchar(); PPMcoder_CleanUp(PPMCI);
      arithFree(ari); }
    LastProgress = CurRawFileLen;

	
	if ( arithTellEncPos(ari) > RawArrayLen ) {
		errputs("\nwarning: expanded, coding raw!\n");
		goto LZP_EncodeArray_Abort;
		}
    }
  }

	LZP_EncodeArray_Abort:

if ( ! PPM_WriteDone(PPMCI) )
  { arithFree(ari); return(0); }

OutFileLen = arithEncodeDone(ari); /* <> */

#ifdef LZPO12
LZPo12_CleanUp(L12I);
#else
LZPdet_CleanUp(LZPDI);
#endif

arithFree(ari);

return(OutFileLen);
}

bool LZP_DecodeArray(ubyte * RawArray,
  ubyte * ArithArray,ulong RawArrayLen,
  PPM_ReadFunc PPM_Read,PPM_InitFunc PPM_Init)
{
ubyte *CurRawArrayPtr;
long CurRawFileLen;
ulong Context,Context2;
struct PPMcoderInfo * PPMCI;
arithInfo * ari;

#ifdef LZPO12
struct LZPo12Info * L12I;
#else
struct LZPdetInfo * LZPDI;
#endif

long MatchLen,Exclude,LastProgress=0;

if ( RawArrayLen < MIN_INPUT_LEN )
  {
  MemCpy(RawArray,ArithArray,RawArrayLen);
  return(1);
  }

CurRawArrayPtr = RawArray;
CurRawFileLen = 0;

if ( (ari = arithInit()) == NULL )
  { errputs("\narithInit failed!!\n\n"); return(0); }

arithDecodeInit(ari,ArithArray);

if ( (PPMCI = PPM_ReadInit(ari,256,DoLRU)) == NULL )
  { errputs("\nPPM_Allocation failed!!\n\n"); arithFree(ari); return(0); }

if ( ! PPM_Init(PPMCI) )
  { errputs("\nPPM_Init failed!!\n\n"); arithFree(ari); return(0); }

#ifdef LZPO12

if ( (L12I = LZPo12_Init(ari)) == NULL )
  { errputs("\nLZPo12_Init failed!!\n\n"); PPMcoder_CleanUp(PPMCI); arithFree(ari); return(0); }

#else

if ( (LZPDI = LZPdet_Init(ari)) == NULL )
  { errputs("\nLZPdet_Init failed!!\n\n"); PPMcoder_CleanUp(PPMCI); arithFree(ari); return(0); }

#endif

while( CurRawFileLen < RawArrayLen )
  {
#ifdef LZPO12
  MatchLen = LZPo12_Read(L12I ,CurRawArrayPtr,&Exclude);
#else
  MatchLen = LZPdet_Read(LZPDI,CurRawArrayPtr,&Exclude);
#endif

  CurRawArrayPtr += MatchLen;
  CurRawFileLen  += MatchLen;

  if ( MatchLen == 0 || DoLAM )
    {
    Context2 = getulong(CurRawArrayPtr - 8);
    Context  = getulong(CurRawArrayPtr - 4);
    *CurRawArrayPtr = PPM_Read(PPMCI,Context,Context2,CurRawArrayPtr);
	CurRawArrayPtr++;
    CurRawFileLen ++;
    }

  if ( (CurRawFileLen-LastProgress) >= PROGRESSQUANTUM ) 
    { fprintf(stderr,"%ld/%ld\r",CurRawFileLen,RawArrayLen); fflush(stderr);
    if ( !PPM_CheckErr(PPMCI) ) { getchar(); PPMcoder_CleanUp(PPMCI);
      arithFree(ari); }
    LastProgress = CurRawFileLen;
    }
  }

if ( ! PPM_ReadDone(PPMCI) )
  { arithFree(ari); return(0); }

arithDecodeDone(ari);

#ifdef LZPO12
LZPo12_CleanUp(L12I);
#else
LZPdet_CleanUp(LZPDI);
#endif

arithFree(ari);

return(1);
}
