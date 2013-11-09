
//#define DEBUG
//#define ACCEPT_FIRST_MATCH //for speed
//#define NO_PPMdet
//#define DO_MTF

//toggles:

#define LONG_DETMINLEN_INIT /* helps, a reasonable amount */
// #define COPY_GOTCONTEXT_INIT /* hurts, barely */
// #define COPY_CNTXML_INIT /* doesn't seem to matter */

#define DET_MAX_ML 99999

/**************************************/

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/mempool.h>
#include <crblib/arithc.h>
#include "exclude.h"
#include "ppmzesc.h"
#include "ppmz_cfg.h"

/* config settings ; shared with PPMZ */

#include "ppmz_cfg.h"

#define PPMDHASHBITS 16
#define PPMDHASH_SIZE  (1<<PPMDHASHBITS)
#define PPMDHASH(x)    (((x>>16)^x) & (PPMDHASH_SIZE-1))

#define CNTXHASH(c1,c2,c3) ( c1 ^ (c2<<17) ^ (c2>>15) ^ (c3<<7) ^ (c3>>25) )


/** context list operations macros **/

#define CONTEXT_ADDHEAD(zContext,zIndex)  {                 \
zContext->next = zIndex->contextList;                       \
zContext->prev = (PPMDcontext *)zIndex;                     \
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

typedef struct PPMDindexStruct PPMDindex;
typedef struct PPMDcontextStruct PPMDcontext;

struct PPMDindexStruct
  {
  PPMDcontext * contextList; /* same place as context->next */

  PPMDindex * next;
  ulong cntxHash;
  };

struct PPMDcontextStruct
  {
  PPMDcontext *next;
  PPMDcontext *prev;

  ulong cntx1,cntx2,cntx3;
  ubyte * string;
  uword detMinLen;
  uword count;
  };

typedef struct PPMDinfoStruct
  {
  long error; /* if it's not 0, you're dead */

  arithInfo * ari;

  MemPool * contextPool;
  MemPool * indexPool;

	escInfo *escapeInfo;

  PPMDindex * indeces[PPMDHASH_SIZE];

  } PPMDinfo;

/* proto */ void PPMdet_CleanUp(PPMDinfo * PPMDI);

/************ funcs *****************/

PPMDinfo * PPMdet_Init(arithInfo * ari)
{
PPMDinfo * PPMDI;

if ( (PPMDI = new(PPMDinfo)) == NULL )
  return(NULL);

if ( (PPMDI->contextPool = AllocPool(sizeof(PPMDcontext),1024,1024)) == NULL )
  { PPMdet_CleanUp(PPMDI); return(NULL); }

if ( (PPMDI->indexPool = AllocPool(sizeof(PPMDindex),1024,1024)) == NULL )
  { PPMdet_CleanUp(PPMDI); return(NULL); }

if ( (PPMDI->escapeInfo = PPMZesc_open(ari)) == NULL )
  { PPMdet_CleanUp(PPMDI); return(NULL); }
  

PPMDI->ari = ari;
PPMDI->error = 0;

return(PPMDI);
}

PPMDcontext * PPMdet_Find(PPMDinfo * PPMDI,ubyte * string,long * cntxMLptr)
{
PPMDindex *curIndex,*prevIndex;
PPMDcontext *curContext,*gotContext;
ulong cntx1,cntx2,cntx3,cntxHash,hash;
long cntxML;
#ifdef LONG_DETMINLEN_INIT
long longestML;
#endif

cntx1 = getulong(string-4);
cntx2 = getulong(string-8);
cntx3 = getulong(string-12);
cntxHash = CNTXHASH(cntx1,cntx2,cntx3);
hash = PPMDHASH(cntxHash);

#ifdef DEBUG
/*$*/ printf("Find : %08X %08X %08X : ",cntx3,cntx2,cntx1);
#endif

prevIndex = NULL;
curIndex = PPMDI->indeces[hash];
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
  if ( (curIndex = GetPoolHunk(PPMDI->indexPool,1)) == NULL )
    { PPMDI->error = 9; return(NULL); }
  curIndex->cntxHash = cntxHash;
  curIndex->contextList = NULL;
  curIndex->next = PPMDI->indeces[hash];
  PPMDI->indeces[hash] = curIndex;
  }
else if ( prevIndex != NULL ) /* MTF */
  {
  prevIndex->next = curIndex->next;

  curIndex->next = PPMDI->indeces[hash];
  PPMDI->indeces[hash] = curIndex;
  }


curContext = curIndex->contextList;
gotContext = NULL;
cntxML = 0;

#ifdef LONG_DETMINLEN_INIT
longestML = 0;
#endif

for(;;) {
  if ( curContext == NULL ) break;

  if ( curContext->cntx1 == cntx1 &&
       curContext->cntx2 == cntx2 &&
       curContext->cntx3 == cntx3 )
    {
    ubyte *ap,*bp;
    long ml;

    ap = string - 13;
    bp = (curContext->string) - 13;
    ml = 12;
    while( *ap-- == *bp-- ) if ( ++ml == DET_MAX_ML ) break;

#ifdef LONG_DETMINLEN_INIT
    if ( ml > longestML ) longestML = ml;
#endif

    if ( ml >= curContext->detMinLen && ml > cntxML )
      {
      cntxML = ml;
      gotContext = curContext;
#ifdef ACCEPT_FIRST_MATCH
      break;
#endif
      }

    }

  curContext = curContext->next;
  }

*cntxMLptr = cntxML;

if ( (curContext = GetPoolHunk(PPMDI->contextPool,1)) == NULL )
  { PPMDI->error = 9; return(0); }

curContext->string = string;
curContext->cntx1 = cntx1;
curContext->cntx2 = cntx2;
curContext->cntx3 = cntx3;
curContext->count = 1;
curContext->detMinLen = 0;

#ifdef LONG_DETMINLEN_INIT
curContext->detMinLen = longestML + 1;
#endif

  if ( gotContext ) {
#ifdef COPY_CNTXML_INIT
    curContext->detMinLen = cntxML + 1;
#endif
#ifdef COPY_GOTCONTEXT_INIT
    curContext->count = gotContext->count;
#endif
  }

CONTEXT_ADDHEAD(curContext,curIndex);

if ( gotContext )
  {
  CONTEXT_MTF(gotContext,curIndex);
  }

#ifdef DEBUG
/*$*/ 
if ( gotContext ) 
  printf(" %02X : %d, %d\n",*(gotContext->string),gotContext->count,gotContext->detMinLen);
else
  printF(" none\n");
/*$*/ 
#endif

return(gotContext);
}

/****************** ****************/

/* Uh... apparently, this structure was ignored in its definition by the
	author of this program because they believed it wasn't important.
	-psilord 2013-11-08
*/
struct PPMdetInfo { };

/* However, lots of places call this function and pass it stuff that will
	get ignored.
	-psilord 2013-11-08
*/
void PPMdet_DecodeGotC(struct PPMdetInfo * PPMDI,long GotC)
{
/* ne fait rien */
PPMDI = PPMDI;
GotC = GotC;
}

bool PPMdet_EncodeC(PPMDinfo * PPMDI,long gotC,ubyte *string,exclusion * Exclusion)
{
PPMDcontext * curContext;
long cntxML,pred;

#ifdef NO_PPMdet
return 0;
#endif

  if ( (curContext = PPMdet_Find(PPMDI,string,&cntxML)) == NULL )
    return false;

  pred = *(curContext->string);

  if ( isExcluded(Exclusion,pred) )
    return false;

  if ( pred == gotC ) {
	writePPMZesc(nope,getulong(string-4),1,1+curContext->count,1,PPMDI->escapeInfo,9);
    curContext->count++;
    return true;
  } else {
	writePPMZesc(yep,getulong(string-4),1,1+curContext->count,1,PPMDI->escapeInfo,9);
    curContext->detMinLen = cntxML + DET_ariLED_ML_INC;
    setExcluded(Exclusion,pred);
    return false;
  }
}

bool PPMdet_DecodeC(PPMDinfo * PPMDI,long * gotCptr,ubyte *string,exclusion * Exclusion)
{
PPMDcontext *curContext;
long cntxML,pred;
bool escape;

#ifdef NO_PPMdet
return 0;
#endif

  if ( (curContext = PPMdet_Find(PPMDI,string,&cntxML)) == NULL )
    return false;

  pred = *(curContext->string);

  if ( isExcluded(Exclusion,pred) )
    return false;

  escape = readPPMZesc(getulong(string-4),1,1+curContext->count,1,PPMDI->escapeInfo,9);

  if ( !escape ) {
    *gotCptr = pred;
    curContext->count++;
  } else {
    setExcluded(Exclusion,pred);
    curContext->detMinLen = cntxML + DET_ariLED_ML_INC;
  }

return(!escape);
}

void PPMdet_CleanUp(PPMDinfo * PPMDI)
{
if (! PPMDI ) return;

if ( PPMDI->escapeInfo ) PPMZesc_close(PPMDI->escapeInfo);

if ( PPMDI->contextPool ) FreePool(PPMDI->contextPool);
if ( PPMDI->indexPool ) FreePool(PPMDI->indexPool);

free(PPMDI);
}
