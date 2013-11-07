#ifndef PPMZ_EXCLUDE_H
#define PPMZ_EXCLUDE_H

typedef struct { int private; } exclusion;

extern exclusion * newExclusion(int numChars);
extern void freeExclusion(exclusion * E);
extern void clearExclusion(exclusion * E);
extern void setExclude(exclusion * E,int sym);
extern bool isExcluded(exclusion * E,int sym);

extern void resetExclusion(exclusion * E);
	/* resetE is really just internal; use clearE instead */

#define setExcluded(e,s) setExclude(e,s)

#endif
 
