
//#define DEBUG

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/arithc.h>

#include "exclude.h"
#include "ppmz_cfg.h"

struct Order0Info
  {
  /* copied in from user */
  arithInfo * ari;
  long NumSymbols;

  /* my stuff */
  long countScale;
  long * CharCounts;
  long CharCountTot;
  long EscapeCount;
  };

/*externs:*/
extern void CleanUp(char * ExitMess);

/*protos:*/
void Order0_CleanUp(struct Order0Info * O0I);

/*functions:*/

struct Order0Info * Order0_Init (arithInfo * ari,long NumSymbols)
{
struct Order0Info * Ret;

if ( (Ret = AllocMem(sizeof(struct Order0Info),MEMF_ANY|MEMF_CLEAR)) == NULL )
  return(NULL);

Ret->NumSymbols = NumSymbols;
Ret->ari = ari;
Ret->EscapeCount = 1;
Ret->CharCountTot = 0;
Ret->countScale = min(O0_ScaleDown,ari->probMaxSafe);

if ( (Ret->CharCounts = AllocMem(NumSymbols*sizeof(long),MEMF_CLEAR)) == NULL )
  { free(Ret); return(NULL); }

return(Ret);
}

void Order0_EncodeC(struct Order0Info * O0I,long Symbol,exclusion * Exclusion)
{
long LowProb,TotProb,HighProb,i,NumSymbols;
long * CharCounts;
bool WroteChar;

NumSymbols = O0I->NumSymbols;
CharCounts = O0I->CharCounts;
TotProb    = O0I->EscapeCount;

for(i=0;i<Symbol;i++)
  {
  if ( ! isExcluded(Exclusion,i) && CharCounts[i] )
    { TotProb += CharCounts[i]; setExcluded(Exclusion,i); }
  }

if ( CharCounts[Symbol] )
  {
  LowProb = TotProb;
  TotProb = HighProb = LowProb + CharCounts[Symbol];
  WroteChar = 1;
  }
else
  {
  LowProb = 0;
  HighProb = O0I->EscapeCount;
  WroteChar = 0;
  }

for(i=Symbol+1;i<NumSymbols;i++)
  {
  if ( ! isExcluded(Exclusion,i) && CharCounts[i] ) { TotProb += CharCounts[i]; setExcluded(Exclusion,i); }
  }


#ifdef DEBUG
if ( ! (LowProb < HighProb && HighProb <= TotProb ) ) dbf();
#endif

arithEncode(O0I->ari,LowProb,HighProb,TotProb);

if ( CharCounts[Symbol] < 2 )
  {
  if ( CharCounts[Symbol] == 0 )
    {
    CharCounts[Symbol] = 1;
    O0I->EscapeCount ++;
    }
  else
    {
    CharCounts[Symbol] = 2;
    if ( O0I->EscapeCount > 1 ) O0I->EscapeCount --;
    }
  }
else
  {
  CharCounts[Symbol] ++;
  }

if ( ! WroteChar )
  {
  /* use order -1 */

  LowProb = 0;
  for(i=0;i<Symbol;i++)
    {
    LowProb += 1 - ( isExcluded(Exclusion,i) ? 1 : 0 );
    }
  TotProb = LowProb;
  for(;i<NumSymbols;i++)
    {
    TotProb += 1 - ( isExcluded(Exclusion,i) ? 1 : 0 );
    }

#ifdef DEBUG
if ( TotProb <= LowProb ) dbf();
#endif

  arithEncode(O0I->ari,LowProb,LowProb+1,TotProb);
  }


if ( (++O0I->CharCountTot) > O0I->countScale ) {
	O0I->CharCountTot = 0;
	for(i=0;i<NumSymbols;i++) { CharCounts[i] >>= 1; O0I->CharCountTot += CharCounts[i]; }
	O0I->EscapeCount >>= 1;
	if ( O0I->EscapeCount < 1 ) O0I->EscapeCount = 1;
  }

return;
}

void Order0_DecodeC(struct Order0Info * O0I,long * SymbolPtr,exclusion * Exclusion)
{
long TargetProb,LowProb,TotProb,HighProb,i;
long * CharCounts;
long Symbol,NumSymbols;

NumSymbols = O0I->NumSymbols;
CharCounts = O0I->CharCounts;
TotProb    = O0I->EscapeCount;

for(i=0;i<NumSymbols;i++) {
  if ( ! isExcluded(Exclusion,i) && CharCounts[i] )
    TotProb += CharCounts[i];
}

TargetProb = arithGet(O0I->ari,TotProb);

#ifdef DEBUG
if ( TargetProb >= TotProb ) dbf();
#endif

if ( TargetProb < O0I->EscapeCount )
  {
  arithDecode(O0I->ari,0,O0I->EscapeCount,TotProb);

  for(i=0;i<NumSymbols;i++) {
    if ( CharCounts[i] ) setExcluded(Exclusion,i);
  }

  /** decode with order -1 **/

  TotProb = 0;
  for(i=0;i<NumSymbols;i++) TotProb += 1 - ( isExcluded(Exclusion,i) ? 1 : 0 );

#ifdef DEBUG
if ( TotProb <= 0 ) dbf();
#endif

  TargetProb = arithGet(O0I->ari,TotProb);

  LowProb = 0;
  Symbol = 0x10000;
  for(i=0;i<NumSymbols;i++)
    {
    if ( !isExcluded(Exclusion,i) )
      {
      if ( TargetProb == 0 ) { Symbol = i; break; }
      TargetProb--;
      LowProb++;
      }
    }

#ifdef DEBUG
if ( Symbol == 0x10000 ) dbf();

 if ( O0I->CharCounts[Symbol] != 0 )
    CleanUp(" Escaped when CharCounts[Symbol] != 0 ");
#endif

  arithDecode(O0I->ari,LowProb,LowProb+1,TotProb);

  O0I->CharCounts[Symbol] = 1;
  O0I->EscapeCount ++;
  
  }
else
  {
  TargetProb -= O0I->EscapeCount;
  Symbol = NumSymbols;
  LowProb = O0I->EscapeCount;
  for(i=0;i<NumSymbols;i++)
    {
    if ( ! isExcluded(Exclusion,i) && CharCounts[i] )
      {
      TargetProb -= CharCounts[i];
      if ( TargetProb < 0 )
        { HighProb = LowProb + CharCounts[i]; Symbol = i; break; }
      LowProb += CharCounts[i];
      }
    }

#ifdef DEBUG
  if ( Symbol == NumSymbols )
    CleanUp("Error : Symbol not found in Order0!");
#endif

  arithDecode(O0I->ari,LowProb,HighProb,TotProb);

  if ( CharCounts[Symbol] < 2 )
    {
    if ( CharCounts[Symbol] == 0 )
      {
      CharCounts[Symbol] = 1;
      O0I->EscapeCount ++;
      }
    else
      {
      CharCounts[Symbol] = 2;
      if ( O0I->EscapeCount > 1 ) O0I->EscapeCount --;
      }
    }
  else
    {
    CharCounts[Symbol] ++;
    }

  }


if ( (++O0I->CharCountTot) > O0I->countScale ) {
	O0I->CharCountTot = 0;
	for(i=0;i<NumSymbols;i++) { CharCounts[i] >>= 1; O0I->CharCountTot += CharCounts[i]; }
	O0I->EscapeCount >>= 1;
	if ( O0I->EscapeCount < 1 ) O0I->EscapeCount = 1;
  }

*SymbolPtr = Symbol;

return;
}

void Order0_CleanUp(struct Order0Info * O0I)
{
FreeMem(O0I,sizeof(struct Order0Info));
}
