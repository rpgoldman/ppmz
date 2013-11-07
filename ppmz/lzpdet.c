
/***********

todo :

  vary LEN_SIZE & lenFlagMore

  use lastML & totLen in WriteML
    1. try delta coding with LastML ( have to flag {pos/neg/zero} )
          (could just add +15 or so instead of flagging)
    2. try LastML as part of 'index' context

------------

stats shows most of our compression comes from
contexts which are only used once.  this is not
surprising since this is a DET coder, and so each
context cannot properly build up stats.

***********/

#define STATS

#define MIN_LEN 30

// #define DO_MTF /* speed? */

#define LONG_DETMINLEN_INIT /* helps */

// #define COPY_GOTCONTEXT_INIT /* hurts */

// #define COPY_CNTXML_INIT /* doesn't seem to matter */

#define DO_DT /* critical */

// #define NO_ML /* helps !! */

/**

10-10-96 :

LONG_DETMINLEN_INIT     on
COPY_GOTCONTEXT_INIT    off
DO_DT                   on
NO_ML                   on
COPY_CNTXML_INIT        doesn't matter

trans  : 1.280
paper1 : 2.284

note : lzplit is using no LOE, so compare to PPMZ-C0

  trans  : 1.288
  paper1 : 2.286

conclusion : we're actually doing a little better than PPMdet , who's functionality
  we replaced.  (possibly because better indexing structure required no amortizing)

However, turning on ML is really hurting.  This should (ideally) NOT be the case.

---

there seems to be a STRONG dependence on lenFlagMore and LEN_SIZE

**/

/**************************************/

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/mempool.h>
#include <crblib/arithc.h>
#include <crblib/o1coder.h>

#ifdef STATS
#include <stdio.h>
#endif // STATS

#define lenFlagMore 16
#define lenTot (lenFlagMore+1)

/* config settings ; shared with PPMZ */

#include "ppmz_cfg.h"

#define DTSX_BITS     5
#define DCT_BITS      4

/* */

#define LEN_SIZE        (16)
#define LEN_HASH(index) ( (index) & (LEN_SIZE - 1) )

#define DSXHASH(x)    ( x & ((1<<DTSX_BITS)-1) )
#define DCTHASH(x)    ( min(x,((1<<DCT_BITS)-1)) )

#define DT_INC_T	20
#define DT_INC_E	18
#define DT_INC_ET	2

#define DT_SIZE       (1<<(DCT_BITS+DTSX_BITS))
#define DTHASH(c,x)   ( (DCTHASH(c)<<DTSX_BITS) + DSXHASH(x) )

#define DT_NUMCOUNTS  (1<<DCT_BITS)
#define NUM_O1_CHARS  (1<<DTSX_BITS)

#define PPMHASHBITS   16
#define PPMHASH_SIZE  (1<<PPMHASHBITS)
#define PPMHASH(x)    (((x>>16)^x) & (PPMHASH_SIZE-1))

#define CNTXHASH(c1,c2,c3) ( c1 ^ (c2<<17) ^ (c2>>15) ^ (c3<<7) ^ (c3>>25) )


/** context list operations macros **/

#define CONTEXT_ADDHEAD(zContext,zIndex)  {                 \
zContext->next = zIndex->contextList;                       \
zContext->prev = (LZPcontext *)zIndex;                      \
if ( zContext->next ) zContext->next->prev = zContext;      \
zContext->prev->next = zContext;    }         /***/

#define CONTEXT_CUT(zContext) { zContext->prev->next = zContext->next; \
if ( zContext->next ) zContext->next->prev = zContext->prev; }  /***/

#ifdef DO_MTF

#define CONTEXT_MTF(zContext,zIndex) if ( zContext == zIndex->contextList ) { } \
else { CONTEXT_CUT(zContext); CONTEXT_ADDHEAD(zContext,zIndex); }       /***/

#else

#define CONTEXT_MTF(zContext,zIndex) /***/

#endif /* DO_MTF */


/****/

typedef struct LZPindexStruct LZPindex;
typedef struct LZPcontextStruct LZPcontext;

struct LZPindexStruct
  {
  LZPcontext * contextList; /* same place as context->next */

  LZPindex * next;
  ulong cntxHash;
  };

struct LZPcontextStruct
  {
  LZPcontext *next;
  LZPcontext *prev;

  ulong cntx1,cntx2,cntx3;
  ubyte * string;
  uword detMinLen;
  uword count;
  ulong lastML,totLen;
  };

typedef struct LZPinfoStruct
  {
  long error; /* if it's not 0, you're dead */

  arithInfo * ari;

  MemPool * contextPool;
  MemPool * indexPool;

  uword * escCounts;
  uword * totCounts;
  oOne * lenCoder;

  LZPindex * indeces[PPMHASH_SIZE];

#ifdef STATS
  ulong numMatches;
  ulong numZeroMatches;
  ulong totMatchLen,longMatchLen,cntxLongMatchLen;
#endif // STATS

  } LZPinfo;

/* proto */ void LZPdet_CleanUp(LZPinfo * LZPDI);

/************ funcs *****************/

LZPinfo * LZPdet_Init(arithInfo * ari)
{
LZPinfo * LZPDI;
int i;

if ( (LZPDI = new(LZPinfo)) == NULL )
  return(NULL);

if ( (LZPDI->lenCoder = oOneCreate(ari,lenTot,LEN_SIZE)) == NULL )
  { LZPdet_CleanUp(LZPDI); return(NULL); }

if ( (LZPDI->contextPool = AllocPool(sizeof(LZPcontext),1024,1024)) == NULL )
  { LZPdet_CleanUp(LZPDI); return(NULL); }

if ( (LZPDI->indexPool = AllocPool(sizeof(LZPindex),1024,1024)) == NULL )
  { LZPdet_CleanUp(LZPDI); return(NULL); }

if ( (LZPDI->escCounts = malloc(sizeof(uword)*DT_SIZE)) == NULL )
  { LZPdet_CleanUp(LZPDI); return(NULL); }
if ( (LZPDI->totCounts = malloc(sizeof(uword)*DT_SIZE)) == NULL )
  { LZPdet_CleanUp(LZPDI); return(NULL); }
  
for(i=0;i<DT_SIZE;i++)
  {
  LZPDI->escCounts[i] = 1;
  if ( (i>>DTSX_BITS) == 0 ) LZPDI->totCounts[i] = 7;
  else LZPDI->totCounts[i] = 100;
  }

LZPDI->ari = ari;
LZPDI->error = 0;

return(LZPDI);
}

LZPcontext * LZPdet_Find(LZPinfo * LZPDI,ubyte * string,long * cntxMLptr)
{
LZPindex *curIndex,*prevIndex;
LZPcontext *curContext,*gotContext;
ulong cntx1,cntx2,cntx3,cntxHash,hash;
long cntxML;
#ifdef LONG_DETMINLEN_INIT
long longestML;
#endif

cntx1 = getulong(string-4);
cntx2 = getulong(string-8);
cntx3 = getulong(string-12);
cntxHash = CNTXHASH(cntx1,cntx2,cntx3);
hash = PPMHASH(cntxHash);

curIndex = LZPDI->indeces[hash];
while(curIndex)
  {
  if ( curIndex->cntxHash == cntxHash )
    break;

  curIndex = curIndex->next;
  }

prevIndex = NULL;
curIndex = LZPDI->indeces[hash];
while(curIndex)
  {
  if ( curIndex->cntxHash == cntxHash )
    break;

#ifdef DO_MTF
  prevIndex = curIndex;
#endif
  curIndex = curIndex->next;
  }

if ( curIndex == NULL )
  {
  if ( (curIndex = GetPoolHunk(LZPDI->indexPool,1)) == NULL )
    { LZPDI->error = 9; return(NULL); }
  curIndex->cntxHash = cntxHash;
  curIndex->contextList = NULL;
  curIndex->next = LZPDI->indeces[hash];
  LZPDI->indeces[hash] = curIndex;
  }
else if ( prevIndex != NULL ) /* MTF */
  {
#ifdef DO_MTF
  prevIndex->next = curIndex->next;

  curIndex->next = LZPDI->indeces[hash];
  LZPDI->indeces[hash] = curIndex;
#endif
  }


curContext = curIndex->contextList;
gotContext = NULL;
cntxML = 0;

#ifdef LONG_DETMINLEN_INIT
longestML = 0;
#endif

while(curContext)
  {
  if ( curContext->cntx1 == cntx1 &&
       curContext->cntx2 == cntx2 &&
       curContext->cntx3 == cntx3 )
    {
    ubyte *ap,*bp;
    long ml;

    ap = string - 13;
    bp = (curContext->string) - 13;
    ml = 12;
    while( *ap-- == *bp-- ) ml++;


#ifdef LONG_DETMINLEN_INIT
    if ( ml > longestML ) longestML = ml;
#endif

    if ( ml >= curContext->detMinLen )
      {
      cntxML = ml;
      gotContext = curContext;
      }

    }

  curContext = curContext->next;
  }

*cntxMLptr = cntxML;

if ( (curContext = GetPoolHunk(LZPDI->contextPool,1)) == NULL )
  { LZPDI->error = 9; return(0); }

curContext->string = string;
curContext->cntx1 = cntx1;
curContext->cntx2 = cntx2;
curContext->cntx3 = cntx3;
curContext->lastML = 0;
curContext->totLen = 0;
curContext->count = 0;
curContext->detMinLen = 0;

#ifdef LONG_DETMINLEN_INIT
curContext->detMinLen = longestML + 1;
#endif

#ifdef COPY_CNTXML_INIT
if ( gotContext ) curContext->detMinLen = cntxML + 1;
#endif

#ifdef COPY_GOTCONTEXT_INIT
if ( gotContext )
  {
  curContext->count = gotContext->count;
  curContext->detMinLen = cntxML + 1;
  curContext->lastML = gotContext->lastML;
  curContext->totLen = gotContext->totLen;
  }
#endif

CONTEXT_ADDHEAD(curContext,curIndex);

if ( gotContext )
  {
  CONTEXT_MTF(gotContext,curIndex);
  }

return(gotContext);
}

/*********** ML I/O func *********/

/* order1 code in LZPDI with index as order1 :

  use Det tables to write match/nomatch

  do order-1 length coding on index; index is roughly 9 bits , so it IS kosher to
    just use index as an order-1 context to the good-old o1coder routines.

*/

ulong LZPdet_ReadML(LZPinfo * LZPDI,ulong index,ulong lastML)
{
ulong len;

#ifdef DO_DT
  {
  ulong escP,totP,gotP;

  escP = LZPDI->escCounts[index];
  totP = LZPDI->totCounts[index];

  if ( totP >= LZPDI->ari->safeProbMax )
    {
    while ( totP >= LZPDI->ari->safeProbMax )
      { totP >>= 1; escP >>= 1; }
    if ( ! escP ) { escP++; totP++; }
    LZPDI->totCounts[index] = totP;
    LZPDI->escCounts[index] = escP;
    }

  gotP = arithGet(LZPDI->ari,totP);
  if ( gotP < escP )
    {
    arithDecode(LZPDI->ari,0,escP,totP);
    LZPDI->escCounts[index] += DT_INC_E;
    LZPDI->totCounts[index] += DT_INC_E + DT_INC_ET;
    return(0);
    }
  else
    {
    arithDecode(LZPDI->ari,escP,totP,totP);
    LZPDI->totCounts[index] += DT_INC_T;
    }
  }
#endif // DO_DT

#ifdef NO_ML
return(1);
#endif

len = oOneDecode(LZPDI->lenCoder,LEN_HASH(index));

if ( len == lenFlagMore )
  {
  ulong readLen;

  do
    {
    readLen = arithGet(LZPDI->ari,lenTot);
    arithDecode(LZPDI->ari,readLen,readLen+1,lenTot);
    len += readLen;
    } while( readLen == lenFlagMore );
  }

len += MIN_LEN;

return(len);
}

void LZPdet_WriteML(LZPinfo * LZPDI,ulong len,ulong index,ulong lastML)
{
long origLen;

#ifdef DO_DT
  {
  ulong escP,totP;

  escP = LZPDI->escCounts[index];
  totP = LZPDI->totCounts[index];

  if ( totP >= LZPDI->ari->safeProbMax )
    {
    while ( totP >= LZPDI->ari->safeProbMax )
      { totP >>= 1; escP >>= 1; }
    if ( ! escP ) { escP++; totP++; }
    LZPDI->totCounts[index] = totP;
    LZPDI->escCounts[index] = escP;
    }

  if ( len > 0 )
    {
    arithEncode(LZPDI->ari,escP,totP,totP);
    LZPDI->totCounts[index] += DT_INC_T;
    }
  else
    {
    arithEncode(LZPDI->ari,0,escP,totP);
    LZPDI->escCounts[index] += DT_INC_E;
    LZPDI->totCounts[index] += DT_INC_E + DT_INC_ET ;
    return;
    }
  }
#endif // DO_DT

#ifdef NO_ML
return;
#endif

len -= MIN_LEN;

origLen = len;
if ( len >= lenFlagMore ) len = lenFlagMore;

oOneEncode(LZPDI->lenCoder,len,LEN_HASH(index));

if ( origLen >= lenFlagMore )
  {
  len = origLen - lenFlagMore;
  
  while( len >= lenFlagMore )
    {
    arithEncode(LZPDI->ari,lenFlagMore,lenFlagMore+1,lenTot); 
    len -= lenFlagMore;
    }

  arithEncode(LZPDI->ari,len,len+1,lenTot);
  }

return;
}

/****************** ****************/

long LZPdet_Write(LZPinfo * LZPDI,ubyte *string,long * excludePtr)
{
LZPcontext * curContext;
long cntxML,matchLen;
ubyte *cntxStr,*vsStr;

curContext = LZPdet_Find(LZPDI,string,&cntxML);

if ( ! curContext ) { *excludePtr = -1; return(0); }

cntxStr = curContext->string; matchLen = 0;
vsStr = string;
*excludePtr = *cntxStr;
while(*vsStr++ == *cntxStr++) matchLen++;

if ( matchLen < MIN_LEN )
  matchLen = 0;

#ifdef NO_ML
if ( matchLen > 1 ) matchLen = 1;
#endif

LZPdet_WriteML(LZPDI,matchLen,DTHASH(curContext->count,string[-1]),curContext->lastML);

curContext->lastML = matchLen;
curContext->totLen += matchLen;

#ifdef STATS
if ( matchLen == 0 ) LZPDI->numZeroMatches++;
else LZPDI->numMatches++;
LZPDI->totMatchLen += matchLen;
LZPDI->longMatchLen = max(LZPDI->longMatchLen,matchLen);
LZPDI->cntxLongMatchLen = max(LZPDI->cntxLongMatchLen,curContext->totLen);
#endif // STATS

if ( matchLen > 0 ) {
  curContext->count++;
  }
else
  {
  curContext->detMinLen = cntxML + 1;
  }

return(matchLen);
}

long LZPdet_Read(LZPinfo * LZPDI,ubyte *string,long * excludePtr)
{
LZPcontext *curContext;
long cntxML, matchLen;

curContext = LZPdet_Find(LZPDI,string,&cntxML);

if ( ! curContext ) { *excludePtr = -1; return(0); }

*excludePtr = *(curContext->string);

matchLen = LZPdet_ReadML(LZPDI,DTHASH(curContext->count,string[-1]),curContext->lastML);

curContext->lastML = matchLen;
curContext->totLen += matchLen;

if ( matchLen > 0 )
  {
  ubyte *fromStr; long ml;

  fromStr = curContext->string;
  for(ml=matchLen;ml--;) *string++ = *fromStr++;

  curContext->count++;
  }
else
  {
  curContext->detMinLen = cntxML + 1;
  }

return(matchLen);
}

void LZPdet_CleanUp(LZPinfo * LZPDI)
{
if (! LZPDI ) return;

#ifdef STATS
if ( (LZPDI->numMatches + LZPDI->numZeroMatches) > 0 ) {
errputs("====== LZPdet Stats =======");
fprintf(stderr,"numMatches      : %ld\n",LZPDI->numMatches);
fprintf(stderr,"numZeroMatches  : %ld\n",LZPDI->numZeroMatches);
fprintf(stderr,"totMatchLen     : %ld\n",LZPDI->totMatchLen);
fprintf(stderr,"longMatchLen    : %ld\n",LZPDI->longMatchLen);
fprintf(stderr,"cntxLongMatchLen: %ld\n",LZPDI->cntxLongMatchLen);
errputs("=============");
}
#endif // STATS

smartfree(LZPDI->escCounts);
smartfree(LZPDI->totCounts);

if ( LZPDI->lenCoder ) oOneFree(LZPDI->lenCoder);

if ( LZPDI->contextPool ) FreePool(LZPDI->contextPool);
if ( LZPDI->indexPool ) FreePool(LZPDI->indexPool);

free(LZPDI);
}
