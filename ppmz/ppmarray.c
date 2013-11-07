
//#define DEBUG_COMPARE

//#define DEBUG_FINGER
//#define DEBUG_RANGE
#define RANGE_STEP 1
#define RANGE_LOW  119103
#define RANGE_HIGH 999999

#define PROGRESSQUANTUM 0x3FF

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/arithc.h>
#include "ppmcoder.h"

#define NUMSEEDS  		0  /* rawarray is pre-padded, so it's safe to read backwards */
#define MIN_INPUT_LEN	8

long PPMcoder_EncodeArray(ubyte * RawArray,ubyte * ArithArray,ulong RawArrayLen,
  PPM_WriteFunc PPM_Write,PPM_InitFunc PPM_Init,
  ubyte * PreArray,ulong PreArrayLen,bool DoLRU)
{
ubyte *CurRawArrayPtr;
long CurRawFileLen,i,OutFileLen;
struct PPMcoderInfo * PPMCI;
arithInfo * ari;

if ( RawArrayLen < MIN_INPUT_LEN )
  {
  MemCpy(ArithArray,RawArray,RawArrayLen);
  return((long)RawArrayLen);
  }

CurRawArrayPtr = RawArray;
CurRawFileLen = 0;

/*seed with some chars to make the context valid*/
for(i=0;i<NUMSEEDS;i++)
  {
  *ArithArray++ = *CurRawArrayPtr++;
  CurRawFileLen ++;
  }

if ( (ari = arithInit()) == NULL )
  {errputs("\narithInit failed!!\n\n"); return(0); }

arithEncodeInit(ari,ArithArray);

if ( (PPMCI = PPM_WriteInit(ari,256,DoLRU)) == NULL )
  { errputs("\nPPM Allocation failed!!\n\n"); arithFree(ari); return(0); }

if ( ! (*PPM_Init)(PPMCI) )
  { errputs("\nPPM_Init failed!!\n\n"); arithFree(ari); return(0); }

if ( PreArray && PreArrayLen > 8 )
  {
  PreArray += 8; PreArrayLen -= 8;

  fprintf(stderr,"Precondition: %ld to go\r",PreArrayLen); fflush(stderr);

  while(PreArrayLen--)
    {
  
    PPMcoder_UpdateC(PPMCI,*PreArray,
		getulong((PreArray - 4)),getulong((PreArray - 8)));

    PreArray++;

    if ( (PreArrayLen&PROGRESSQUANTUM)==0 )
      { fprintf(stderr,"Precondition: %ld to go\r",PreArrayLen); fflush(stderr); }
    }

  fprintf(stderr,"Precondition: done\n");
  }

	if ( (CurRawFileLen&PROGRESSQUANTUM)==0 ) {
		fprintf(stderr,"%ld/%ld\r",CurRawFileLen,RawArrayLen); fflush(stderr);
    }

while( CurRawFileLen < RawArrayLen )
  {
  (*PPM_Write)(PPMCI,*CurRawArrayPtr,
		getulong((CurRawArrayPtr - 4)),getulong((CurRawArrayPtr - 8)),CurRawArrayPtr);

#ifdef DEBUG_RANGE
  if ( CurRawFileLen >= RANGE_LOW && CurRawFileLen <= RANGE_HIGH ) {
	dbf();
	PPMCI->doDebug = true;
    if ( (CurRawFileLen-RANGE_LOW)%RANGE_STEP == 0 ) {
		printf("%ld/%ld :",CurRawFileLen,RawArrayLen); 
#ifdef DEBUG_FINGER
		PPM_ShowFingerPrint(PPMCI);
#endif
	    if ( !PPM_CheckErr(PPMCI) ) getchar();
	}
  } else {
	PPMCI->doDebug = false;
	}
#endif //debug_range

  CurRawArrayPtr++;
  CurRawFileLen ++;

  if ( (CurRawFileLen&PROGRESSQUANTUM)==0 ) {
	fprintf(stderr,"%ld/%ld\r",CurRawFileLen,RawArrayLen); fflush(stderr);

	if ( arithTellEncPos(ari) > RawArrayLen ) {
		errputs("\nwarning: expanded, coding raw!\n");
			goto EncodeArray_Abort;
		}
    }
  }

	EncodeArray_Abort:

if ( ! PPM_WriteDone(PPMCI) )
  { arithFree(ari); return(RawArrayLen + 1); }

OutFileLen = arithEncodeDone(ari); /* <> */
OutFileLen += NUMSEEDS;

arithFree(ari);

return(OutFileLen);
}

bool PPMcoder_DecodeArray(ubyte * RawArray,ubyte * ArithArray,ulong RawArrayLen,
  PPM_ReadFunc PPM_Read,PPM_InitFunc PPM_Init,
  ubyte * PreArray,ulong PreArrayLen,bool DoLRU,ubyte *DebugCompare)
{
ubyte *CurRawArrayPtr;
long CurRawFileLen,i;
struct PPMcoderInfo * PPMCI;
arithInfo * ari;

if ( RawArrayLen < MIN_INPUT_LEN )
  {
  MemCpy(RawArray,ArithArray,RawArrayLen);
  return(1);
  }

CurRawArrayPtr = RawArray;
CurRawFileLen = 0;

/*seed with some chars to make the context valid*/
for(i=0;i<NUMSEEDS;i++)
  {
  *CurRawArrayPtr++ = *ArithArray++;
  CurRawFileLen ++;
  }

if ( (ari = arithInit()) == NULL )
  { errputs("\narithInit failed!!\n\n"); return(0); }

arithDecodeInit(ari,ArithArray);

if ( (PPMCI = PPM_ReadInit(ari,256,DoLRU)) == NULL )
  { errputs("\nPPM_Allocation failed!!\n\n"); arithFree(ari); return(0); }

if ( ! (*PPM_Init)(PPMCI) )
  { errputs("\nPPM_Init failed!!\n\n"); arithFree(ari); return(0); }

if ( PreArray && PreArrayLen > 8 )
  {
  PreArray += 8; PreArrayLen -= 8;

  fprintf(stderr,"Precondition: %ld to go\r",PreArrayLen);
  fflush(stderr);

  while(PreArrayLen--)
    {
    PPMcoder_UpdateC(PPMCI,*PreArray,
		getulong(PreArray - 4),getulong(PreArray - 8));

    PreArray++;

    if ( (PreArrayLen&PROGRESSQUANTUM)==0 )
      { fprintf(stderr,"Precondition: %ld to go\r",PreArrayLen); fflush(stderr); }
    }

  fprintf(stderr,"Precondition: done           \n");
  }

  if ( (CurRawFileLen&PROGRESSQUANTUM)==0 ) {
	fprintf(stderr,"%ld/%ld\r",CurRawFileLen,RawArrayLen); fflush(stderr);
    }

while( CurRawFileLen < RawArrayLen )
  {
  *CurRawArrayPtr = (*PPM_Read)(PPMCI,
	getulong(CurRawArrayPtr - 4),getulong(CurRawArrayPtr - 8),CurRawArrayPtr);

#ifdef DEBUG_COMPARE
	if ( DebugCompare ) {
		if ( DebugCompare[CurRawFileLen] != *CurRawArrayPtr ) {
			printf("debug compare : expected %3d , got %3d , at %d\n",DebugCompare[CurRawFileLen],*CurRawArrayPtr,CurRawFileLen);
			dbf();
		}
	}
#endif

#ifdef DEBUG_RANGE
  if ( CurRawFileLen >= RANGE_LOW && CurRawFileLen <= RANGE_HIGH ) {
	dbf();
	PPMCI->doDebug = true;
    if ( (CurRawFileLen-RANGE_LOW)%RANGE_STEP == 0 ) {
		printf("%ld/%ld :",CurRawFileLen,RawArrayLen); 
#ifdef DEBUG_FINGER
PPM_ShowFingerPrint(PPMCI); 
#endif
	    if ( !PPM_CheckErr(PPMCI) ) getchar();
	}
  } else {
	PPMCI->doDebug = false;
	}
#endif //debug_range

  CurRawArrayPtr++;
  CurRawFileLen ++;

  if ( (CurRawFileLen&PROGRESSQUANTUM)==0 ) {
	fprintf(stderr,"%ld/%ld\r",CurRawFileLen,RawArrayLen); fflush(stderr);
    }
  }

if ( ! PPM_ReadDone(PPMCI) )
  { arithFree(ari); return(0); }

arithDecodeDone(ari);

arithFree(ari);

return(1);
}
