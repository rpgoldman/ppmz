#ifndef PPMDET_H
#define PPMDET_H

struct PPMdetInfo
  {
  long Error; /* if it's not 0, you're in trouble */

  long Private;
  };

extern struct PPMdetInfo * PPMdet_Init(arithInfo * ari);

/*
 * return-value indicates whether char was written with
 *  current order or not.  If not, you MUST write it with
 *  some other model.
 *
 */
extern bool PPMdet_EncodeC(struct PPMdetInfo * PPMI,long  symbol,ubyte *curPtr,exclusion * Exclusion);
extern bool PPMdet_DecodeC(struct PPMdetInfo * PPMI,long *symPtr,ubyte *curPtr,exclusion * Exclusion);
extern void PPMdet_DecodeGotC(struct PPMdetInfo * PPMI,long GotC);
extern void PPMdet_CleanUp(struct PPMdetInfo * PPMI);

#endif
