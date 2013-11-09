#ifndef _CRBEQUTIL_H
#define _CRBEQUTIL_H

#include <crblib/inc.h>

extern void StrBump (ubyte *Dat,long Off,long Len);
extern ubyte * CorrespondP(ubyte *str);
extern long Min(long a,long b);
extern long UnmatchedParenthesis (ubyte *str); /*returns 0 if all match ok*/
extern void StrUprNotQuoted(ubyte *str);
extern void StrCutSpaceNotQuoted(ubyte *str);
extern void MakeResult(char *Str,double V,int Precision);

#endif
