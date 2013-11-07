#ifndef PPMZESC_H
#define PPMZESC_H

typedef struct {
	int private;
} escInfo;

escInfo * PPMZesc_open(arithInfo * ari);
void PPMZesc_close(escInfo *ei);

void writePPMZesc(bool escape,ulong cntx,int escC,int totC,int numC,escInfo * ei,int order);
bool readPPMZesc(ulong cntx,int escP,int totP,int numC,escInfo * ei,int order);

void PPMZesc_updateP(bool escape,ulong cntx,int escC,int totC,int numC,escInfo * ei,int order);
void PPMZesc_getP(int *escPptr,int *totPptr,ulong cntx,int escC,int totC,int numC,escInfo * ei,int order);

#endif //PPMZESC_H
