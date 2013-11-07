/***

compare to malcolm's hash:

he uses:

4 bits for tot,4 for esc // doesn't seem to help
bit 6 from last 4 bytes  // is good
1 bit to flag whether the next lower order is empty or not // doesn't seem to help
merges orders for esc > 3 // I don't do esc > 3

***/

//#define DEBUG

//#define FLOAT_WEIGHTED

#define SEED_HASHES

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <crblib/inc.h>
#include <crblib/arithc.h>
#include <crblib/intmath.h>

#define ORDERS 3

#include "ppmz_cfg.h"

int order_bits[ORDERS] = { 7, 15, 16 };
#define order_size(x) (1<<order_bits[(x)])

typedef struct {
	arithInfo * ari;
	uword * esc[ORDERS];
	uword * tot[ORDERS];
} escInfo;

void PPMZesc_getP(int *escPptr,int *totPptr,ulong cntx,int escC,int totC,int numC,escInfo * ei,int PPMorder);
void PPMZesc_updateP(bool escape,ulong cntx,int escC,int totC,int numC,escInfo * ei,int PPMorder);
void PPMZesc_close(escInfo *ei);

escInfo * PPMZesc_open(arithInfo * ari)
{
escInfo *ei;
int i,j;

	if ( (ei = new(escInfo)) ==NULL )
		return NULL;

	ei->ari = ari;

	for(i=0;i<ORDERS;i++) {
		if ( (ei->esc[i] = malloc( order_size(i) * sizeof(uword) )) == NULL )
			PPMZesc_close(ei);
		if ( (ei->tot[i] = malloc( order_size(i) * sizeof(uword) )) == NULL )
			PPMZesc_close(ei);
	}

	/* seed tables, requires some knowledge of the hash */

	for(i=0;i<ORDERS;i++) {
		for(j=0;j<order_size(i);j++) {
#ifdef SEED_HASHES
			int esc,tot;
			esc = j & 0x3; tot = (j>>2)&0x7;
			ei->esc[i][j] = 1 + (ZESC_INIT_SCALE * esc) + ZESC_INIT_ESC;
			ei->tot[i][j] = 2 + (ZESC_INIT_SCALE * tot) + ZESC_INIT_ESC + ZESC_INIT_TOT;
#else
			ei->esc[i][j] = 1 + ZESC_INIT_ESC;
			ei->tot[i][j] = 2 + ZESC_INIT_ESC + ZESC_INIT_TOT;
#endif
		}
	}

return(ei);
}

void PPMZesc_close(escInfo *ei)
{

	if ( ei ) {
		int i;

		for(i=0;i<ORDERS;i++) {
			smartfree( ei->esc[i] );
			smartfree( ei->tot[i] );
		}

		free(ei);
	}

}

void writePPMZesc(bool escape,ulong cntx,int escC,int totC,int numC,escInfo * ei,int PPMorder)
{
int escP,totP;

if ( numC == 0 || escC == 0 || escC == totC ) // this can happen when all characters in a context are excluded
	return;

PPMZesc_getP(&escP,&totP,cntx,escC,totC,numC,ei,PPMorder);
PPMZesc_updateP(escape,  cntx,escC,totC,numC,ei,PPMorder);

//printf("%d,%d\n",escP,totP);

if ( escape )
  arithEncode(ei->ari,0,escP,totP);
else
  arithEncode(ei->ari,escP,totP,totP);

return;
}

bool readPPMZesc(ulong cntx,int escC,int totC,int numC,escInfo * ei,int PPMorder)
{
int escP,totP,gotP;
bool escape;

if ( numC == 0 || escC == totC ) {
	// this can happen when all characters in a context are excluded
	// just return escape
	return true;
} else if ( escC == 0 ) {
	return false; //just return no-escape
}

PPMZesc_getP(&escP,&totP,cntx,escC,totC,numC,ei,PPMorder);

//printf("%d,%d\n",escP,totP);

gotP = arithGet(ei->ari,totP);
if ( gotP < escP ) {
	escape = true;
	arithDecode(ei->ari,0,escP,totP);
} else {
	escape = false;
	arithDecode(ei->ari,escP,totP,totP);
}

PPMZesc_updateP(escape,  cntx,escC,totC,numC,ei,PPMorder);

return escape;
}

bool PPMZesc_hash(ulong *hashPtr,ulong cntx,int escC,int totC,int PPMorder,escInfo *ei)
{
int counts,totP;

switch(PPMorder) { // map it to 2 bits
	case 0: case 1: PPMorder = 0; break;
	case 2: case 3: PPMorder = 1; break;
	case 4: case 5: PPMorder = 2; break;
	default: PPMorder = 3; break;
}

switch(escC-1) {
	case 0: counts = 0; break;
	case 1: counts = 1; break;
	case 2: counts = 2; break;
	case 3: counts = 3; break;
	default: return false; // don't handle escapes higher than 3
}
switch(totC-2) {
	case 0: totP = 0; break;
	case 1: totP = 1; break;
	case 2: totP = 2; break;
	case 3: case 4: totP = 3; break;
	case 5: case 6: totP = 4; break;
	case 7: case 8: case 9: totP = 5; break;
	case 10: case 11: case 12: totP = 6; break;
	default: totP = 7; break;
}
counts += totP << 2; // total of 5 bits

//actually, numC is totally un-interesting to us, because
// numC == escC when both are low

/** <> hash alg matters alot and is very hard to tune **/

/***

hashPtr[2] = counts + ((cntx&0x7F)<<5) + (((cntx>>13)&0x3)<<12) + (((cntx>>21)&0x3)<<14) + (PPMorder<<18);
hashPtr[1] = counts + ((cntx & 0x7F)<<5) + (PPMorder<<12);
hashPtr[0] = counts + ((cntx & 0x60)<<1) + (PPMorder<<8);

***/

/*** this (&0x3) from two next characters is the 'a'/'A'/' '/'\n' flag ***/

hashPtr[2] = counts + (( (cntx&0x7F) + (((cntx>>13)&0x3)<<7) + (PPMorder<<9) )<<5);

hashPtr[1] = counts + (( ((cntx>>5)&0x3) + (((cntx>>13)&0x3)<<2)
			+ (((cntx>>21)&0x3)<<4) + (((cntx>>29)&0x3)<<6) + (PPMorder<<8) )<<5);

hashPtr[0] = counts + (PPMorder<<5);

#ifdef DEBUG
if ( hashPtr[0] >= order_size(0) ) errputs("too big zesc hash 0");
if ( hashPtr[1] >= order_size(1) ) errputs("too big zesc hash 1");
if ( hashPtr[2] >= order_size(2) ) errputs("too big zesc hash 2");
#endif

return true;
}

void PPMZesc_getP(int *escPptr,int *totPptr,ulong cntx,int escC,int totC,int numC,escInfo * ei,int PPMorder)
{
ulong hash[3];

if ( ! PPMZesc_hash(hash,cntx,escC,totC,PPMorder,ei) ) {
	*escPptr = escC;
	*totPptr = totC;
	return;
}

//end pre-process

#ifdef FLOAT_WEIGHTED
	{
	double e1,e2,e0,h1,h2,h0;
	e2 = (double)ei->esc[2][hash[2]]/(double)ei->tot[2][hash[2]];
	e1 = (double)ei->esc[1][hash[1]]/(double)ei->tot[1][hash[1]];
	e0 = (double)ei->esc[0][hash[0]]/(double)ei->tot[0][hash[0]];
	h2 = 1/(e2 * log2(e2) + (1-e2)*log2(1-e2));
	h1 = 1/(e1 * log2(e1) + (1-e1)*log2(1-e1));
	h0 = 1/(e0 * log2(e0) + (1-e0)*log2(1-e0));

	*escPptr = 8192 * (e2*h2 + e1*h1 + e0*h0) / (h0 + h1 + h2);
	if ( *escPptr < 1 ) *escPptr = 1;
	*totPptr = 8192;
	}
#else //  do it with integers
	{
	int e1,e2,e0,t1,t2,t0,h1,h2,h0;
	e2 = ei->esc[2][hash[2]]; t2 = ei->tot[2][hash[2]];
	e1 = ei->esc[1][hash[1]]; t1 = ei->tot[1][hash[1]];
	e0 = ei->esc[0][hash[0]]; t0 = ei->tot[0][hash[0]];
#ifdef DEBUG
	if ( t0 < 1 || t1 < 1 || t2 < 1 ) { puts("small tot!"); dbf(); }
	if ( t0 < 1 ) t0 = 1; if (t1 < 1) t1 = 1; if ( t2 < 1) t2 = 1;
#endif
	h2 = (t2<<12)/(t2 * ilog2r(t2) - e2 * ilog2r(e2) - (t2-e2) * ilog2r(t2-e2) + 1);
	h1 = (t1<<12)/(t1 * ilog2r(t1) - e1 * ilog2r(e1) - (t1-e1) * ilog2r(t1-e1) + 1);
	h0 = (t0<<12)/(t0 * ilog2r(t0) - e0 * ilog2r(e0) - (t0-e0) * ilog2r(t0-e0) + 1);

	*totPptr = (h0 + h1 + h2);
	*escPptr = (e2*h2/t2 + e1*h1/t1 + e0*h0/t0);
	while ( *totPptr >= 16000 ) {
		*totPptr >>= 1; *escPptr >>= 1;
	}
	if ( *escPptr < 1 ) *escPptr = 1;
	if ( *escPptr >= *totPptr ) *escPptr = *totPptr - 1;

	}
#endif // not float_weight

}

void PPMZesc_updateP(bool escape,ulong cntx,int escC,int totC,int numC,escInfo * ei,int PPMorder)
{
ulong hash[3];
int order;

	if ( ! PPMZesc_hash(hash,cntx,escC,totC,PPMorder,ei) )
		return;

	for(order=ORDERS-1;order>=0;order--) {

		if ( escape ) {
			ei->esc[order][hash[order]] += ZESC_ESC_INC;
			ei->tot[order][hash[order]] += ZESC_ESC_INC + ZESC_ESCTOT_INC;
		} else {
			ei->tot[order][hash[order]] += ZESC_TOT_INC;
		}

		if ( ei->tot[order][hash[order]] > 16000 ) {
			ei->tot[order][hash[order]] >>= 1;
			ei->esc[order][hash[order]] >>= 1;
			if ( ei->esc[order][hash[order]] < 1 )
				ei->esc[order][hash[order]] = 1;
		}

	}
}

