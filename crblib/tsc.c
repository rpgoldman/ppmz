
#include <crblib/inc.h>
#include <time.h>

#include "tsc.h"

#ifdef _MSC_VER //{

void readTSC(ulong *hi)
{
ulong *lo = hi + 1;
	__asm 
	{
		rdtsc
		mov EBX,hi
		mov DWORD PTR [EBX],EDX
		mov EBX,lo
		mov DWORD PTR [EBX],EAX
	}
	return;
}

double diffTSChz(ulong *tsc1,ulong *tsc2)
{

 return	(tsc2[0] - tsc1[0])*4294967296.0 +
		(double)(tsc2[1] - tsc1[1]);

}

double readTSChz(void)
{
tsc_type tsc;
	readTSC(tsc);
return	(tsc[0]*4294967296.0 + (double)tsc[1]);
}

#else // }{ PC

void readTSC(ulong *hi)
{
	hi[0] = clock();
	hi[1] = 0;
}
double diffTSChz(ulong *tsc1,ulong *tsc2)
{
	return tsc2[0] - tsc1[0]; 
}

double readTSChz(void)
{
	return	clock();
}

#endif // } PC


typedef struct tscNode tscNode;
struct tscNode 
{
	tscNode *next;
	ulong tsc[2];
};
tscNode * head = NULL;

void pushTSC(void)
{
tscNode *tn;
	tn = new(tscNode);
	if ( !tn ) return;
	tn->next = head;
	head = tn;
	readTSC(tn->tsc);
}

double popTSC(void)
{
tscNode *tn;
ulong tsc[2];
	readTSC(tsc);
	if ( ! head ) return 0.0;
	tn = head;
	head = head->next;
return diffTSC(tn->tsc,tsc);
}

double popTSChz(void)
{
tscNode *tn;
ulong tsc[2];
	readTSC(tsc);
	if ( ! head ) return 0.0;
	tn = head;
	head = head->next;
return diffTSChz(tn->tsc,tsc);
}

double diffTSC(ulong *tsc1,ulong *tsc2)
{
return diffTSChz(tsc1,tsc2) * secondsPerTSC();
}

void showPopTSC(const char *tag,FILE * fp)
{
double time;

	time = popTSC();

	showTSC(tag,fp,time);
}

void showTSC(const char *tag,FILE * fp,double time)
{	
	fprintf(fp,"%s : %f secs\n",tag,time);
}

void showPopTSCper(FILE * fp,const char *tag,int items,const char *itemTag)
{
double time;

	time = popTSC();
	showTSCper(fp,tag,items,itemTag,time);
}

void showTSCper(FILE * fp,const char *tag,int items,const char *itemTag,double time)
{
double hz,per;

	hz = time * tscMHZ() * 1000000;
	per = time/(double)items;
	
	fprintf(fp,"%s : %f secs = %2.1f cycles / %s = ",tag,time,hz/items,itemTag);

	if ( per < 0.0001 ) 
	{
		fprintf(fp,"%d %ss /sec\n",(int)(1.0/per),itemTag);
	}
	else
	{
		fprintf(fp,"%f %ss /sec\n",(1.0/per),itemTag);
	}
}

static double secondsPerClock = 0.0f;
static bool haveSecondsPerClock = false;
static int TSC_MHZ = 0;

double secondsPerTSC(void)
{
clock_t clock1;
tsc_type tsc1,tsc2;
double clocksPerSec,MHZ;

	if ( haveSecondsPerClock )
	{
		return secondsPerClock;
	}

	do
	{

		clock1 = clock();
		readTSC(tsc1);
		while( (clock() - clock1) < CLOCKS_PER_SEC ) ;
		readTSC(tsc2);
		clocksPerSec = diffTSChz(tsc1,tsc2);

		MHZ = clocksPerSec / (1000000.0);

		// check for common errors
		{
		int Z50,Z83;
			Z50 = ((int)((MHZ + 25)/50))*50; // round to the nearest fifty
			Z83 = ((int)(((MHZ + 41)*12)/1000))*1000/12; // round to the nearest 83.333 == 1000/12
			if ( ABS((double)Z50 - MHZ) < 14 )
				MHZ = Z50;
			else if ( ABS((double)Z83 - MHZ) < 14 )
				MHZ = Z83;
			else
			{
				MHZ = ((int)((MHZ + 5)/10))*10; // round to the nearest ten
			}
			TSC_MHZ = (int) MHZ;
		}

	} while( TSC_MHZ > 1000 );

	clocksPerSec  = MHZ * (1000000.0);

	haveSecondsPerClock = true;
	secondsPerClock = 1.0f / clocksPerSec;

return secondsPerClock;
}

double timeTSC(void)
{
double time;
	time = readTSChz();
return time * secondsPerTSC();
}

int tscMHZ(void)
{
	secondsPerTSC();
return TSC_MHZ;
}
