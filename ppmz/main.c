
/*******

HeaderLen not included in report of results because all info
in the header is not necessary for compression decompression.

see ppmzhead.c for details

*******/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include <crblib/inc.h>
#include <crblib/timer.h>
#include <crblib/memutil.h>
#include <crblib/crc32.h>
#include <crblib/cindcatr.h>
#include <crblib/fileutil.h>
#include <crblib/runtrans.h>

#include "ppmcoder.h"
#include "ppmarray.h"
#include "ppmzhead.h"
#include "ppmz_cfg.h"

/************** consts ************/

/* <> this needs to be smarter; use an env variable or something */
#ifdef _AMIGA
#define DEFAULT_OUT_ENC "ram:ppmz.enc"
#define DEFAULT_OUT_DEC "ram:ppmz.dec"
#else
#define DEFAULT_OUT_ENC "r:\\ppmz.enc"
#define DEFAULT_OUT_DEC "r:\\ppmz.dec"
#endif

#define RAWARRAY_PREPAD 1781
  /* for PPMdet ; PPMdet_MaxOrder <= RAWARRAY_PREPAD */

#define ARRAY_POSTPAD 10000

extern const int PPMZ_Version,PPMZ_Revision;

const ulong PPMZ_Sig = ('P'<<24) + ('P'<<16) + ('M'<<8) + 'Z';

/********** exit-freed globals ************/

FILE * RawFP = NULL;
FILE * ArithFP = NULL;
FILE * PrecondFP = NULL;
FILE * DeCompareFP = NULL;
ubyte * DeCompareArray = NULL;
ubyte * PrecondArray = NULL;
ubyte * RawArrayPadded = NULL;
ubyte * ArithArray = NULL;
ubyte * DebugRawArrayPadded = NULL;

/************** protos: *****************/

void CleanUp(char * ExitMess);

/************** functions: *****************/

void main(int argc, char *argv[])
{
clock_t DiffClock;
bool GotDecompressFlag = 0;
ulong MadeCRC,ReadCRC;
ubyte * RawArray = NULL;
ulong RawFileLen,ArithArrayLen;
ulong RT_LiteralArrayLen,RT_NumRuns,RT_RunPackedLen,PPMPackedLen;
ubyte CoderNum=0;
ubyte *RunArray,*PPMArray;
int i;
bool DoAllCoders,WantsHelp,FindBestCoder,TuneConfig;
char *InName,*OutName,*PreconditionName,*DeCompareName;
int PrecondArrayLen=0;
bool DoLRU,Debug,DoRunTransform;
PPMZ_Header header;
bool DoLZP;
#ifdef unix
#define NumPerSec(RawFileLen,DiffClock) ((ulong)CLOCKS_PER_SEC*RawFileLen/DiffClock)
#endif

fprintf(stderr,"PPMZ v%d.%d copyright (c) 1995,96,97 Charles Bloom\n",PPMZ_Version,PPMZ_Revision);

/** parse input **/

InName = OutName = NULL;

/* flag defaults */
DoLRU = TuneConfig = DoAllCoders = FindBestCoder = WantsHelp = Debug = 0; //false
CoderNum = 9;
PreconditionName = NULL;
DeCompareName = NULL;
DoRunTransform = 1;
DoLZP = 0;

for(i=1;i<argc;i++)
  {
  if ( argv[i][0] == '-' )
    {
    switch(argv[i][1])
      {
      case '?':
      case 'h':
      case 'H':
        WantsHelp = 1;
        break;

      case 'a':
      case 'A':
        errputs("option: do all coders, make no output");
        DoAllCoders = 1;
        break;

      case 'b':
      case 'B':
        errputs("option: try all coders, encode with best");
        FindBestCoder = 1;
        break;

      case 'c':
      case 'C':
        if ( toupper(argv[i][2]) == 'Z' )
          {
          DoLZP = true;
          errputs("option: LZP coder");
          }
        else
          {
          int CoderNumInt = atoi(&argv[i][2]);
          if ( (CoderNumInt < 0 || CoderNumInt > CoderNum_Max ||
                ( CoderNumInt == 0 && argv[i][2] != '0' )) )
            CleanUp("CoderNum out of valid range");
          CoderNum = CoderNumInt;
          fprintf(stderr,"option: coder number %d, order %d\n",CoderNum,PPMcoder_OrderList[CoderNum]);
          }
        break;

      case 'f':
      case 'F':
        PreconditionName = &argv[i][2];
        fprintf(stderr,"option: precondition with %s\n",PreconditionName);
        break;

      case 't':
      case 'T':
	TuneConfig = true;
	errputs("Tuning Config.. will take a while!");
//        tune_param = atol(&argv[i][2]);
//        fprintf(stderr,"option: tune_param = %d\n",tune_param);
        break;

      case 'l':
      case 'L':
        {
        int T;
        DoLRU = 1;
        fprintf(stderr,"option: LRU node limit \n");
        }
        break;

      case 'd':
      case 'D':
        Debug = 1;
        errputs("option: debug");
		if ( argv[i][2] > 32 ) {
	        DeCompareName = &argv[i][2];
    	    fprintf(stderr,"option: decompare with %s\n",DeCompareName);
		}
        break;

      case 'r':
      case 'R':
        DoRunTransform = 0;
        errputs("option: no run-transform");
        break;

      default:
        fprintf(stderr,"Unknown flag: %c\n",argv[i][1]);
        break;
      }
    }
  else
    {
    if ( ! InName ) InName = argv[i];
    else if ( ! OutName ) OutName = argv[i];
    else fprintf(stderr,"Extra arguments: %s ignored\n",argv[i]);
    }
  }

if ( ! InName ) WantsHelp = 1;

if ( WantsHelp )
  {
  errputs("Usage : PPMZ [flags] <infile> <outfile>");
  errputs(" decoding done automatically when <infile> is packed");
  errputs("------------------------------------------------------");
  errputs("Flags: ( precede with a - or / )");
  errputs(" -c<#>    : use PPM coder <#>");
  errputs(" -cZ      : use LZP coder (on top of PPM)");
  errputs(" -f<name> : pre-condition PPM with file <name>");
  errputs(" -l[#]    : limit memory use with LRU, optional # of nodes");
  errputs(" -r       : don't do runtransform");
  errputs(" -b       : try all coders, encode with best");
  errputs(" -d[name] : debug (for testing) (with decompare optional)");
  errputs(" -a       : run all coders, produces no output (for testing)");
  errputs(" -t<#>    : set tune_param to # (for testing)");
  errputs("Note: most flags apply to Encoder only");
  errputs("------------------------------------------------------");
  errputs("press a key more more."); getchar();
  errputs("------------------------------------------------------");
  errputs("PPM coders:");
  for(i=0;i<=CoderNum_Max;i++)
    {
    fprintf(stderr," %2d : ",i);
    if ( PPMcoder_OrderList[i] == 88 )
      fprintf(stderr,"LOE");
    else
      fprintf(stderr,"order %d",PPMcoder_OrderList[i]);
    if ( i == CoderNum ) fprintf(stderr," (default)");
    fprintf(stderr,"\n");
    }
  errputs("LOE stands for Local Order Estimation, 8 >= Order >= 1");
  exit(10);
  }

/** initialize config values **/
{
	configTune *curTune;
	for(curTune = configTable;curTune->name != NULL;curTune++) {
		*(curTune->paramPtr) = curTune->best;
	}
}
/** **/

if ( (RawFP = fopen(InName,"rb")) == NULL )
  CleanUp("Error opening input file");

if ( PreconditionName )
  {
  if ( (PrecondFP = fopen(PreconditionName,"rb")) == NULL )
    CleanUp("Error opening precondition file");

  fseek(PrecondFP,0,SEEK_END);
  PrecondArrayLen = ftell(PrecondFP);
  fseek(PrecondFP,0,SEEK_SET);

  /* <> PrecondArray needs pre-padding */

  if ( (PrecondArray = malloc(PrecondArrayLen)) == NULL )
    CleanUp("malloc failed on Precondition!");

  FRead(PrecondFP,PrecondArray,PrecondArrayLen);

  fclose(PrecondFP); PrecondFP = NULL;
  }

if ( TuneConfig || DoAllCoders ) {

	/** not a real run **/

	  fseek(RawFP,0,SEEK_END);
	  RawFileLen = ftell(RawFP);
	  fseek(RawFP,0,SEEK_SET);
  
	  ArithArrayLen = RawFileLen + 200000; /* be wary of expansion */

	  fprintf(stderr,"Compressing %s from %ld bytes...\n",InName,RawFileLen);
  
	  if ( (RawArrayPadded = malloc(RawFileLen+RAWARRAY_PREPAD+ARRAY_POSTPAD)) == NULL )
    	CleanUp("AllocMem failed!");
	  RawArray = RawArrayPadded + RAWARRAY_PREPAD;

	/**/{
	    ubyte * tp;
	    for(tp=RawArrayPadded;tp<RawArray;tp++)
	      *tp = ( (tp - RawArrayPadded) ^ (13*(RawArray - tp)) ) & 0xFF;
	/**/}
  
	  if ( (ArithArray = malloc(ArithArrayLen+ARRAY_POSTPAD)) == NULL )
	    CleanUp("AllocMem failed!");
  
	  FRead(RawFP,RawArray,RawFileLen);

	  if ( DoRunTransform ) {
	    if ( ! RunPack(RawArray,RawFileLen,RawArray,&RT_LiteralArrayLen,ArithArray,&RT_NumRuns,&RT_RunPackedLen) )
	      CleanUp("RunTransform failed");
	  } else {
	    RT_LiteralArrayLen = RawFileLen;
	    RT_NumRuns = 0;
	    RT_RunPackedLen = 0;
	    }

  } /** tuneconfig or DoAllCoders **/

if ( TuneConfig ) {
	long OutFileLen,originalOutLen;
	configTune *curTune;
	int curStep,curVal,curBestVal,curBestLen;
	bool done;

	originalOutLen = 0;

	do {
		errputs("Another pass..");
		done = true;
		for(curTune = configTable;curTune->name != NULL;curTune++) {

			if ( curTune->high != curTune->low ) {

				curStep = max((curTune->high - curTune->low) / 4,curTune->minStep);

				/** seed with guessed best for sooner convergence **/

				curBestVal = curTune->best;
				*(curTune->paramPtr) = curTune->best;
			    curBestLen = PPMcoder_EncodeArray(RawArray,ArithArray,RT_LiteralArrayLen,
				      PPM_Writers[CoderNum],PPM_Inits[CoderNum],
			    	  PrecondArray,PrecondArrayLen,DoLRU);
				if ( originalOutLen == 0 ) {
					originalOutLen = curBestLen;
					fprintf(stderr,"initially : %s : %d -> %d = ",FilePart(InName),RawFileLen,curBestLen);
					TheCompressionIndicatorMin(RawFileLen,curBestLen+RT_RunPackedLen+1,stderr);
					errputs("");
				}

				for(curVal=curTune->low;curVal<=curTune->high;curVal += curStep) {
					if ( curVal != curBestVal ) { // don't run it twice!
						*(curTune->paramPtr) = curVal; // set it

					    OutFileLen = PPMcoder_EncodeArray(RawArray,ArithArray,RT_LiteralArrayLen,
					      PPM_Writers[CoderNum],PPM_Inits[CoderNum],
				    	  PrecondArray,PrecondArrayLen,DoLRU);

					    if ( !OutFileLen ) {
							errputs("PPMcoder_EncodeArray failed!");
							fprintf(stderr,"tuning: %s = %d\n",curTune->name,curVal);
						} else if ( OutFileLen < curBestLen ) {
							curBestLen = OutFileLen;
							curBestVal = curVal;
						}				
					}
				}

				if ( curTune->best != curBestVal )
					done = false;

				curTune->best = curBestVal;	
				*(curTune->paramPtr) = curBestVal; //other vals tuned with this guy at best
				curTune->low  = max(curTune->low , (curBestVal - ((curTune->high - curTune->low)>>2)) );
				curTune->high = min(curTune->high, (curBestVal + ((curTune->high - curTune->low)>>2)) );

				fprintf(stderr,"best for: %s = %d : at ",curTune->name,curBestVal);
				TheCompressionIndicatorMin(RawFileLen,curBestLen+RT_RunPackedLen+1,stderr);
				errputs("");
			}
		}
	} while( !done );

	errputs("done tuning! final report:");
	errputs("---------------------------------------");

	printf("Tuning %s improved ",FilePart(InName)); TheCompressionIndicatorMin(RawFileLen,originalOutLen+RT_RunPackedLen+1,stdout);
	printf(" to "); TheCompressionIndicatorMin(RawFileLen,curBestLen+RT_RunPackedLen+1,stdout);
	puts("");

	for(curTune = configTable;curTune->name != NULL;curTune++)
		printf("%s = %d\n",curTune->name,curTune->best);

  CleanUp(NULL);
  } /** tuneconfig **/
 
if ( DoAllCoders ) {
	long OutFileLen;

  printf("=====================\n");
  for(CoderNum=9;CoderNum<=CoderNum_Max;CoderNum++)
    {
    DiffClock = clock();

    OutFileLen = PPMcoder_EncodeArray(RawArray,ArithArray,RT_LiteralArrayLen,
      PPM_Writers[CoderNum],PPM_Inits[CoderNum],
      PrecondArray,PrecondArrayLen,DoLRU);

    DiffClock = clock() - DiffClock;

	if ( OutFileLen > RT_LiteralArrayLen ) OutFileLen = RT_LiteralArrayLen+1;
    
    printf("C%2d:O%2d: %-8s:",CoderNum,PPMcoder_OrderList[CoderNum],FilePart(InName));
    
    if ( !OutFileLen ) printf("PPMcoder_EncodeArray failed!");
    else TheCompressionIndicator(RawFileLen,OutFileLen+RT_RunPackedLen+1,stdout);

    printf(", %ld byps\n=====================\n", NumPerSec(RawFileLen,DiffClock) );
    }

/********
  printf("=====================\n");
  for(CoderNum=9;CoderNum<=CoderNum_Max;CoderNum++)
    {
    DiffClock = clock();

    OutFileLen = LZP_EncodeArray(RawArray,ArithArray,RT_LiteralArrayLen,
      PPM_Writers[CoderNum],PPM_Inits[CoderNum]);

    DiffClock = clock() - DiffClock;
    
    if ( !OutFileLen ) CleanUp("LZP_EncodeArray failed!");
    
    printf("LZP C%2d:O%2d: %-8s:",CoderNum,PPMcoder_OrderList[CoderNum],FilePart(InName));   
    TheCompressionIndicator(RawFileLen,OutFileLen+RT_RunPackedLen+1,stdout);
    printf(", %ld byps\n=====================\n", NumPerSec(RawFileLen,DiffClock) );
    }
***********/

  CleanUp(NULL);
  } /** doall coders**/

PPMZheader_Read(RawFP,&header);
if ( header.signature == PPMZ_Sig ) {
  errputs("Got PPMZ Signature in input file");
  if ( header.version == PPMZ_Version )
    {
    if ( header.revision != PPMZ_Revision )
      errputs("Encoded by same version, different revision : Ok");
    }
  else
    CleanUp("File encoded by different version. Not compatible.");

  GotDecompressFlag = 1;
} else
  GotDecompressFlag = 0;

if ( ! GotDecompressFlag ) {

  if ( ! OutName ) {
    fprintf(stderr,"No output specified, using %s\n",DEFAULT_OUT_ENC);
    OutName = DEFAULT_OUT_ENC;
    }

  if ( (ArithFP = fopen(OutName,"wb")) == NULL )
    CleanUp("Error creating output file");
  
  fseek(RawFP,0,SEEK_END);
  RawFileLen = ftell(RawFP);
  fseek(RawFP,0,SEEK_SET);

  ArithArrayLen = RawFileLen*5/4; /* be wary of expansion */
} else {
  int startOff;

  ArithFP = RawFP;

  if ( ! OutName ) {
    fprintf(stderr,"No output specified, using %s\n",DEFAULT_OUT_DEC);
    OutName = DEFAULT_OUT_DEC;
    }

  if ( (RawFP = fopen(OutName,"wb")) == NULL )
    CleanUp("Error creating output file");

  fseek(ArithFP,0,SEEK_SET);
  PPMZheader_Read(ArithFP,&header);
  startOff = ftell(ArithFP);
  fseek(ArithFP,0,SEEK_END);
  ArithArrayLen = ftell(ArithFP) - startOff;
  fseek(ArithFP,startOff,SEEK_SET);

  /* header */

  CoderNum         =  header.coderNum     ;
  RawFileLen       =  header.rawLen       ;
  ReadCRC          =  header.crc          ;

  DoLZP = (header.flags & HeaderFlag_DoLZP);

  DoRunTransform = (header.flags & HeaderFlag_DoRunTransform);
  if ( DoRunTransform )
    {
    RT_LiteralArrayLen  =  header.literalLen   ;
    RT_RunPackedLen     =  header.runPackedLen ;
    RT_NumRuns          =  header.numRuns      ;
    }
  else
    {
    RT_LiteralArrayLen = RawFileLen;
    RT_NumRuns = 0;
    RT_RunPackedLen = 0;
    }

  DoLRU            = (header.flags & HeaderFlag_DoLRU);

  if ( (header.flags & HeaderFlag_Preconditioned) && !PreconditionName )
    CleanUp(" File was encoded with a precoditioning file.\n You must supply the same file to the decoder");
  
  if ( CoderNum > CoderNum_Max && CoderNum != CODERNUM_RAW )
    CleanUp("CoderNum out of valid range");

  if ( DoLZP ) fprintf(stderr,"LZP : ");

  fprintf(stderr,"Decompressing file: %s from %ld to %ld bytes with coder %d ...\n",
    FilePart(InName),ArithArrayLen+1,RawFileLen,CoderNum);

  PPMPackedLen = ArithArrayLen - RT_RunPackedLen;

	if ( DeCompareName ) {
		int DeCompareArrayLen;

		if ( (DeCompareFP = fopen(DeCompareName,"rb")) == NULL )
			CleanUp("Error opening DeCompare file");

		fseek(DeCompareFP,0,SEEK_END);
		DeCompareArrayLen = ftell(DeCompareFP);
		fseek(DeCompareFP,0,SEEK_SET);

		if ( (DeCompareArray = malloc(DeCompareArrayLen)) == NULL )
			CleanUp("malloc failed on DeCompare!");

		FRead(DeCompareFP,DeCompareArray,DeCompareArrayLen);

		fclose(DeCompareFP); DeCompareFP = NULL;

		if ( DoRunTransform ) {
			ubyte * TempArray;

			if ( ! (TempArray = malloc(header.runPackedLen + 100)) )
				CleanUp("malloc failed!");

			if ( ! RunPack(DeCompareArray,DeCompareArrayLen,DeCompareArray,
				&RT_LiteralArrayLen,TempArray,&RT_NumRuns,&RT_RunPackedLen) )	// temp store
			      CleanUp("RunTransform failed");
			free(TempArray);
		}

	} // decompare

  } // decode

if ( (RawArrayPadded = malloc(RawFileLen+RAWARRAY_PREPAD+ARRAY_POSTPAD)) == NULL )
  CleanUp("AllocMem failed!");
RawArray = RawArrayPadded + RAWARRAY_PREPAD;

/**/{
    ubyte * tp;
    for(tp=RawArrayPadded;tp<RawArray;tp++)
      *tp = ( (tp - RawArrayPadded) ^ (13 * (RawArray - tp)) ) & 0xFF;
/**/}

if ( (ArithArray = malloc(ArithArrayLen+ARRAY_POSTPAD)) == NULL )
  CleanUp("AllocMem failed!");

if ( ! GotDecompressFlag )
  {
  long OutFileLen;

  FRead(RawFP,RawArray,RawFileLen);
  MadeCRC = crc32(RawArray,RawFileLen);

  RunArray = ArithArray;

  errputs("Compressing...");

  if ( DoRunTransform )
    {
    if ( ! RunPack(RawArray,RawFileLen,RawArray,&RT_LiteralArrayLen,
        RunArray,&RT_NumRuns,&RT_RunPackedLen) )
      CleanUp("RunTransform failed");

    fprintf(stderr,"RunTransform: %ld -> %ld literals & %ld runs ( -> %ld packed )\n",
      RawFileLen,RT_LiteralArrayLen,RT_NumRuns,RT_RunPackedLen);
    }
  else
    {
    RT_LiteralArrayLen = RawFileLen;
    RT_NumRuns = 0;
    RT_RunPackedLen = 0;
    }

  PPMArray = ArithArray + RT_RunPackedLen;

  if ( FindBestCoder )
    {
    long CurOutLen,BestOutLen = LONG_MAX;
    int BestCoder = 0;
  
    errputs("=====================");
    for(CoderNum=0;CoderNum<=CoderNum_Max;CoderNum++)
      {
      CurOutLen = PPMcoder_EncodeArray(RawArray,PPMArray,RT_LiteralArrayLen,
        PPM_Writers[CoderNum],PPM_Inits[CoderNum],
        PrecondArray,PrecondArrayLen,DoLRU);

		if ( CurOutLen > RT_LiteralArrayLen )
			CurOutLen = RT_LiteralArrayLen + 1;	

      printf("PPM C%2d:O%2d: %-8s:",CoderNum,PPMcoder_OrderList[CoderNum],FilePart(InName));  
      if ( !CurOutLen ) fprintf(stderr,"PPMcoder_EncodeArray failed!");
      else TheCompressionIndicator(RawFileLen,CurOutLen+RT_RunPackedLen+1,stdout);
		puts("");
  
      if ( CurOutLen>0 && CurOutLen < BestOutLen ) {
        BestOutLen = CurOutLen;   BestCoder = CoderNum;
        DoLZP = 0;
        }

      }

/*********
    errputs("=====================");
    for(CoderNum=0;CoderNum<=CoderNum_Max;CoderNum++)
      {
      CurOutLen = LZP_EncodeArray(RawArray,PPMArray,RT_LiteralArrayLen,PPM_Writers[CoderNum],PPM_Inits[CoderNum]);

      printf("LZP C%2d:O%2d: %-8s:",CoderNum,PPMcoder_OrderList[CoderNum],FilePart(InName));  
      if ( !CurOutLen ) fprintf(stderr,"LZPcoder_EncodeArray failed!");
      else TheCompressionIndicator(RawFileLen,CurOutLen+RT_RunPackedLen+1,stdout);
		puts("");
   
      if ( CurOutLen>0 && CurOutLen < BestOutLen ) {
        BestOutLen = CurOutLen;   BestCoder = CoderNum;
        DoLZP = 1;
        }
    
      }
    errputs("=====================");
***********/

    fprintf(stderr,"Coder %d chosen\n",BestCoder);

    CoderNum = BestCoder; //DoLZP set on or off
    }

  /* header */

  header.signature = PPMZ_Sig;
  header.version = PPMZ_Version;
  header.revision = PPMZ_Revision;
  header.coderNum = CoderNum;
  header.rawLen = RawFileLen;
  header.crc = MadeCRC;
  header.flags = 0;

  if ( DoLZP )
    header.flags |= HeaderFlag_DoLZP;

  if ( DoRunTransform )
    {
    header.flags |= HeaderFlag_DoRunTransform;
    header.literalLen = RT_LiteralArrayLen;
    header.runPackedLen = RT_RunPackedLen;
    header.numRuns = RT_NumRuns;
    }
  else
    {
    header.literalLen = RawFileLen;
    header.runPackedLen = 
      header.numRuns = 0;
    }

  if ( PreconditionName ) header.flags |= HeaderFlag_Preconditioned;
  if ( DoLRU ) header.flags |= HeaderFlag_DoLRU;

  PPMZheader_Write(ArithFP,&header);

  DiffClock  = clock();

  if ( DoLZP )
    {
    PPMPackedLen = LZP_EncodeArray(
      RawArray,PPMArray,RT_LiteralArrayLen,
      PPM_Writers[CoderNum],PPM_Inits[CoderNum]);
    }
  else
    {
    PPMPackedLen = PPMcoder_EncodeArray(
      RawArray,PPMArray,RT_LiteralArrayLen,
      PPM_Writers[CoderNum],PPM_Inits[CoderNum],
      PrecondArray,PrecondArrayLen,DoLRU );
    }

  DiffClock  = clock() - DiffClock;

  if ( !PPMPackedLen ) CleanUp("PPMcoder_EncodeArray failed!");

	if ( PPMPackedLen > RT_LiteralArrayLen ) {
		header.coderNum = CODERNUM_RAW;
		fseek(ArithFP,0,SEEK_SET);
		PPMZheader_Write(ArithFP,&header);
		PPMPackedLen = RT_LiteralArrayLen;
		memcpy(PPMArray,RawArray,RT_LiteralArrayLen);
	}

  if ( RT_RunPackedLen > 0 ) FWrite(ArithFP,RunArray,RT_RunPackedLen);
  FWrite(ArithFP,PPMArray,PPMPackedLen);

  OutFileLen = PPMPackedLen + RT_RunPackedLen;

  printf("%-12s:",FilePart(InName));
  TheCompressionIndicator(RawFileLen,OutFileLen,stdout);
  }
else
  {
  bool Ok;

  RunArray = ArithArray;
  PPMArray = ArithArray + RT_RunPackedLen;

	if ( RT_RunPackedLen > 0 )
		FRead(ArithFP,RunArray,RT_RunPackedLen);

	if ( CoderNum == CODERNUM_RAW ) {
		FRead(ArithFP,RawArray,RT_LiteralArrayLen);
	} else {
		FRead(ArithFP,PPMArray,PPMPackedLen);

		/* make sure PPMArray is zero-padded */
		PPMArray[PPMPackedLen] = 0;
		PPMArray[PPMPackedLen+1] = 0;

		DiffClock  = clock();
		if ( DoLZP )	{
			Ok = LZP_DecodeArray(RawArray,PPMArray,RT_LiteralArrayLen,
			  PPM_Readers[CoderNum],PPM_Inits[CoderNum]);
		} else	{
			Ok = PPMcoder_DecodeArray(RawArray,PPMArray,RT_LiteralArrayLen,
			  PPM_Readers[CoderNum],PPM_Inits[CoderNum],
			  PrecondArray,PrecondArrayLen,DoLRU,DeCompareArray);
		}
		DiffClock  = clock() - DiffClock;

		if ( !Ok ) CleanUp("PPMcoder_DecodeArray failed!");
	}

  if ( DoRunTransform )
    {
    ubyte * UnRTRawArray;
  
    if ( (UnRTRawArray = malloc(RawFileLen+ARRAY_POSTPAD)) != NULL )
      {
      if ( ! UnRunPack(UnRTRawArray,RawFileLen,RawArray,RunArray) )
        CleanUp("UnRunTransform failed");

      free(RawArrayPadded);
      RawArray = RawArrayPadded = UnRTRawArray;
      }
    else
      {
      CleanUp("malloc failed");
      }
    }

  FWrite(RawFP,RawArray,RawFileLen);

  MadeCRC = crc32(RawArray,RawFileLen);

  if ( ReadCRC != MadeCRC )
    { errputs("Failed CRC32 check!"); }
  else
    { errputs("CRC32 checked Ok"); }

  printf("%-12s:",FilePart(InName));
  TheDecompressionIndicator(RawFileLen,ArithArrayLen,stdout);
  }

printf(", %ld byps\n", NumPerSec(RawFileLen,DiffClock) );

CleanUp(NULL);
}

void CleanUp(char * ExitMess)
{

smartfree(RawArrayPadded);
smartfree(ArithArray);
smartfree(PrecondArray);
smartfree(DeCompareArray);
smartfree(DebugRawArrayPadded);
if ( PrecondFP  ) fclose(PrecondFP);
if ( DeCompareFP  ) fclose(DeCompareFP);
if ( RawFP  ) fclose(RawFP);
if ( ArithFP ) fclose(ArithFP);

if ( ExitMess )
  {
  fprintf(stderr," Error : %s\n",ExitMess);
  exit(10);
  }
else
  {
  fprintf(stderr,"Standard Exit.\n");
  exit(0);
  }
}
