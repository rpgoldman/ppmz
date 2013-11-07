
/***

hmm. we need DO_EXCLUDES on to get good compression, but
we cannot code DO_EXCLUDES for MIN_LEN > 1 , since the
char predicted by LZP may be the right one.

------

if we set min_len very high, then coding all the escapes for
no-matches becomes worth-while, but then we create the separate
problem that then we cut down the total number of matches to
only a tiny number (100 matches or so) , (though they comprise
a great number of bytes (10000 or so) which means that the
coders for writing the lengths do not gather enough statistics
to make good coders.

****/

//#define USE_LENCODER

#define MIN_LEN 64

//#define ALWAYS_LEN1

#define STATS

//#define DEBUG

/***********

8-8-97 : well LZP in PPMZ works, but performance is not great,
        and I get bugs whenever I try to fix things up.

----------------

todo :

  vary LEN_SIZE & lenFlagMore

  use lastML & totLen in WriteML
    1. try delta coding with LastML ( have to flag {pos/neg/zero} )
          (could just add +15 or so instead of flagging)
    2. try LastML as part of 'index' context

todo :
  DEMAND_GOOD option to not code unless
    (esc * GOOD_RATIO) < tot
  (where GOOD_RATIO = 3 or so)
  why hard :
    must update esc & tot even when not coding
    must put in special call in lzparray to fix decoder, since it
      can't know how to update.

***********/

// #define LEN_DELTA  /* causes errors! ?!!? */

// #define DO_MTF /* speed? : hurts */

// #define NO_ML  /* no lengths, just do ppm : helps !! */

// #define MATCHP_L   /** use (totLen/esc) vs. (tot/esc) **/  /* helps a little */

/** turn on excludes whenever it's allowed **/

#if MIN_LEN < 2
#define NO_ML
#define DO_EXCLUDES
#endif

#ifdef ALWAYS_LEN1
#define DO_EXCLUDES
#endif

/**************************************/

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/mempool.h>
#include <crblib/arithc.h>
#include <crblib/o1coder.h>
#include "ppmz_cfg.h"

#ifdef STATS
#include <stdio.h>
#endif // STATS

#define lenFlagMore     64

#define EXTRA_TOT_INIT  256

#define MATCHP_L_DIV  1

#define lenTot (lenFlagMore+1)

#define DT_INC_T	20
#define DT_INC_E	18
#define DT_INC_ET	2

#define DCT_BITS      3   // doesn't seem to affect it!
#define DTSX_BITS     2

#define ESC_PAD       1000  // new params, need tuning
#define TOT_PAD_1     0     // pads take values 0 -> infinity
#define TOT_PAD_2     0

/* */

#define DSXHASH(x)    ( x & ((1<<DTSX_BITS)-1) )
#define DCTHASH(x)    ( min(x,((1<<DCT_BITS)-1)) )

#define DT_SIZE       (1<<(DCT_BITS+DTSX_BITS))
#define DTHASH(c,x)   ( (DCTHASH(c)<<DTSX_BITS) + DSXHASH(x) )

#define LEN_SIZE        ( DT_SIZE )
#define LEN_HASH(index) ( index )

#define DT_NUMCOUNTS  (1<<DCT_BITS)
#define NUM_O1_CHARS  (1<<DTSX_BITS)

#define PPMHASHBITS   16
#define PPMHASH_SIZE  (1<<PPMHASHBITS)
#define PPMHASH(x)    (((x>>16)^x) & (PPMHASH_SIZE-1))

#define CNTXHASH(c1,c2,c3) ( c1 ^ (c2<<17) ^ (c2>>15) ^ (c3<<17) ^ (c3>>25) )

/** context list operations macros **/

#define CONTEXT_ADDHEAD(zContext,zIndex)  {                 \
zContext->next = zIndex->contextList;                       \
zContext->prev = (LZPo12context *)zIndex;                     \
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

typedef struct LZPo12indexStruct LZPo12index;
typedef struct LZPo12contextStruct LZPo12context;

struct LZPo12indexStruct
  {
  LZPo12context * contextList; /* same place as context->next */
  LZPo12index * next;
  ulong cntxHash;
  };

struct LZPo12contextStruct
  {
  LZPo12context *next;
  LZPo12context *prev;

  ulong cntx1,cntx2,cntx3;
  ubyte *string;
  uword esc,tot;
  ulong lastML,totLen;
  };

typedef struct LZPo12infoStruct
  {
  long error; /* if it's not 0, you're dead */

  arithInfo * ari;

  MemPool * contextPool;
  MemPool * indexPool;

  uword * escCounts;
  uword * totCounts;

#ifdef USE_LENCODER
  oOne * lenCoder;
#endif

  LZPo12index * indeces[PPMHASH_SIZE];

#ifdef STATS
  ulong numMatches;
  ulong numZeroMatches;
  ulong totMatchLen,longMatchLen,cntxLongMatchLen;
#endif // STATS

  } LZPo12info;

/* proto */ void LZPo12_CleanUp(LZPo12info * LZPI);

/************ funcs *****************/

LZPo12info * LZPo12_Init(arithInfo * ari)
{
LZPo12info * LZPI;
int i;

if ( (LZPI = new(LZPo12info)) == NULL )
  return(NULL);

#ifdef USE_LENCODER
if ( (LZPI->lenCoder = oOneCreate(ari,lenTot,LEN_SIZE)) == NULL )
  { LZPo12_CleanUp(LZPI); return(NULL); }
#endif

if ( (LZPI->contextPool = AllocPool(sizeof(LZPo12context),1024,1024)) == NULL )
  { LZPo12_CleanUp(LZPI); return(NULL); }

if ( (LZPI->indexPool = AllocPool(sizeof(LZPo12index),1024,1024)) == NULL )
  { LZPo12_CleanUp(LZPI); return(NULL); }

if ( (LZPI->escCounts = malloc(sizeof(uword)*DT_SIZE)) == NULL )
  { LZPo12_CleanUp(LZPI); return(NULL); }
if ( (LZPI->totCounts = malloc(sizeof(uword)*DT_SIZE)) == NULL )
  { LZPo12_CleanUp(LZPI); return(NULL); }
  
for(i=0;i<DT_SIZE;i++)  {
  LZPI->escCounts[i] = 1 + ESC_PAD;
  if ( (i>>DTSX_BITS) == 0 ) LZPI->totCounts[i] = 2 + ESC_PAD + TOT_PAD_1;
  else LZPI->totCounts[i] = 2 + ESC_PAD + TOT_PAD_2;
  }

LZPI->ari = ari;
LZPI->error = 0;

return(LZPI);
}

LZPo12context * LZPo12_Find(LZPo12info * LZPI,ubyte * string)
{
LZPo12index *curIndex,*prevIndex;
LZPo12context *curContext,*gotContext;
ulong cntx1,cntx2,cntx3,cntxHash,hash;

cntx1 = getulong(string-4);
cntx2 = getulong(string-8);
cntx3 = getulong(string-12);
cntxHash = CNTXHASH(cntx1,cntx2,cntx3);
hash = PPMHASH(cntxHash);

curIndex = LZPI->indeces[hash];
while(curIndex)
  {
  if ( curIndex->cntxHash == cntxHash )
    break;

  curIndex = curIndex->next;
  }

prevIndex = NULL;
curIndex = LZPI->indeces[hash];
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
  if ( (curIndex = GetPoolHunk(LZPI->indexPool,1)) == NULL )
    { LZPI->error = 9; return(NULL); }
  curIndex->cntxHash = cntxHash;
  curIndex->contextList = NULL;
  curIndex->next = LZPI->indeces[hash];
  LZPI->indeces[hash] = curIndex;
  }
else if ( prevIndex != NULL ) /* MTF */
  {
  prevIndex->next = curIndex->next;
  curIndex->next = LZPI->indeces[hash];
  LZPI->indeces[hash] = curIndex;
  }

curContext = curIndex->contextList;
gotContext = NULL;

#ifdef DEBUG
printf("%08X %08X %08X : ",cntx3,cntx2,cntx1);
#endif

while(curContext) {

  if ( curContext->cntx1 == cntx1 &&
       curContext->cntx2 == cntx2 &&
       curContext->cntx3 == cntx3 ) {
    gotContext = curContext;
    CONTEXT_MTF(gotContext,curIndex);

#ifdef DEBUG
printf(" : %02X , %d / %d\n",gotContext->string[0],gotContext->esc,gotContext->tot);
#endif

    return(gotContext);
  }

  curContext = curContext->next;
}

#ifdef DEBUG
printf(" none \n");
#endif

/*** not found, make new ****/

if ( (curContext = GetPoolHunk(LZPI->contextPool,1)) == NULL )
  { LZPI->error = 9; return(0); }

curContext->cntx1 = cntx1;
curContext->cntx2 = cntx2;
curContext->cntx3 = cntx3;
curContext->string = string;
curContext->lastML = 0;
curContext->totLen = 0;
curContext->esc = 1; curContext->tot = 2;

CONTEXT_ADDHEAD(curContext,curIndex);

return(NULL);
}

/*********** ML I/O func ********

 order1 code in LZPI with index as order1 :

  use Det tables to write match/nomatch

  do order-1 length coding on index; index is roughly 9 bits , so it IS kosher to
    just use index as an order-1 context to the good-old o1coder routines.

*********/

void LZP_WriteExtra(arithInfo * ari,int len)
{
int max;

max = EXTRA_TOT_INIT;

  for(;;) {
    if ( len >= max ) {
      arithEncode(ari,max,max+1,max+1);
      len -= max;
      if ( max < 1024 ) max += max;
    } else {
      arithEncode(ari,len,len+1,max+1);
      return;
    }
  }
}

int LZP_ReadExtra(arithInfo * ari)
{
int len,read,max;

max = EXTRA_TOT_INIT;
len = 0;

  read_more:

    read = arithGet(ari,max+1);
    arithDecode(ari,read,read+1,max+1);
    len += read;

    if ( read == max ) {
      if ( max < 1024 ) max += max;
      goto read_more;
    }

return(len);
}


ulong LZPo12_ReadML(LZPo12info * LZPI,ulong index,ulong lastML)
{
ulong len;
ulong escP,totP,gotP;

  escP = LZPI->escCounts[index];
  totP = LZPI->totCounts[index];

  if ( totP >= LZPI->ari->safeProbMax ) {
    while ( totP >= LZPI->ari->safeProbMax )
      { totP >>= 1; escP >>= 1; }
    if ( ! escP ) { escP++; totP++; }
    LZPI->totCounts[index] = totP;
    LZPI->escCounts[index] = escP;
   }

  LZPI->totCounts[index] += DT_INC_T;

  gotP = arithGet(LZPI->ari,totP);

  if ( gotP < escP ) {
    arithDecode(LZPI->ari,0,escP,totP);
    LZPI->escCounts[index] += DT_INC_E;
    return(0);
  } else {
    arithDecode(LZPI->ari,escP,totP,totP);
  }

#ifdef NO_ML
return(1);
#endif

#ifdef USE_LENCODER
len = oOneDecode(LZPI->lenCoder,LEN_HASH(index));

  if ( len == 0 ) {
    len = - LZP_ReadExtra(LZPI->ari);
  } else if ( len == lenFlagMore ) {
    len += LZP_ReadExtra(LZPI->ari);
  }
#else
len = LZP_ReadExtra(LZPI->ari);
#endif

#ifdef LEN_DELTA
len = len + lastML - (lenFlagMore>>1);
#endif

#ifdef ALWAYS_LEN1
if ( len == 0 )
  len = 1;
else
  len += MIN_LEN - 1;
#else
  len += MIN_LEN;
#endif

#ifdef DEBUG
  printf("l:%d , h: %04X\n",len,index);
#endif

return(len);
}

void LZPo12_WriteML(LZPo12info * LZPI,ulong len,ulong index,ulong lastML)
{
long origLen;
ulong escP,totP;

  escP = LZPI->escCounts[index];
  totP = LZPI->totCounts[index];

  if ( totP >= LZPI->ari->safeProbMax ) {
    while ( totP >= LZPI->ari->safeProbMax )
      { totP >>= 1; escP >>= 1; }
    if ( ! escP ) { escP++; totP++; }
    LZPI->totCounts[index] = totP;
    LZPI->escCounts[index] = escP;
   }

  LZPI->totCounts[index] += DT_INC_T;

  if ( len > 0 ) {
    arithEncode(LZPI->ari,escP,totP,totP);
  } else {
    arithEncode(LZPI->ari,0,escP,totP);
    LZPI->escCounts[index] += DT_INC_E;
    return;
   }

#ifdef NO_ML
return;
#endif

#ifdef DEBUG
  printf("l:%d , h: %04X\n",len,index);
#endif

#ifdef ALWAYS_LEN1
if ( len == 1 )
  len = 0;
else
  len -= MIN_LEN - 1;
#else
  len -= MIN_LEN;
#endif

#ifdef LEN_DELTA
len = len - lastML + (lenFlagMore>>1);
#endif

#ifdef USE_LENCODER
  if ( len <= 0 ) {
    oOneEncode(LZPI->lenCoder,0,LEN_HASH(index));
    LZP_WriteExtra(LZPI->ari, - len);
  } else if ( len >= lenFlagMore ) {
    oOneEncode(LZPI->lenCoder,lenFlagMore,LEN_HASH(index));
    LZP_WriteExtra(LZPI->ari,len - lenFlagMore);
  } else {
    oOneEncode(LZPI->lenCoder,len,LEN_HASH(index));
  }
#else
  LZP_WriteExtra(LZPI->ari,len);
#endif


return;
}

/****************** ****************/

long LZPo12_Write(LZPo12info * LZPI,ubyte *string,long * excludePtr)
{
LZPo12context * curContext;
long matchLen,matchP;
ubyte *cntxStr,*vsStr;

*excludePtr = -1; 

if ( ! (curContext = LZPo12_Find(LZPI,string)) )
  return 0;

cntxStr = curContext->string; matchLen = 0;
vsStr = string;

#ifdef DO_EXCLUDES
*excludePtr = *cntxStr;
#endif

while(*vsStr++ == *cntxStr++) matchLen++;

#ifdef MATCHP_L
matchP = (curContext->totLen/curContext->esc)/MATCHP_L_DIV;
#else
matchP = (curContext->tot - curContext->esc - 1)/curContext->esc;
#endif

#ifdef DEBUG
printf(" p: %02X , h: %04X , mp:%d , ml:%d\n",curContext->string[0],DTHASH(matchP,string[-1]),matchP,matchLen);
#endif

#ifdef ALWAYS_LEN1
if ( matchLen < MIN_LEN && matchLen > 1 )
  matchLen = 0;
#else
if ( matchLen < MIN_LEN )
  matchLen = 0;
#endif

#ifdef NO_ML
if ( matchLen > 1 ) matchLen = 1;
#endif

LZPo12_WriteML(LZPI,matchLen,DTHASH(matchP,string[-1]),curContext->lastML);

curContext->lastML = matchLen;
curContext->totLen += matchLen;
curContext->tot++;
if ( matchLen == 0 ) curContext->esc++;

#ifdef STATS
if ( matchLen == 0 ) LZPI->numZeroMatches++;
else LZPI->numMatches++;
LZPI->totMatchLen += matchLen;
LZPI->longMatchLen = max(LZPI->longMatchLen,matchLen);
LZPI->cntxLongMatchLen = max(LZPI->cntxLongMatchLen,curContext->totLen);
#endif // STATS

return(matchLen);
}

long LZPo12_Read(LZPo12info * LZPI,ubyte *string,long * excludePtr)
{
LZPo12context *curContext;
long matchLen,matchP;

*excludePtr = -1; 

if ( ! (curContext = LZPo12_Find(LZPI,string)) ) {
  return(0);
}

#ifdef DO_EXCLUDES
*excludePtr = *(curContext->string);
#endif

#ifdef MATCHP_L
matchP = (curContext->totLen/curContext->esc)/MATCHP_L_DIV;
#else
matchP = (curContext->tot - curContext->esc - 1)/curContext->esc;
#endif

matchLen = LZPo12_ReadML(LZPI,
  DTHASH(matchP,string[-1]),
  curContext->lastML);

#ifdef DEBUG
printf(" p: %02X , h: %04X , mp:%d , ml:%d\n",curContext->string[0],DTHASH(matchP,string[-1]),matchP,matchLen);
#endif

curContext->lastML = matchLen;
curContext->totLen += matchLen;
curContext->tot++;

  if ( matchLen > 0 ) {
    ubyte *fromStr; long ml;

    fromStr = curContext->string;
    for(ml=matchLen;ml--;) *string++ = *fromStr++;
  } else {
    curContext->esc++;
  }

return(matchLen);
}

void LZPo12_CleanUp(LZPo12info * LZPI)
{
if (! LZPI ) return;

#ifdef STATS
if ( (LZPI->numMatches + LZPI->numZeroMatches) > 0 ) {
  int num_counts = 1<<DCT_BITS;
  ulong esc[1<<DCT_BITS],tot[1<<DCT_BITS];
  int i,j;

  for(i=0;i<num_counts;i++) {
    esc[i] = tot[i] = 0;
  }

  for(i=0;i<DT_SIZE;i++) {
    j = i>>DTSX_BITS;
    esc[j] += LZPI->escCounts[i];
    tot[j] += LZPI->totCounts[i];
  }

  errputs("=============");

  for(i=0;i<num_counts;i++) {
    printf("%d : %f\n",i,((float)esc[i]/(float)tot[i]));
  }

  errputs("====== LZPo12 Stats =======");
  fprintf(stderr,"numMatches      : %ld\n",LZPI->numMatches);
  fprintf(stderr,"numZeroMatches  : %ld\n",LZPI->numZeroMatches);
  fprintf(stderr,"totMatchLen     : %ld\n",LZPI->totMatchLen);
  fprintf(stderr,"longMatchLen    : %ld\n",LZPI->longMatchLen);
  fprintf(stderr,"cntxLongMatchLen: %ld\n",LZPI->cntxLongMatchLen);
  errputs("=============");
}
#endif // STATS

smartfree(LZPI->escCounts);
smartfree(LZPI->totCounts);

#ifdef USE_LENCODER
if ( LZPI->lenCoder ) oOneFree(LZPI->lenCoder);
#endif

if ( LZPI->contextPool ) FreePool(LZPI->contextPool);
if ( LZPI->indexPool ) FreePool(LZPI->indexPool);

free(LZPI);
}
