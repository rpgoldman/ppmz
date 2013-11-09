#include <stdio.h>
#include <math.h>

#define GETINFO_INFLATE_TOT // seems to help!!!
#define GETINFO_INFLATE_MPS_DET // seems to help!!!

// #define GETINFO_ENTROPY //uses doubles and goes very slow

/*************************************/

// #define UPDATE_ESCAPE /* seems to hurt a lot */

// #define DO_MTF /* helps speed?  a little */

/*******

we'd like all orders to share index structures, but that's a mess

----

LastC recency scaling doesn't seem to help at all.  Turning it on
has essentially no effect, though a reasonable number of characters
are LastC hits: about 1/8 of them.

  ( progc & bib might be showing improvement.
    supposedly obj2 benefits alot from this )

***************/

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/mempool.h>
#include <crblib/arithc.h>
#include <crblib/crc32.h>

#include "exclude.h"
#include "ppmzesc.h"

/* config settings */

/** moved to ppmz_cfg for faster testing **/

#include "ppmz_cfg.h"

#define CharInc_Novel 1
#define CharInc       1     /** 2 for PPMD , 1 for PPMC **/
#define EscpInc       1

#define PPMHASHBITS 14

/****/

#define PPMHASHMEMBERS (1<<PPMHASHBITS)
#define PPMHASH(x) ( ((x>>9)^x) & (PPMHASHMEMBERS-1))

/** context list operations macros **/

#define CONTEXT_ADDHEAD(CurContextInfo,CurIndex)  {                                   \
CurContextInfo->Next = CurIndex->ContextList;                                         \
CurContextInfo->Prev = (struct PPMZ_ContextInfo *)CurIndex;                           \
if ( CurContextInfo->Next ) CurContextInfo->Next->Prev = CurContextInfo;              \
CurContextInfo->Prev->Next = CurContextInfo;    }         /***/

#define CONTEXT_CUT(CurContextInfo)   {                                               \
CurContextInfo->Prev->Next = CurContextInfo->Next;                                    \
if ( CurContextInfo->Next ) CurContextInfo->Next->Prev = CurContextInfo->Prev;  } /***/

#ifdef DO_MTF

#define CONTEXT_MTF(CurContextInfo,CurIndex)                                          \
if ( CurContextInfo == CurIndex->ContextList ) { } else                               \
 { CONTEXT_CUT(CurContextInfo); CONTEXT_ADDHEAD(CurContextInfo,CurIndex); }       /***/

#else

#define CONTEXT_MTF(CurContextInfo,CurIndex)

#endif /* DO_MTF */

/****/

struct PPMZ_Node
  {
  struct PPMZ_Node * Next;
  uword Count;
  uword Char;
  };

struct PPMZ_ContextIndex
  {
  struct PPMZ_ContextIndex * Next;

  struct PPMZ_ContextInfo * ContextList;
  
  ulong Context;
  };

struct PPMZ_ContextInfo
  {
  struct PPMZ_Node * Nodes;     /* must be in same place as Node->Next */

  struct PPMZ_ContextInfo *Next; /* must be in same place as Index->List */
  struct PPMZ_ContextInfo *Prev;

  ulong HOContext;
  uword TotCount,EscapeCount;
  };

struct PPMZ_MemPools
  {
  MemPool * NodePool;
  MemPool * ContextPool;
  MemPool * IndexPool;
  };

struct PPMZInfo
  {
  long Error; /* if it's not 0, you're fucked */
  int order;

  /* copied in from users */
  long scaleDown;
  arithInfo * ari;

  /* state save between DecodeC and DecodeGotC */
  uword * PPMZ_WriteC_Ptr;

  struct PPMZ_MemPools * MyPools;
  MemPool * NodePool;
  MemPool * ContextPool;
  MemPool * IndexPool;

  escInfo * escapeInfo;

  struct PPMZ_ContextIndex * ContextIndeces[PPMHASHMEMBERS];
  };

/*** ****/

void PPMZ_CleanUp(struct PPMZInfo * PPMI);
void PPMZ_FreePools(struct PPMZ_MemPools * Pools);

/*** ****/

struct PPMZ_MemPools * PPMZ_AllocPools(void)
{
struct PPMZ_MemPools * Ret;

if ( (Ret = AllocMem(sizeof(struct PPMZ_MemPools),MEMF_ANY|MEMF_CLEAR)) == NULL )
  return(NULL);

if ( (Ret->NodePool = AllocPool(sizeof(struct PPMZ_Node),1024,1024)) == NULL )
  { PPMZ_FreePools(Ret); return(NULL); }

if ( (Ret->ContextPool = AllocPool(sizeof(struct PPMZ_ContextInfo),1024,1024)) == NULL )
  { PPMZ_FreePools(Ret); return(NULL); }

if ( (Ret->IndexPool = AllocPool(sizeof(struct PPMZ_ContextIndex),1024,1024)) == NULL )
  { PPMZ_FreePools(Ret); return(NULL); }

return(Ret);
}

void PPMZ_FreePools(struct PPMZ_MemPools * Pools)
{
if (!Pools) return;

if ( Pools->NodePool )    FreePool(Pools->NodePool);
if ( Pools->ContextPool ) FreePool(Pools->ContextPool);
if ( Pools->IndexPool )   FreePool(Pools->IndexPool);

}

struct PPMZInfo * PPMZ_Init(arithInfo * ari,int Flags,int scaleDown,
  struct PPMZ_MemPools * Pools,int order,escInfo *ei)
{
struct PPMZInfo * PPMI;

if ( (PPMI = AllocMem(sizeof(struct PPMZInfo),MEMF_ANY|MEMF_CLEAR)) == NULL )
  return(NULL);

PPMI->order = order;

if ( Pools )
  {
  PPMI->MyPools = NULL;
  }
else
  {
  if ( (Pools = PPMZ_AllocPools()) == NULL )
    { free(PPMI); return(NULL); }
  PPMI->MyPools = Pools;
  }

PPMI->NodePool    = Pools->NodePool;
PPMI->ContextPool = Pools->ContextPool;
PPMI->IndexPool   = Pools->IndexPool;

PPMI->ari = ari;
PPMI->Error = 0;
PPMI->scaleDown = min(scaleDown,ari->safeProbMax);

PPMI->escapeInfo = ei;

return(PPMI);
}


void PPMZ_Scale(struct PPMZInfo * PPMI,struct PPMZ_ContextInfo * CurContextInfo)
{
struct PPMZ_Node *CurNode,*PrevNode;
long TotProb;

CurContextInfo->EscapeCount >>= 1;
CurContextInfo->EscapeCount += EscpInc;
TotProb = CurContextInfo->EscapeCount;
CurNode = CurContextInfo->Nodes;
PrevNode = (struct PPMZ_Node *) CurContextInfo;
while(CurNode)
  {
  if ( CurNode->Count <= CharInc )
    {
    PrevNode->Next = CurNode->Next;

    if ( ! FreePoolHunk(PPMI->NodePool,CurNode) )
      PPMI->Error = 9;

    CurNode = PrevNode->Next;
    }
  else
    {
    CurNode->Count >>= 1;
    TotProb += CurNode->Count;

    PrevNode = CurNode;
    CurNode = CurNode->Next;
    }
  }

CurContextInfo->TotCount = TotProb;

}

/*
 * return-value indicates whether char was written with
 *  current order or not.  If not, you MUST write it with
 *  some other model.
 *
 */
bool PPMZ_EncodeC(struct PPMZInfo * PPMI,long Symbol,ulong Context,ulong HOContext,exclusion * Exclusion)
{
struct PPMZ_ContextIndex *CurIndex;
#ifdef DO_MTF
struct PPMZ_ContextIndex *PrevIndex;
#endif
struct PPMZ_ContextInfo *CurContextInfo;
struct PPMZ_Node *CurNode;
long Hash;

Hash = PPMHASH(Context);

/** find a context index **/

#ifdef DO_MTF
PrevIndex = NULL;
#endif

CurIndex = PPMI->ContextIndeces[Hash];
while(CurIndex)
  {
  if ( CurIndex->Context == Context )
    {
#ifdef DO_MTF
    if ( PrevIndex ) {
      /* MTF */
      PrevIndex->Next = CurIndex->Next;
      CurIndex->Next = PPMI->ContextIndeces[Hash];
      PPMI->ContextIndeces[Hash] = CurIndex;
      }
#endif
    goto PPMZ_Encode_FoundCurIndex;
    }
#ifdef DO_MTF
  PrevIndex = CurIndex;
#endif
  CurIndex = CurIndex->Next;
  }

/* index not found */
if ( (CurIndex = GetPoolHunk(PPMI->IndexPool,1)) == NULL )
  { PPMI->Error = 9; return(0); }
CurIndex->Context = Context;
CurIndex->Next = PPMI->ContextIndeces[Hash];
PPMI->ContextIndeces[Hash] = CurIndex;
CurIndex->ContextList = NULL;

/***/
  PPMZ_Encode_FoundCurIndex:
/***/

CurContextInfo = CurIndex->ContextList;
while(CurContextInfo)
  {
  if ( CurContextInfo->HOContext == HOContext )
    {
    long TotProb,NumCs;

    /*** got context ***/

#ifdef DEBUG
if ( CurContextInfo->EscapeCount < 1 || (CurContextInfo->TotCount - CurContextInfo->EscapeCount) < 1 )
	errputs("\n\nbad 1");
#endif

    CONTEXT_MTF(CurContextInfo,CurIndex);

    CurNode = CurContextInfo->Nodes;

    if ( CurNode && CurNode->Next == NULL ) {  /* deterministic context */

		if ( CurNode->Char == Symbol ) {
			writePPMZesc(nope,Context,1,CurNode->Count+1,1,PPMI->escapeInfo,PPMI->order);
			CurNode->Count            += CharInc;
			CurContextInfo->TotCount  += CharInc;

	        if ( CurContextInfo->TotCount >= PPMI->scaleDown )
	          PPMZ_Scale(PPMI,CurContextInfo);

			return(1);
		} else {

	        if ( ! isExcluded(Exclusion,CurNode->Char) ) {
				writePPMZesc(yep,Context,1,CurNode->Count+1,1,PPMI->escapeInfo,PPMI->order);
				setExcluded(Exclusion,CurNode->Char);
			}
        
	        CurContextInfo->EscapeCount += EscpInc;
	        CurContextInfo->TotCount    += EscpInc;

	        if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
	          { PPMI->Error = 9; return(0); }
    
	        CurContextInfo->TotCount += CharInc_Novel;
    
	        CurNode->Char = Symbol;
	        CurNode->Count = CharInc_Novel;
	        CurNode->Next = CurContextInfo->Nodes;
	        CurContextInfo->Nodes = CurNode;

	        return(0);
        }
	} /* deterministic special case, end */
    
    NumCs=0;
    TotProb = CurContextInfo->EscapeCount;
    while(CurNode)
      {
      if ( CurNode->Char == Symbol )
        {
        struct PPMZ_Node *GotNode;
        long LowProb,HighProb;
  
        NumCs++;
        LowProb = TotProb;
        TotProb = HighProb = TotProb + CurNode->Count;
        GotNode = CurNode;

        CurNode = CurNode->Next;
        while(CurNode)
          {
          if ( !isExcluded(Exclusion,CurNode->Char) )
            { NumCs++; TotProb += CurNode->Count; }
          CurNode = CurNode->Next;
          }
        CurNode = GotNode;
  
		writePPMZesc(nope,Context,
			CurContextInfo->EscapeCount,TotProb,NumCs,PPMI->escapeInfo,PPMI->order);

        LowProb -= CurContextInfo->EscapeCount;
        HighProb -= CurContextInfo->EscapeCount;
        TotProb -= CurContextInfo->EscapeCount;

        arithEncode(PPMI->ari,LowProb,HighProb,TotProb);

        if ( CurNode->Count == CharInc_Novel && CurContextInfo->EscapeCount > EscpInc )
          {
          CurContextInfo->EscapeCount -= EscpInc;
          CurContextInfo->TotCount -= EscpInc;
          }
  
        CurNode->Count += CharInc;
        CurContextInfo->TotCount += CharInc;

        if ( CurContextInfo->TotCount >= PPMI->scaleDown )
          PPMZ_Scale(PPMI,CurContextInfo);
  
        return(1);
        }
      if ( !isExcluded(Exclusion,CurNode->Char) )
        { NumCs++; TotProb += CurNode->Count; }
      CurNode = CurNode->Next;
      }

    /** char not found in list **/

    CurNode = CurContextInfo->Nodes;
    while(CurNode)
      {
      setExcluded(Exclusion,CurNode->Char);
      CurNode = CurNode->Next;
      }

	writePPMZesc(yep,Context,
		CurContextInfo->EscapeCount,TotProb,NumCs,PPMI->escapeInfo,PPMI->order);
  
    CurContextInfo->EscapeCount += EscpInc;
    CurContextInfo->TotCount += EscpInc;
  
    if ( CurContextInfo->EscapeCount >= PPMZ_escapeScaleDown )
      PPMZ_Scale(PPMI,CurContextInfo);
  
    if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
      { PPMI->Error = 9; return(0); }
  
    CurContextInfo->TotCount += CharInc_Novel;
  
    CurNode->Char = Symbol;
    CurNode->Count = CharInc_Novel;
    CurNode->Next = CurContextInfo->Nodes;
    CurContextInfo->Nodes = CurNode;
  
    return(0);
    }
  CurContextInfo = CurContextInfo->Next;
  }

/** context not found **/

if ( (CurContextInfo = GetPoolHunk(PPMI->ContextPool,1)) == NULL )
  { PPMI->Error = 9; return(0); }
if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
  { PPMI->Error = 9; return(0); }

CurNode->Char = Symbol;
CurNode->Count = CharInc_Novel;
CurNode->Next = NULL;

CurContextInfo->HOContext = HOContext;
CurContextInfo->TotCount = CharInc_Novel + EscpInc;
CurContextInfo->EscapeCount = EscpInc;
CurContextInfo->Nodes = CurNode;
CurContextInfo->Next = CurContextInfo->Prev = NULL;

CONTEXT_ADDHEAD(CurContextInfo,CurIndex);

return(0);
}


/*
 * return-value indicates whether an escape was read or not
 *  If so, you MUST read it with some other model.
 *    and then call DecodeGotC
 *  If not, don't call DecodeGotC
 *
 */
bool PPMZ_DecodeC(struct PPMZInfo * PPMI,long * SymbolPtr,ulong Context,ulong HOContext,exclusion * Exclusion)
{
struct PPMZ_ContextIndex *CurIndex;
#ifdef DO_MTF
struct PPMZ_ContextIndex *PrevIndex;
#endif
struct PPMZ_ContextInfo *CurContextInfo;
struct PPMZ_Node *CurNode;
long Hash;

//printf("O%d decode\n",PPMI->order);

Hash = PPMHASH(Context);

CurIndex = PPMI->ContextIndeces[Hash];
#ifdef DO_MTF
PrevIndex = NULL;
#endif
while(CurIndex)
  {
  if ( CurIndex->Context == Context )
    {
#ifdef DO_MTF
    if ( PrevIndex )
      {
      /* MTF */
      PrevIndex->Next = CurIndex->Next;
      CurIndex->Next = PPMI->ContextIndeces[Hash];
      PPMI->ContextIndeces[Hash] = CurIndex;
      }
#endif
    goto PPMZ_Decode_FoundCurIndex;
    }
#ifdef DO_MTF
  PrevIndex = CurIndex;
#endif
  CurIndex = CurIndex->Next;
  }

/* index not found */
if ( (CurIndex = GetPoolHunk(PPMI->IndexPool,1)) == NULL )
  { PPMI->Error = 9; return(0); }
CurIndex->Context = Context;
CurIndex->Next = PPMI->ContextIndeces[Hash];
PPMI->ContextIndeces[Hash] = CurIndex;
CurIndex->ContextList = NULL;

/***/
  PPMZ_Decode_FoundCurIndex:
/***/

CurContextInfo = CurIndex->ContextList;
while(CurContextInfo)
  {
  if ( CurContextInfo->HOContext == HOContext )
    {
    long TargetProb;
    long LowProb,HighProb;
    long TotProb,NumCs;

    /* got context */

    CONTEXT_MTF(CurContextInfo,CurIndex);

    CurNode = CurContextInfo->Nodes;

    if ( CurNode && CurNode->Next == NULL ) { /* deterministic context */
      
      if ( isExcluded(Exclusion,CurNode->Char) )
        {
        CurContextInfo->EscapeCount += EscpInc;
        CurContextInfo->TotCount += EscpInc;

        if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
          { PPMI->Error = 9; return(0); }
    
        CurContextInfo->TotCount += CharInc_Novel;
  
        PPMI->PPMZ_WriteC_Ptr = & ( CurNode->Char );  
  
        CurNode->Count = CharInc_Novel;
        CurNode->Next = CurContextInfo->Nodes;
        CurContextInfo->Nodes = CurNode;

        return(0);
        }

      if ( readPPMZesc(Context,1,CurNode->Count + 1,1,PPMI->escapeInfo,PPMI->order) ) {
        
        setExcluded(Exclusion,CurNode->Char);
        
        CurContextInfo->EscapeCount += EscpInc;
        CurContextInfo->TotCount += EscpInc;

        if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
          { PPMI->Error = 9; return(0); }
    
        CurContextInfo->TotCount += CharInc_Novel;
  
        PPMI->PPMZ_WriteC_Ptr = & ( CurNode->Char );  
  
        CurNode->Count = CharInc_Novel;
        CurNode->Next = CurContextInfo->Nodes;
        CurContextInfo->Nodes = CurNode;

        return(false);
	  } else {
        CurNode->Count += CharInc;
        CurContextInfo->TotCount += CharInc;

        *SymbolPtr = CurNode->Char;

        if ( CurContextInfo->TotCount >= PPMI->scaleDown )
          PPMZ_Scale(PPMI,CurContextInfo);

        return(true);
        }
      }

    TotProb = CurContextInfo->EscapeCount;
    NumCs = 0;
    while(CurNode)
      {
      if ( !isExcluded(Exclusion,CurNode->Char) ) { NumCs++; TotProb += CurNode->Count; }
      CurNode = CurNode->Next;
      }

	if ( readPPMZesc(Context,
			CurContextInfo->EscapeCount,TotProb,NumCs,PPMI->escapeInfo,PPMI->order) )
      {
      CurNode = CurContextInfo->Nodes;
      while(CurNode)
        {
        if ( !isExcluded(Exclusion,CurNode->Char) )
          setExcluded(Exclusion,CurNode->Char);
        CurNode = CurNode->Next;
        }

      CurContextInfo->EscapeCount += EscpInc;
      CurContextInfo->TotCount += EscpInc;
    
      if ( CurContextInfo->EscapeCount >= PPMZ_escapeScaleDown )
        PPMZ_Scale(PPMI,CurContextInfo);

      if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
        { PPMI->Error = 9; return(0); }
    
      CurContextInfo->TotCount += CharInc_Novel;
    
      PPMI->PPMZ_WriteC_Ptr = & ( CurNode->Char );  
  
      CurNode->Count = CharInc_Novel;
      CurNode->Next = CurContextInfo->Nodes;
      CurContextInfo->Nodes = CurNode;
    
      return(0);
      }
    else
      {
      TotProb -= CurContextInfo->EscapeCount;

      TargetProb = arithGet(PPMI->ari,TotProb);

      LowProb = 0;

      CurNode = CurContextInfo->Nodes;
      while(CurNode)
        {
        if ( !isExcluded(Exclusion,CurNode->Char) )
          {
          TargetProb -= CurNode->Count;
          if ( TargetProb < 0 )
            {
            HighProb = LowProb + CurNode->Count;
            arithDecode(PPMI->ari,LowProb,HighProb,TotProb);

            if ( CurNode->Count == CharInc_Novel && CurContextInfo->EscapeCount > EscpInc )
              {
              CurContextInfo->EscapeCount-= EscpInc; CurContextInfo->TotCount-= EscpInc;
              }
    
            CurNode->Count += CharInc;
            CurContextInfo->TotCount += CharInc;

            *SymbolPtr = CurNode->Char;

	        if ( CurContextInfo->TotCount >= PPMI->scaleDown )
	          PPMZ_Scale(PPMI,CurContextInfo);

            return(1);
            }
          LowProb += CurNode->Count;
          }
        CurNode = CurNode->Next;
        }

      *SymbolPtr = 0x10000;
      PPMI->Error = 3;
      return(1);
      }
    }
  CurContextInfo = CurContextInfo->Next;
  }

if ( (CurContextInfo = GetPoolHunk(PPMI->ContextPool,1)) == NULL )
  { PPMI->Error = 9; return(0); }
if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
  { PPMI->Error = 9; return(0); }

PPMI->PPMZ_WriteC_Ptr = & ( CurNode->Char );  
CurNode->Count = CharInc_Novel;
CurNode->Next = NULL;

CurContextInfo->HOContext = HOContext;
CurContextInfo->TotCount = CharInc_Novel + EscpInc;
CurContextInfo->EscapeCount = EscpInc;
CurContextInfo->Nodes = CurNode;

CurContextInfo->Next=CurContextInfo->Prev=NULL;

CONTEXT_ADDHEAD(CurContextInfo,CurIndex);

return(0);
}

void PPMZ_DecodeGotC(struct PPMZInfo * PPMI,uword GotC)
{

if ( PPMI->PPMZ_WriteC_Ptr == NULL )
  {
  PPMI->Error = 4;
  return;
  }

*(PPMI->PPMZ_WriteC_Ptr) = GotC;

PPMI->PPMZ_WriteC_Ptr = NULL;

}

void PPMZ_CleanUp(struct PPMZInfo * PPMI)
{
if (! PPMI ) return;

if ( PPMI->MyPools )
	PPMZ_FreePools(PPMI->MyPools);

free(PPMI);
}


bool PPMZ_UpdateC(struct PPMZInfo * PPMI,long Symbol,ulong Context,ulong HOContext)
{
struct PPMZ_ContextIndex *CurIndex;
#ifdef DO_MTF
struct PPMZ_ContextIndex *PrevIndex;
#endif
struct PPMZ_ContextInfo *CurContextInfo;
struct PPMZ_Node *CurNode;
long Hash;

//printf("O%d update\n",PPMI->order);

Hash = PPMHASH(Context);

/** find a context index **/

#ifdef DO_MTF
PrevIndex = NULL;
#endif
CurIndex = PPMI->ContextIndeces[Hash];
while(CurIndex)
  {
  if ( CurIndex->Context == Context )
    {
#ifdef DO_MTF
    if ( PrevIndex )
      {
      /* MTF */
      PrevIndex->Next = CurIndex->Next;
      CurIndex->Next = PPMI->ContextIndeces[Hash];
      PPMI->ContextIndeces[Hash] = CurIndex;
      }
#endif
    goto PPMZ_Encode_FoundCurIndex;
    }
#ifdef DO_MTF
  PrevIndex = CurIndex;
#endif
  CurIndex = CurIndex->Next;
  }

/* index not found */
if ( (CurIndex = GetPoolHunk(PPMI->IndexPool,1)) == NULL )
  { PPMI->Error = 9; return(0); }
CurIndex->Context = Context;
CurIndex->Next = PPMI->ContextIndeces[Hash];
PPMI->ContextIndeces[Hash] = CurIndex;
CurIndex->ContextList = NULL;

/***/
  PPMZ_Encode_FoundCurIndex:
/***/

CurContextInfo = CurIndex->ContextList;
while(CurContextInfo)
  {
  if ( CurContextInfo->HOContext == HOContext )
    {
    long TotProb,NumCs;
 
    /*** got context ***/

    CONTEXT_MTF(CurContextInfo,CurIndex);

    CurNode = CurContextInfo->Nodes;

    if ( CurNode && CurNode->Next == NULL ) {

      if ( CurNode->Char == Symbol ) {

#ifdef UPDATE_ESCAPE
		PPMZesc_updateP(nope,Context,1,CurContextInfo->TotCount,1,PPMI->escapeInfo,PPMI->order);
#endif

        CurNode->Count            += CharInc;
        CurContextInfo->TotCount  += CharInc;

        if ( CurContextInfo->TotCount >= PPMI->scaleDown )
          PPMZ_Scale(PPMI,CurContextInfo);

        return(1);
      } else {
#ifdef UPDATE_ESCAPE
		PPMZesc_updateP(yep,Context,1,CurContextInfo->TotCount,1,PPMI->escapeInfo,PPMI->order);
#endif
        CurContextInfo->EscapeCount += EscpInc;
        CurContextInfo->TotCount    += EscpInc;

        if ( CurContextInfo->TotCount >= PPMI->scaleDown )
          PPMZ_Scale(PPMI,CurContextInfo);

        if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
          { PPMI->Error = 9; return(0); }
    
        CurContextInfo->TotCount += CharInc_Novel;
    
        CurNode->Char = Symbol;
        CurNode->Count = CharInc_Novel;
        CurNode->Next = CurContextInfo->Nodes;
        CurContextInfo->Nodes = CurNode;

        return(0);
        }
    }
    
    NumCs=0;
    TotProb = CurContextInfo->EscapeCount;
    while(CurNode)
      {
      if ( CurNode->Char == Symbol )
        {
        struct PPMZ_Node *GotNode;
        long LowProb,HighProb;
  
        NumCs++;  
        LowProb = TotProb;
        TotProb = HighProb = TotProb + CurNode->Count;
        GotNode = CurNode;

        CurNode = CurNode->Next;
        while(CurNode)
          {
          NumCs++; TotProb += CurNode->Count;
          CurNode = CurNode->Next;
          }
        CurNode = GotNode;
  
#ifdef UPDATE_ESCAPE
		PPMZesc_updateP(nope,Context,CurContextInfo->EscapeCount,TotProb,NumCs,PPMI->escapeInfo,PPMI->order);
#endif

        LowProb -= CurContextInfo->EscapeCount;
        HighProb -= CurContextInfo->EscapeCount;
        TotProb -= CurContextInfo->EscapeCount;

        if ( CurNode->Count == CharInc_Novel && CurContextInfo->EscapeCount > EscpInc )
          {
          CurContextInfo->EscapeCount -= EscpInc;
          CurContextInfo->TotCount -= EscpInc;
          }
  
        CurNode->Count += CharInc;
        CurContextInfo->TotCount += CharInc;

	        if ( CurContextInfo->TotCount >= PPMI->scaleDown )
	          PPMZ_Scale(PPMI,CurContextInfo);

        return(1);
        }
      NumCs++;
      TotProb += CurNode->Count;
      CurNode = CurNode->Next;
      }

    /** char not found in list **/

#ifdef UPDATE_ESCAPE
	PPMZesc_updateP(yep,Context,CurContextInfo->EscapeCount,TotProb,NumCs,PPMI->escapeInfo,PPMI->order);
#endif

    CurContextInfo->EscapeCount += EscpInc;
    CurContextInfo->TotCount += EscpInc;
  
    if ( CurContextInfo->EscapeCount >= PPMZ_escapeUpdateScaleDown )
      PPMZ_Scale(PPMI,CurContextInfo);
  
    if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
      { PPMI->Error = 9; return(0); }
  
    CurContextInfo->TotCount += CharInc_Novel;
  
    CurNode->Char = Symbol;
    CurNode->Count = CharInc_Novel;
    CurNode->Next = CurContextInfo->Nodes;
    CurContextInfo->Nodes = CurNode;
  
    return(0);
    }
  CurContextInfo = CurContextInfo->Next;
  }

/** context not found **/

if ( (CurContextInfo = GetPoolHunk(PPMI->ContextPool,1)) == NULL )
  { PPMI->Error = 9; return(0); }
if ( (CurNode = GetPoolHunk(PPMI->NodePool,1)) == NULL )
  { PPMI->Error = 9; return(0); }

CurNode->Char = Symbol;
CurNode->Count = CharInc_Novel;
CurNode->Next = NULL;

CurContextInfo->HOContext = HOContext;
CurContextInfo->TotCount = CharInc_Novel + EscpInc;
CurContextInfo->EscapeCount = EscpInc;
CurContextInfo->Nodes = CurNode;
CurContextInfo->Next = CurContextInfo->Prev = NULL;

CONTEXT_ADDHEAD(CurContextInfo,CurIndex);

return(0);
}

#define ESC_SCALE (1<<12)

ulong PPMZ_TellEscP(struct PPMZInfo * PPMI,long EscC,long TotC,long NumC,ulong Context)
{
int EscP,TotP;

PPMZesc_getP(&EscP,&TotP,Context,EscC,TotC,NumC,PPMI->escapeInfo,PPMI->order);

return( (ESC_SCALE*EscP)/TotP );
}

void PPMZ_GetInfo(struct PPMZInfo * PPMI,ulong Context,ulong HOContext,
  ulong * MPS_P_ptr,ulong * Esc_P_ptr,ulong *entropy_ptr)
{
struct PPMZ_ContextIndex *CurIndex;
struct PPMZ_ContextInfo *CurContextInfo;
struct PPMZ_Node *CurNode;
long Hash;
ulong MPS_P,Esc_P;

*MPS_P_ptr = ESC_SCALE/128;
*Esc_P_ptr = ESC_SCALE/2;
*entropy_ptr = ESC_SCALE * 5; //needs to be less than the o0

Hash = PPMHASH(Context);

/** find a context index **/

CurIndex = PPMI->ContextIndeces[Hash];
if ( !CurIndex) return;
while( CurIndex->Context != Context )
  {
  CurIndex = CurIndex->Next;
  if ( !CurIndex) return;
  }

CurContextInfo = CurIndex->ContextList;
if ( !CurContextInfo ) return;
while(CurContextInfo->HOContext != HOContext) {
  CurContextInfo = CurContextInfo->Next;
  if ( !CurContextInfo ) return;
  }

CurNode = CurContextInfo->Nodes;

if ( CurNode && CurNode->Next == NULL ) {
	Esc_P = PPMZ_TellEscP(PPMI,1,1 + CurNode->Count,1,Context);
#ifdef GETINFO_INFLATE_MPS_DET
	MPS_P = ESC_SCALE;
#else
	MPS_P = ESC_SCALE - Esc_P;
#endif
#ifdef GETINFO_ENTROPY
	*entropy_ptr = (ulong)( Esc_P * log2((double)ESC_SCALE/(double)Esc_P)
				+ MPS_P * log2((double)ESC_SCALE/(double)Esc_P) );
#endif

} else {
  struct PPMZ_Node * MPS_Node;
  int NumCs=0;
#ifdef GETINFO_ENTROPY
  int tot_char_count;
  double p,entropy; // temp test
#endif

	MPS_Node = CurNode;

#ifdef GETINFO_ENTROPY
	tot_char_count = CurContextInfo->TotCount - CurContextInfo->EscapeCount;
	entropy = 0;
#endif

	while(CurNode) {
		if ( CurNode->Count > MPS_Node->Count ) MPS_Node = CurNode;
		NumCs++;

#ifdef GETINFO_ENTROPY
		p = ((double)CurNode->Count/(double)tot_char_count);
		entropy +=  p * log2(p); 
#endif

		CurNode = CurNode->Next;
	}

#ifdef GETINFO_INFLATE_TOT
  Esc_P = PPMZ_TellEscP(PPMI,
		CurContextInfo->EscapeCount,
		CurContextInfo->TotCount+CurContextInfo->EscapeCount, //but totcount already include escapecount!
		NumCs,Context);
#else
  Esc_P = PPMZ_TellEscP(PPMI,
		CurContextInfo->EscapeCount,CurContextInfo->TotCount,NumCs,Context);
#endif

  MPS_P = MPS_Node->Count*ESC_SCALE/CurContextInfo->TotCount;

#ifdef GETINFO_ENTROPY
	p = (double)Esc_P/(double)ESC_SCALE;
	entropy = p * log2(p) + (1-p)*(log2(1-p) + entropy);
	entropy = - ESC_SCALE * entropy;
	*entropy_ptr = (ulong)entropy;
#endif

  }

*MPS_P_ptr = MPS_P * ( ESC_SCALE - Esc_P ) / ESC_SCALE;
*Esc_P_ptr = Esc_P;

return;
}


/*********

  FingerPrint is a useful debugging tool.  It makes 32-bit CRC of the PPMZ data structure.
  After N bytes, the encoder and decoder should have the same fingerprint.

  Note that FingerPrint is quite slow because it had to walk all over in the data structure.

***********/

ulong PPMZ_FingerPrint(struct PPMZInfo * PPMI)
{
struct PPMZ_ContextIndex *CurIndex;
struct PPMZ_ContextInfo *CurInfo;
struct PPMZ_Node *CurNode;
long Hash;
ulong C;
ulong NumNodes=0,NumInfos=0,NumIndeces=0;

C = PPMI->NodePool->NumItemsActive; /* seed */

CRC_START(C);

for(Hash=0;Hash<PPMHASHMEMBERS;Hash++)
  {
  CurIndex = PPMI->ContextIndeces[Hash];
  while(CurIndex)
    {

    C = addcrcl(C,CurIndex->Context);

    CurInfo = CurIndex->ContextList;
    while(CurInfo)
      {

      C = addcrcl(C,CurInfo->HOContext);
      C = addcrcw(C,CurInfo->TotCount);
      C = addcrcw(C,CurInfo->EscapeCount);

      CurNode = CurInfo->Nodes;
      while(CurNode)
        {

        C = addcrcw(C,CurNode->Char);
        C = addcrcw(C,CurNode->Count);

        NumNodes++;
        CurNode = CurNode->Next;
        }
      NumInfos++;
      CurInfo = CurInfo->Next;
      }
    NumIndeces++;
    CurIndex = CurIndex->Next;
    }
  }

/* interesting debug info in Num#? */

C = addcrcl(C,NumIndeces);
C = addcrcl(C,NumInfos);
C = addcrcl(C,NumNodes);

CRC_FINISH(C);

return(C);
}

