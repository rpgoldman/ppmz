#ifndef LZPo12_H
#define LZPo12_H

struct LZPo12Info { int private; };
extern struct LZPo12Info * LZPo12_Init(arithInfo * ari);
extern void LZPo12_CleanUp(struct LZPo12Info * LZPDI);
extern long LZPo12_Write(struct LZPo12Info * LZPDI,ubyte *string,long * excludeP);
extern long LZPo12_Read (struct LZPo12Info * LZPDI,ubyte *string,long * excludeP);

#endif

