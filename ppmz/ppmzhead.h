#ifndef PPMZ_HEADER_H
#define PPMZ_HEADER_H

typedef struct PPMZ_Header_Struct PPMZ_Header;

#define HeaderFlag_Preconditioned (1<<0)
#define HeaderFlag_DoLRU          (1<<1)
#define HeaderFlag_DoRunTransform (1<<2)
#define HeaderFlag_DoLZP          (1<<3)

#define CODERNUM_RAW				0xFF

struct PPMZ_Header_Struct
	{
	ulong signature;
	ubyte version;
	ubyte revision;
	ubyte coderNum;
	ubyte flags;
	ulong rawLen;
	ulong crc;

	/* stuff conditionally written based on flags : */

	ulong literalLen,runPackedLen,numRuns; /* only written if DoRT is on */
	ulong LRUlen; /* only written if DoLRU is on */
	};

extern void PPMZheader_Read (FILE *FP,PPMZ_Header * header);
extern void PPMZheader_Write(FILE *FP,PPMZ_Header * header);

#endif
