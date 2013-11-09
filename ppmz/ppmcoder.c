
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/arithc.h>

#include "exclude.h"
#include "order0.h"
#include "ppmz.h"
#include "ppmdet.h"
#include "ppmz_cfg.h" /* for scaledowns */

struct PPMcoderInfo {
	bool doDebug;

  long NumSymbols;
  arithInfo * ari; /* copied in */

  bool DoLRU;
  int PPMZ_GlobalFlags;

  escInfo * escapeInfo;

  exclusion * Exclusion;
  struct Order0Info * O0I;

  struct PPMZ_MemPools * Pools;
  struct PPMZInfo * O1_PPMI;
  struct PPMZInfo * O2_PPMI;
  struct PPMZInfo * O3_PPMI;
  struct PPMZInfo * O4_PPMI;
  struct PPMZInfo * O5_PPMI;
  struct PPMZInfo * O6_PPMI;
  struct PPMZInfo * O7_PPMI;
  struct PPMZInfo * O8_PPMI;

  struct PPMdetInfo * PPMdetI;
  };


/********/

struct PPMcoderInfo * PPM_WriteInit(arithInfo * ari,long NumSymbols,bool DoLRU);
bool PPM_WriteDone(struct PPMcoderInfo * PPMCI);

struct PPMcoderInfo * PPM_ReadInit(arithInfo * ari,long NumSymbols,bool DoLRU);
bool PPM_ReadDone(struct PPMcoderInfo * PPMCI);

struct PPMcoderInfo * PPMcoder_Init(arithInfo * ari,long NumSymbols,bool DoLRU);
void PPMcoder_CleanUp(struct PPMcoderInfo * PPMCI);

typedef void (*PPM_WriteFunc) (struct PPMcoderInfo *,long,ulong,ulong,ubyte *);
typedef long (*PPM_ReadFunc)  (struct PPMcoderInfo *,ulong,ulong,ubyte *);
typedef bool (*PPM_InitFunc)  (struct PPMcoderInfo *);

/********/


struct PPMcoderInfo * PPMcoder_Init(arithInfo * ari,long NumSymbols,bool DoLRU)
{
struct PPMcoderInfo * PPMCI;

if ( (PPMCI = AllocMem(sizeof(struct PPMcoderInfo),MEMF_CLEAR)) == NULL )
  return(NULL);

PPMCI->doDebug = false;
PPMCI->ari = ari;
PPMCI->NumSymbols = NumSymbols;

PPMCI->DoLRU = DoLRU;

if ( PPMCI->DoLRU ) 
  PPMCI->PPMZ_GlobalFlags = PPMZF_LRU;
else
  PPMCI->PPMZ_GlobalFlags = 0;

if ( (PPMCI->Pools = PPMZ_AllocPools()) == NULL )
  { PPMcoder_CleanUp(PPMCI); return(NULL); }

if ( (PPMCI->O0I = Order0_Init(PPMCI->ari,NumSymbols)) == NULL )
  { PPMcoder_CleanUp(PPMCI); return(NULL); }

if ( (PPMCI->Exclusion = newExclusion(PPMCI->NumSymbols)) == NULL )
  { PPMcoder_CleanUp(PPMCI); return(NULL); }

if ( (PPMCI->escapeInfo = PPMZesc_open(ari)) == NULL )
  { PPMcoder_CleanUp(PPMCI); return(NULL); }

return(PPMCI);
}

void PPMcoder_CleanUp(struct PPMcoderInfo * PPMCI)
{
if ( ! PPMCI ) return;
if ( PPMCI->escapeInfo ) PPMZesc_close(PPMCI->escapeInfo);
if ( PPMCI->Exclusion ) freeExclusion(PPMCI->Exclusion);
if ( PPMCI->O0I ) Order0_CleanUp(PPMCI->O0I);
if ( PPMCI->Pools   ) PPMZ_FreePools(PPMCI->Pools);
if ( PPMCI->O1_PPMI ) PPMZ_CleanUp(PPMCI->O1_PPMI);
if ( PPMCI->O2_PPMI ) PPMZ_CleanUp(PPMCI->O2_PPMI);
if ( PPMCI->O3_PPMI ) PPMZ_CleanUp(PPMCI->O3_PPMI);
if ( PPMCI->O4_PPMI ) PPMZ_CleanUp(PPMCI->O4_PPMI);
if ( PPMCI->O5_PPMI ) PPMZ_CleanUp(PPMCI->O5_PPMI);
if ( PPMCI->O6_PPMI ) PPMZ_CleanUp(PPMCI->O6_PPMI);
if ( PPMCI->O7_PPMI ) PPMZ_CleanUp(PPMCI->O7_PPMI);
if ( PPMCI->O8_PPMI ) PPMZ_CleanUp(PPMCI->O8_PPMI);
if ( PPMCI->PPMdetI ) PPMdet_CleanUp(PPMCI->PPMdetI);

free(PPMCI);
}

struct PPMcoderInfo * PPM_WriteInit(arithInfo * ari,long NumSymbols,bool DoLRU)
{
struct PPMcoderInfo * PPMCI;

if ( (PPMCI = PPMcoder_Init(ari,NumSymbols,DoLRU)) == NULL )
  return(NULL);

return(PPMCI);
}

bool PPMcoder_AddO1(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O1_PPMI ) return(1);
if ( (PPMCI->O1_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,
  O1_ScaleDown,PPMCI->Pools,1,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddO2(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O2_PPMI ) return(1);
if ( (PPMCI->O2_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,O2_ScaleDown,PPMCI->Pools,2,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddO3(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O3_PPMI ) return(1);
if ( (PPMCI->O3_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,O3_ScaleDown,PPMCI->Pools,3,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddO4(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O4_PPMI ) return(1);
if ( (PPMCI->O4_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,
  O4_ScaleDown,PPMCI->Pools,4,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddO5(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O5_PPMI ) return(1);
if ( (PPMCI->O5_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,
  O5_ScaleDown,PPMCI->Pools,5,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddO6(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O6_PPMI ) return(1);
if ( (PPMCI->O6_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,
  O6_ScaleDown,PPMCI->Pools,6,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddO7(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O7_PPMI ) return(1);
if ( (PPMCI->O7_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,
  O7_ScaleDown,PPMCI->Pools,7,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddO8(struct PPMcoderInfo * PPMCI,int Flags)
{
if ( PPMCI->O8_PPMI ) return(1);
if ( (PPMCI->O8_PPMI = PPMZ_Init(PPMCI->ari,Flags|PPMCI->PPMZ_GlobalFlags,
  O8_ScaleDown,PPMCI->Pools,8,PPMCI->escapeInfo)) == NULL )
  return(0);
return(1);
}

bool PPMcoder_AddDet(struct PPMcoderInfo * PPMCI)
{
if ( PPMCI->PPMdetI ) return(1);
if ( (PPMCI->PPMdetI = PPMdet_Init(PPMCI->ari)) == NULL )
  return(0);
return(1);
}

bool PPM_CheckErr(struct PPMcoderInfo * PPMCI)
{
bool Ret=1;

#define CHECKERROR(zz) if ( PPMCI->zz && PPMCI->zz->Error ) { Ret =0; fprintf(stderr,"\n PPMZ order %d Interal Error : %ld\n\n",PPMCI->zz->order,PPMCI->zz->Error); dbf(); }

CHECKERROR(O1_PPMI);
CHECKERROR(O2_PPMI);
CHECKERROR(O3_PPMI);
CHECKERROR(O4_PPMI);
CHECKERROR(O5_PPMI);
CHECKERROR(O6_PPMI);
CHECKERROR(O7_PPMI);
CHECKERROR(O8_PPMI);
//CHECKERROR(PPMdetI);

return(Ret);
}

bool PPM_Done(struct PPMcoderInfo * PPMCI)
{
bool Ret=PPM_CheckErr(PPMCI);

PPMcoder_CleanUp(PPMCI);

return(Ret);
}

bool PPM_WriteDone(struct PPMcoderInfo * PPMCI)
{
return(PPM_Done(PPMCI));
}

struct PPMcoderInfo * PPM_ReadInit(arithInfo * ari,long NumSymbols,bool DoLRU)
{
struct PPMcoderInfo * PPMCI;

if ( (PPMCI = PPMcoder_Init(ari,NumSymbols,DoLRU)) == NULL )
  return(NULL);

return(PPMCI);
}

bool PPM_ReadDone(struct PPMcoderInfo * PPMCI)
{
return(PPM_Done(PPMCI));
}

void PPMcoder_UpdateC(struct PPMcoderInfo *PPMCI,long GotC,ulong Cntx,ulong Cntx2)
{
/** PPMdet not done right now <> **/
if ( PPMCI->O8_PPMI ) if ( PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Cntx,Cntx2) ) return;
if ( PPMCI->O5_PPMI ) if ( PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Cntx,Cntx2) ) return;
if ( PPMCI->O4_PPMI ) if ( PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Cntx,Cntx2) ) return;
if ( PPMCI->O3_PPMI ) if ( PPMZ_UpdateC(PPMCI->O3_PPMI,GotC,Cntx,Cntx2) ) return;
if ( PPMCI->O2_PPMI ) if ( PPMZ_UpdateC(PPMCI->O2_PPMI,GotC,Cntx,Cntx2) ) return;
if ( PPMCI->O1_PPMI ) if ( PPMZ_UpdateC(PPMCI->O1_PPMI,GotC,Cntx,Cntx2) ) return;
/** Order0 not done right now <> **/
return;
}

/********* ********* ***********/


bool PPM_Init0(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO2(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO3(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO4(PPMCI,PPMZF_DT_ONE)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO5(PPMCI,PPMZF_DT_ONE)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO8(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddDet(PPMCI)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init1(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO2(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO3(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO4(PPMCI,PPMZF_DT_ONE)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO5(PPMCI,PPMZF_DT_ONE)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO7(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddDet(PPMCI)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init2(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO2(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO3(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO4(PPMCI,PPMZF_DT_ONE)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO5(PPMCI,PPMZF_DT_ONE)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO6(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddDet(PPMCI)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init3(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO2(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO3(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO4(PPMCI,PPMZF_DT_ONE)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO5(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddDet(PPMCI)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init4(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO2(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO3(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO4(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddDet(PPMCI)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init5(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO2(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO3(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init6(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,0)) { PPMcoder_CleanUp(PPMCI); return(0); }
if(!PPMcoder_AddO2(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init7(struct PPMcoderInfo * PPMCI)
{
if(!PPMcoder_AddO1(PPMCI,PPMZF_DT_TWO)) { PPMcoder_CleanUp(PPMCI); return(0); }
return(1);
}

bool PPM_Init8(struct PPMcoderInfo * PPMCI)
{
return(1);
}

void PPM_Write0(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{
clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_EncodeC(PPMCI->PPMdetI,GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_EncodeC(PPMCI->O8_PPMI,GotC,Context,Cntx2,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_EncodeC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
              {
              Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
              }
            }
          }
        }
      }
    }
  }
}

long PPM_Read0(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;

clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_DecodeC(PPMCI->PPMdetI,&GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_DecodeC(PPMCI->O8_PPMI,&GotC,Context,Cntx2,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_DecodeC(PPMCI->O5_PPMI,&GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
              {
              Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
              PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
              }
            PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
            }
          PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
          }
        PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
        }
      PPMZ_DecodeGotC(PPMCI->O5_PPMI,GotC);
      }
    PPMZ_DecodeGotC(PPMCI->O8_PPMI,GotC);
    }
  PPMdet_DecodeGotC(PPMCI->PPMdetI,GotC);
  }

return(GotC);
}


void PPM_Write1(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{


clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_EncodeC(PPMCI->PPMdetI,GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_EncodeC(PPMCI->O7_PPMI,GotC,Context,Cntx2&0xFFFFFF,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_EncodeC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
              {
              Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
              }
            }
          }
        }
      }
    }
  }
}

long PPM_Read1(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;


clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_DecodeC(PPMCI->PPMdetI,&GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_DecodeC(PPMCI->O7_PPMI,&GotC,Context,Cntx2&0xFFFFFF,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_DecodeC(PPMCI->O5_PPMI,&GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
              {
              Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
              PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
              }
            PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
            }
          PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
          }
        PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
        }
      PPMZ_DecodeGotC(PPMCI->O5_PPMI,GotC);
      }
    PPMZ_DecodeGotC(PPMCI->O7_PPMI,GotC);
    }
  PPMdet_DecodeGotC(PPMCI->PPMdetI,GotC);
  }

return(GotC);
}

void PPM_Write2(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{
clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_EncodeC(PPMCI->PPMdetI,GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_EncodeC(PPMCI->O6_PPMI,GotC,Context,Cntx2&0xFFFF,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_EncodeC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
              {
              Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
              }
            }
          }
        }
      }
    }
  }
}

long PPM_Read2(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;

clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_DecodeC(PPMCI->PPMdetI,&GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_DecodeC(PPMCI->O6_PPMI,&GotC,Context,Cntx2&0xFFFF,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_DecodeC(PPMCI->O5_PPMI,&GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
              {
              Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
              PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
              }
            PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
            }
          PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
          }
        PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
        }
      PPMZ_DecodeGotC(PPMCI->O5_PPMI,GotC);
      }
    PPMZ_DecodeGotC(PPMCI->O6_PPMI,GotC);
    }
  PPMdet_DecodeGotC(PPMCI->PPMdetI,GotC);
  }

return(GotC);
}

void PPM_Write3(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{

clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_EncodeC(PPMCI->PPMdetI,GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_EncodeC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
            {
            Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
            }
          }
        }
      }
    }
  }
}

long PPM_Read3(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;


clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_DecodeC(PPMCI->PPMdetI,&GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_DecodeC(PPMCI->O5_PPMI,&GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
            {
            Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
            PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
            }
          PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
          }
        PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
        }
      PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
      }
    PPMZ_DecodeGotC(PPMCI->O5_PPMI,GotC);
    }
  PPMdet_DecodeGotC(PPMCI->PPMdetI,GotC);
  }

return(GotC);
}

void PPM_Write4(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{


clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_EncodeC(PPMCI->PPMdetI,GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
          {
          Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
          }
        }
      }
    }
  }
}

long PPM_Read4(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;


clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_DecodeC(PPMCI->PPMdetI,&GotC,curPtr,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
          {
          Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
          PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
          }
        PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
        }
      PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
      }
    PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
    }
  PPMdet_DecodeGotC(PPMCI->PPMdetI,GotC);
  }

return(GotC);
}


void PPM_Write5(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{


clearExclusion(PPMCI->Exclusion);

if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
      {
      Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
      }
    }
  }

}

long PPM_Read5(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;


clearExclusion(PPMCI->Exclusion);

if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
    {
    if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
      {
      Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
      PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
      }
    PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
    }
  PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
  }

return(GotC);
}

void PPM_Write6(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{


clearExclusion(PPMCI->Exclusion);

if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
    {
    Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
    }
  }

}

long PPM_Read6(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;


clearExclusion(PPMCI->Exclusion);

if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
  {
  if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
    {
    Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
    PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
    }
  PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
  }

return(GotC);
}

void PPM_Write7(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{


clearExclusion(PPMCI->Exclusion);

if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
  {
  Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
  }

}

long PPM_Read7(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;


clearExclusion(PPMCI->Exclusion);

if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
  {
  Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
  PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
  }

return(GotC);
}

void PPM_Write8(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{

clearExclusion(PPMCI->Exclusion);
Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
}

long PPM_Read8(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{
long GotC;

clearExclusion(PPMCI->Exclusion);
Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
return(GotC);
}


/*************** LOE section ***********

Coder 0_A routines are identical to Coder 0 , but use
  local-order-estimation heuristics

 Order-Ranking PseudoEntropy functions are at the heart of LOE
 the order with the highest PseudoEntropy Rank is used

**/

typedef long (*EntropyFuncType) (long,long,long);

#include <crblib/intmath.h>

#define SCALE_BITS 12
#define ESC_SCALE (1<<SCALE_BITS)

#define zlog(x) ((long)(intlog2(x) - SCALE_BITS))

#define P_ENT_BYTE zlog(ESC_SCALE/256)

long PseudoEntropy0(long P,long E,long H)
  { return( P ); } /* same as P*zlog(P) */

long PseudoEntropy1(long P,long E,long H) 
  { return( (ZCODER_P *P*zlog(P)) + (ZCODER_E *E*(zlog(E) - ZCODER_BYTE)) + 
		(ZCODER_1P * (ESC_SCALE - P)*(zlog(E) - ZCODER_BYTE)) ); }

long PseudoEntropy2(long P,long E,long H) 
  { return( 15*zlog(P) + zlog(E) ); }

long PseudoEntropy3(long P,long E,long H)
  { return( P*zlog(P) + (ESC_SCALE-P)*( zlog(E) + P_ENT_BYTE ) ); }

long PseudoEntropy4(long P,long E,long H)
  { return( P*zlog(P) + ( (ESC_SCALE-P) + E ) * P_ENT_BYTE ); }

long PseudoEntropy5(long P,long E,long H)
  { return( P*zlog(P) + E*zlog(E) + (ESC_SCALE-P) * P_ENT_BYTE ); }

long PseudoEntropy6(long P,long E,long H)
  { return( P*zlog(P) + E*zlog(E) + 8*(ESC_SCALE-P)*zlog(ESC_SCALE-P) ); }

long PseudoEntropy7(long P,long E,long H)
  { return( P*zlog(P) + (ESC_SCALE-P)*( zlog(P) + zlog(E) + P_ENT_BYTE ) ); }

long PseudoEntropy8(long P,long E,long H)
  { return( P*zlog(P) + (ESC_SCALE-P)*( zlog(P) + zlog(E) + 2*P_ENT_BYTE ) ); }

long PseudoEntropy9(long P,long E,long H)
  { return( 10*P - E ); }

long PseudoEntropy10(long P,long E,long H)
  { return( E*( zlog(E) + P_ENT_BYTE ) + (ESC_SCALE-P)*P_ENT_BYTE ); }

/*********************************/

void PPM_Write_LOE(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,
  ubyte * curPtr,EntropyFuncType EntropyFunc);

long PPM_Read_LOE(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,
  ubyte * curPtr,EntropyFuncType EntropyFunc);

void PPM_Write0_A(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy0); }

long PPM_Read0_A(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy0) ); }

void PPM_Write0_B(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy1); }

long PPM_Read0_B(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy1) ); }

void PPM_Write0_C(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy2); }

long PPM_Read0_C(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy2) ); }

void PPM_Write0_D(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy3); }

long PPM_Read0_D(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy3) ); }

void PPM_Write0_E(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy4); }

long PPM_Read0_E(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy4) ); }

void PPM_Write0_F(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy5); }

long PPM_Read0_F(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy5) ); }

void PPM_Write0_G(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy6); }

long PPM_Read0_G(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy6) ); }

void PPM_Write0_H(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy7); }

long PPM_Read0_H(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy7) ); }

void PPM_Write0_I(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy8); }

long PPM_Read0_I(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy8) ); }

void PPM_Write0_J(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy9); }

long PPM_Read0_J(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy9) ); }

void PPM_Write0_K(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte * curPtr)
{ PPM_Write_LOE(PPMCI,GotC,Context,Cntx2,curPtr,PseudoEntropy10); }

long PPM_Read0_K(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,ubyte * curPtr)
{ return( PPM_Read_LOE(PPMCI,Context,Cntx2,curPtr,PseudoEntropy10) ); }


/******************* LOE internals *******************/

#define LOE_GETINFO()                                                      \
ulong O8P,O5P,O4P,O3P,O2P,O1P,O0P;                                         \
ulong O8E,O5E,O4E,O3E,O2E,O1E,O0E;                                         \
ulong O8H,O5H,O4H,O3H,O2H,O1H,O0H;                                         \
                                                                           \
PPMZ_GetInfo(PPMCI->O8_PPMI,Context,Cntx2     ,&O8P,&O8E,&O8H);            \
PPMZ_GetInfo(PPMCI->O5_PPMI,Context,Cntx2&0xFF,&O5P,&O5E,&O5H);            \
PPMZ_GetInfo(PPMCI->O4_PPMI,Context,0         ,&O4P,&O4E,&O4H);            \
PPMZ_GetInfo(PPMCI->O3_PPMI,Context&0xFFFFFF,0,&O3P,&O3E,&O3H);            \
PPMZ_GetInfo(PPMCI->O2_PPMI,Context&0xFFFF  ,0,&O2P,&O2E,&O2H);            \
PPMZ_GetInfo(PPMCI->O1_PPMI,Context&0xFF    ,0,&O1P,&O1E,&O1H);            \
O0P = ESC_SCALE/64; O0E = ESC_SCALE/128; /* rough */                       \
O0H = ESC_SCALE * 6;
/****/

void PPM_Write_LOE(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,
  ubyte * curPtr,EntropyFuncType EntropyFunc)
{
clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_EncodeC(PPMCI->PPMdetI,GotC,curPtr,PPMCI->Exclusion) )
  {
  LOE_GETINFO();

  O8P = (*EntropyFunc)(O8P,O8E,O8H);
  O5P = (*EntropyFunc)(O5P,O5E,O5H);
  O4P = (*EntropyFunc)(O4P,O4E,O4H);
  O3P = (*EntropyFunc)(O3P,O3E,O3H);
  O2P = (*EntropyFunc)(O2P,O2E,O2H);
  O1P = (*EntropyFunc)(O1P,O1E,O1H);
  O0P = (*EntropyFunc)(O0P,O0E,O0H);

  if ( O8P >= O5P && O8P >= O4P && O8P >= O3P && O8P >= O2P && O8P >= O1P && O8P >= O0P )
    {
    if ( ! PPMZ_EncodeC(PPMCI->O8_PPMI,GotC,Context,Cntx2,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_EncodeC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
                }
              }
            }
          }
        }
      }
    }
  else if ( O5P >= O4P && O5P >= O3P && O5P >= O2P && O5P >= O1P && O5P >= O0P )
    {

      if ( ! PPMZ_EncodeC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
                }
              }
            }
          }
        }
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O4P >= O3P && O4P >= O2P && O4P >= O1P && O4P >= O0P )
    {
        if ( ! PPMZ_EncodeC(PPMCI->O4_PPMI,GotC,Context,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
                }
              }
            }
          }
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O3P >= O2P && O3P >= O1P && O3P >= O0P )
    {
          if ( ! PPMZ_EncodeC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
                }
              }
            }
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O2P >= O1P && O2P >= O0P )
    {
            if ( ! PPMZ_EncodeC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
                }
              }
    PPMZ_UpdateC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0);
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O1P >= O0P )
    {
              if ( ! PPMZ_EncodeC(PPMCI->O1_PPMI,GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
                }
    PPMZ_UpdateC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0);
    PPMZ_UpdateC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0);
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else
    {
    Order0_EncodeC(PPMCI->O0I,GotC,PPMCI->Exclusion);
    PPMZ_UpdateC(PPMCI->O1_PPMI,GotC,Context&0xFF,0);
    PPMZ_UpdateC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0);
    PPMZ_UpdateC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0);
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);	
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  }
}

long PPM_Read_LOE(struct PPMcoderInfo * PPMCI,ulong Context,ulong Cntx2,
  ubyte * curPtr,EntropyFuncType EntropyFunc)
{
long GotC;

clearExclusion(PPMCI->Exclusion);

if ( ! PPMdet_DecodeC(PPMCI->PPMdetI,&GotC,curPtr,PPMCI->Exclusion) )
  {
  LOE_GETINFO();

  O8P = (*EntropyFunc)(O8P,O8E,O8H);
  O5P = (*EntropyFunc)(O5P,O5E,O5H);
  O4P = (*EntropyFunc)(O4P,O4E,O4H);
  O3P = (*EntropyFunc)(O3P,O3E,O3H);
  O2P = (*EntropyFunc)(O2P,O2E,O2H);
  O1P = (*EntropyFunc)(O1P,O1E,O1H);
  O0P = (*EntropyFunc)(O0P,O0E,O0H);
  
  if ( O8P >= O5P && O8P >= O4P && O8P >= O3P && O8P >= O2P && O8P >= O1P && O8P >= O0P )
    {
    if ( ! PPMZ_DecodeC(PPMCI->O8_PPMI,&GotC,Context,Cntx2,PPMCI->Exclusion) )
      {
      if ( ! PPMZ_DecodeC(PPMCI->O5_PPMI,&GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
                PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
                }
              PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
              }
            PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
            }
          PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
          }
        PPMZ_DecodeGotC(PPMCI->O5_PPMI,GotC);
        }
      PPMZ_DecodeGotC(PPMCI->O8_PPMI,GotC);
      }
    }
  else if ( O5P >= O4P && O5P >= O3P && O5P >= O2P && O5P >= O1P && O5P >= O0P )
    {
      if ( ! PPMZ_DecodeC(PPMCI->O5_PPMI,&GotC,Context,Cntx2&0xFF,PPMCI->Exclusion) )
        {
        if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
                PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
                }
              PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
              }
            PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
            }
          PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
          }
        PPMZ_DecodeGotC(PPMCI->O5_PPMI,GotC);
        }
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O4P >= O3P && O4P >= O2P && O4P >= O1P && O4P >= O0P )
    {
        if ( ! PPMZ_DecodeC(PPMCI->O4_PPMI,&GotC,Context,0,PPMCI->Exclusion) )
          {
          if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
                PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
                }
              PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
              }
            PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
            }
          PPMZ_DecodeGotC(PPMCI->O4_PPMI,GotC);
          }
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O3P >= O2P && O3P >= O1P && O3P >= O0P )
    {
          if ( ! PPMZ_DecodeC(PPMCI->O3_PPMI,&GotC,Context&0xFFFFFF,0,PPMCI->Exclusion) )
            {
            if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
                PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
                }
              PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
              }
            PPMZ_DecodeGotC(PPMCI->O3_PPMI,GotC);
            }
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O2P >= O1P && O2P >= O0P )
    {
            if ( ! PPMZ_DecodeC(PPMCI->O2_PPMI,&GotC,Context&0xFFFF,0,PPMCI->Exclusion) )
              {
              if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
                PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
                }
              PPMZ_DecodeGotC(PPMCI->O2_PPMI,GotC);
              }
    PPMZ_UpdateC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0);
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else if ( O1P >= O0P )
    {
              if ( ! PPMZ_DecodeC(PPMCI->O1_PPMI,&GotC,Context&0xFF,0,PPMCI->Exclusion) )
                {
                Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
                PPMZ_DecodeGotC(PPMCI->O1_PPMI,GotC);
                }
    PPMZ_UpdateC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0);
    PPMZ_UpdateC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0);
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);
    }
  else
    {
    Order0_DecodeC(PPMCI->O0I,&GotC,PPMCI->Exclusion);
    PPMZ_UpdateC(PPMCI->O1_PPMI,GotC,Context&0xFF,0); 
    PPMZ_UpdateC(PPMCI->O2_PPMI,GotC,Context&0xFFFF,0);
    PPMZ_UpdateC(PPMCI->O3_PPMI,GotC,Context&0xFFFFFF,0);
    PPMZ_UpdateC(PPMCI->O4_PPMI,GotC,Context,0);	
    PPMZ_UpdateC(PPMCI->O5_PPMI,GotC,Context,Cntx2&0xFF);
    PPMZ_UpdateC(PPMCI->O8_PPMI,GotC,Context,Cntx2);	
    }

  PPMdet_DecodeGotC(PPMCI->PPMdetI,GotC);
  }

return(GotC);
}


/****** ********/

const int CoderNum_Max = 19;

PPM_InitFunc PPM_Inits[] =
  {
  PPM_Init0,
  PPM_Init1,
  PPM_Init2,
  PPM_Init3,
  PPM_Init4,
  PPM_Init5,
  PPM_Init6,
  PPM_Init7,
  PPM_Init8,

/* LOE coders: */
  PPM_Init0, /* A */
  PPM_Init0,
  PPM_Init0,
  PPM_Init0,
  PPM_Init0,
  PPM_Init0,
  PPM_Init0,
  PPM_Init0,
  PPM_Init0,
  PPM_Init0,
  PPM_Init0  /* K */
  };

PPM_WriteFunc PPM_Writers[] =
  {
  PPM_Write0,
  PPM_Write1,
  PPM_Write2,
  PPM_Write3,
  PPM_Write4,
  PPM_Write5,
  PPM_Write6,
  PPM_Write7,
  PPM_Write8,

  PPM_Write0_A,
  PPM_Write0_B,
  PPM_Write0_C,
  PPM_Write0_D,
  PPM_Write0_E,
  PPM_Write0_F,
  PPM_Write0_G,
  PPM_Write0_H,
  PPM_Write0_I,
  PPM_Write0_J,
  PPM_Write0_K
  };

PPM_ReadFunc PPM_Readers[] =
  {
  PPM_Read0,
  PPM_Read1,
  PPM_Read2,
  PPM_Read3,
  PPM_Read4,
  PPM_Read5,
  PPM_Read6,
  PPM_Read7,
  PPM_Read8,

  PPM_Read0_A,
  PPM_Read0_B,
  PPM_Read0_C,
  PPM_Read0_D,
  PPM_Read0_E,
  PPM_Read0_F,
  PPM_Read0_G,
  PPM_Read0_H,
  PPM_Read0_I,
  PPM_Read0_J,
  PPM_Read0_K
  };

int PPMcoder_OrderList[] = { 8,7,6,5,4,3,2,1,0,  88,88,88,88,88,88,88,88,88,88,88 };



void PPM_ShowFingerPrint(struct PPMcoderInfo * PPMCI)
{
if ( PPMCI->O8_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O8_PPMI));
if ( PPMCI->O7_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O7_PPMI));
if ( PPMCI->O6_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O6_PPMI));
if ( PPMCI->O5_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O5_PPMI));
if ( PPMCI->O4_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O4_PPMI));
if ( PPMCI->O3_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O3_PPMI));
if ( PPMCI->O2_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O2_PPMI));
if ( PPMCI->O1_PPMI ) printf("%08lX ",PPMZ_FingerPrint(PPMCI->O1_PPMI));
printf("\n");
}

