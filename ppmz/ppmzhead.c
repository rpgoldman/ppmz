
/*******

HeaderLen not included in report of results because all info
in the header is not necessary for compression decompression.

Header consists of:

  4-char signature, for convenient decoding
  ulong  CRC ,   for convenient error-checking
  ulong  rawlen, so that the buffer size can be known (i.e. I can 
                    use fread instead of fgetc)
  
  3-ulong RunTransform info, again for array allocation
    (the fact that these are not needed is proved by the fact
      that they are not passed to UnRunTransform)

---

exception :

  char   CoderNum : this one actually is needed
                  it is only 1 byte long and IS included in the report

*******/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <crblib/inc.h>
#include <crblib/memutil.h>
#include <crblib/fileutil.h>

#include "ppmzhead.h"

void PPMZheader_Read (FILE *FP,PPMZ_Header * header)
{
//fflush(FP);
MemClear(header,sizeof(PPMZ_Header));
header->signature    = readL(FP); 
header->version      = readB(FP);
header->revision     = readB(FP);
header->coderNum     = readB(FP);
header->flags        = readB(FP);
header->rawLen       = readL(FP); 
header->crc          = readL(FP); 
if ( header->flags & HeaderFlag_DoRunTransform )
  {
  header->literalLen   = readL(FP); 
  header->runPackedLen = readL(FP); 
  header->numRuns      = readL(FP);
  }
if ( header->flags & HeaderFlag_DoLRU )
  header->LRUlen = readL(FP);
//fflush(FP);
}

void PPMZheader_Write(FILE *FP,PPMZ_Header * header)
{
fflush(FP);
writeL(FP,header->signature  );
writeB(FP,header->version );
writeB(FP,header->revision );
writeB(FP,header->coderNum);
writeB(FP,header->flags);
writeL(FP,header->rawLen);
writeL(FP,header->crc);
if ( header->flags & HeaderFlag_DoRunTransform )
  {
  writeL(FP,header->literalLen);
  writeL(FP,header->runPackedLen);
  writeL(FP,header->numRuns);
  }
if ( header->flags & HeaderFlag_DoLRU )
  writeL(FP,header->LRUlen);
fflush(FP);
}

