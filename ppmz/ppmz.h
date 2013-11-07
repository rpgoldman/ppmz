#ifndef PPMZ_H
#define PPMZ_H

#include "ppmzesc.h"

struct PPMZ_MemPools { long Private; };

struct PPMZInfo
  {
  long Error; /* if it's not 0, you're fucked */
  int order;
  
  /** all else is private **/
  };

#define PPMZF_DT_ONE 1
#define PPMZF_DT_TWO 2
#define PPMZF_LRU    4 /* turns on memory-use amortization */

extern struct PPMZ_MemPools * PPMZ_AllocPools(void);
extern void PPMZ_FreePools(struct PPMZ_MemPools * Pools);

extern struct PPMZInfo * PPMZ_Init(arithInfo * ari,
	int Flags,int scaleDown,struct PPMZ_MemPools * Pools,int order,
	escInfo * ei);
extern void PPMZ_CleanUp(struct PPMZInfo * PPMI);

extern void PPMZ_GetInfo(struct PPMZInfo * PPMI,ulong Context,ulong HOContext,
  ulong * MPS_P_ptr,ulong * Esc_P_ptr,ulong *entropy_ptr);

/** mps_p and esc_p are returned as fractions multiplied by ESC_SCALE
**/

/*
 * return-value indicates whether char was written with
 *  current order or not.  If not, you MUST write it with
 *  some other model.
 *
 */
extern bool PPMZ_EncodeC(struct PPMZInfo * PPMI,long Symbol,ulong Context,ulong HOContext,exclusion * Exclusion);

extern bool PPMZ_UpdateC(struct PPMZInfo * PPMI,long Symbol,ulong Context,ulong HOContext);

extern bool PPMZ_DecodeC(struct PPMZInfo * PPMI,long * SymbolPtr,ulong Context,ulong HOContext,exclusion * Exclusion);
extern void PPMZ_DecodeGotC(struct PPMZInfo * PPMI,uword GotC);


/*********

	FingerPrint is a useful debugging tool.  It makes 32-bit CRC of the context tree.
	After N bytes, the encoder and decoder should have the same fingerprint.

	Note that FingerPrint is quite slow because it has to walk all over the data structure.

***********/

extern ulong PPMZ_FingerPrint(struct PPMZInfo * PPMI);

#endif /* PPMZ */
