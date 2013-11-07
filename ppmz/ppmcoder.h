#ifndef PPMCODER_H
#define PPMCODER_H

#include <crblib/arithc.h>

struct PPMcoderInfo {
	bool doDebug; // set externally
	long Private; 
};

extern struct PPMcoderInfo * PPM_WriteInit(arithInfo * ari,long NumSymbols,bool DoLRU);
extern bool PPM_WriteDone(struct PPMcoderInfo * PPMCI);

extern bool PPM_CheckErr(struct PPMcoderInfo * PPMCI); /* returns 0 for bad */

extern struct PPMcoderInfo * PPM_ReadInit(arithInfo * ari,long NumSymbols,bool DoLRU);
extern bool PPM_ReadDone(struct PPMcoderInfo * PPMCI);

extern void PPMcoder_CleanUp(struct PPMcoderInfo * PPMCI);
extern void PPMcoder_UpdateC(struct PPMcoderInfo *PPMCI,long GotC,ulong Cntx,ulong Cntx2);

typedef void (*PPM_WriteFunc) (struct PPMcoderInfo *,long,ulong,ulong,ubyte *);
typedef long (*PPM_ReadFunc)  (struct PPMcoderInfo *,ulong,ulong,ubyte *);
typedef bool (*PPM_InitFunc)  (struct PPMcoderInfo *);

extern void PPM_ShowFingerPrint(struct PPMcoderInfo * PPMCI);
	/* for debug, prints to stdout */

extern const int CoderNum_Max;
extern PPM_InitFunc PPM_Inits[];
extern PPM_WriteFunc PPM_Writers[];
extern PPM_ReadFunc  PPM_Readers[];
extern int PPMcoder_OrderList[];

extern bool LZPlit_Init  (struct PPMcoderInfo * PPMCI);
extern void LZPlit_Write (struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2,ubyte *s,long LZPlit_ExcludeC);
extern void LZPlit_Update(struct PPMcoderInfo * PPMCI,long GotC,ulong Context,ulong Cntx2);
extern long LZPlit_Read  (struct PPMcoderInfo * PPMCI,          ulong Context,ulong Cntx2,ubyte *s,long LZPlit_ExcludeC);

#endif
