#ifndef LZPDET_H
#define LZPDET_H

struct LZPdetInfo { int private; };

extern struct LZPdetInfo * LZPdet_Init(arithInfo * ari);
extern void LZPdet_CleanUp(struct LZPdetInfo * LZPDI);

extern long LZPdet_Write(struct LZPdetInfo * LZPDI,ubyte *string,long * excludeP);

extern long LZPdet_Read (struct LZPdetInfo * LZPDI,ubyte *string,long * excludeP);

#endif

