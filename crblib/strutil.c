#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <crblib/strutil.h>

char * skipwhitespace(char * str)
{
	while( *str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' )
		str++;

return str;
}

#define Apostrophe ((char)39)	// the ''' char	

char * nexttok(char *str)	/** modifies str! **/
{
	for(;;) {
		switch( *str ) {
			case '"':
				str++;
				while(*str != '"') { if ( *str == 0 ) return NULL; str++; }
				break;
			case Apostrophe:		
				str++;
				while(*str != Apostrophe) { if ( *str == 0 ) return NULL; str++; }
				break;	
			case 0:
				return NULL;
			case ' ': case '\n': case '\t':
				*str++ = 0;
				while(*str<=32 && *str>0) *str++ = 0;
				return str;
			default:
				break;
		}
		str++;
	}
}





char * find_numeric(char *str)
{
	for(;;) {
		if ( isdigit(*str) ) return str;
		if( *str++ == 0 ) return NULL;
	}
}

void strcommas(char *str,ulong num)
{
int cnt;
char * strbase = str;

*str++ = (char)('0' + num%10); num /= 10;
cnt=1;

while(num)
  {
  if ( cnt == 3 ) { *str++ = ','; cnt = 0; }
	*str++ = (char)('0' + num%10); num /= 10;
	cnt++;
  }
*str = 0;

strrev(strbase);
}

void strrep(unsigned char *str,char fm,char to)
{
while( *str )
  {
  if ( *str == fm ) *str = to;
  str++;
  }
}

#ifndef IN_COMPILER
int stricmp(const char *str,const char *vs)
{

while( toupper(*str) == toupper(*vs) )
  {
  if ( ! *str ) return(0);
  str++; vs++;
  }

return( toupper(*str) - toupper(*vs) );
}
#endif

char * strichr(const char *str,short c)
{
short oc;

if (isupper(c)) oc = tolower(c);
else oc = toupper(c);

while ( *str )
  {
  if ( *str == c || *str == oc) return((char *)str);
  str++;
  }

return(NULL);
}

char * strnchr(const char *str,short c,int len)
{
register char *donestr;

donestr=(char *)((int)str + len);

while (str < donestr)
  {
  if (*str == c) return((char *)str);
  str++;
  }

return(NULL);
}

char * stristr(const char *StrBase,const char *SubBase)
{

while ( *StrBase )
  {
  if ( toupper(*StrBase) == toupper(*SubBase) )
	 {
	 const char * Str,* Sub;
	 Str = StrBase + 1;
	 Sub = SubBase + 1;
	 while ( *Sub && toupper(*Sub) == toupper(*Str) )
		{
		Sub++; Str++;
		}
	 if ( ! *Sub) return((char *)StrBase);
	 }
  StrBase++;
  }

return(NULL);
}

void strtolower(char *str)
{
while(*str)
  {
  *str = tolower(*str);
  str++;
  }
}

void strins(char *to,const char *fm)
{
int tolen = strlen(to);
int fmlen = strlen(fm);
char *newto,*oldto;

newto = to+fmlen+tolen; oldto = to+tolen;
tolen++;
while(tolen--) *newto-- = *oldto--;

while(fmlen--) *to++ = *fm++;

}

/* Count how many bytes we see from the start of the pointer until we reach
	a zero byte. This is like strlen(), but typed for ubyte memory.
*/
int blocklen(ubyte *ptr) 
{
	int count = 0;

	while(*ptr++ != 0x00) {
		count++;
	}

	return count;
}

/* Copy the ubyte region delimited by a 0x00 byte from src into dst,
	including the terminating 0x00 byte. This is like strcpy(), but typed
	for ubyte memory.
*/
ubyte* blockcpy(void *vdst, void *vsrc) 
{
	ubyte *dst = (ubyte*)vdst;
	ubyte *src = (ubyte*)vsrc;

	while((*dst++ = *src++));

	return (ubyte*)vdst; /* the original vdst pointer */
}

/* copy the first len bytes, or up to the first 0x00 from srcinto dest.
	Much like strncpy, don't terminate the dst with 0x00 if it is not found
	within the first len bytes.
*/
ubyte* blockncpy(void *vdst, void *vsrc, int len)
{
	ubyte *dst = (ubyte*)vdst;
	ubyte *src = (ubyte*)vsrc;
	int i;

	for (i = 0; i < len && *src != 0x00; i++) {
		*dst++ = *src++;
	}

	return (ubyte*)vdst; /* the original vdst pointer */
}

/* similar in function to strchr(), but for ubyte spaces */
ubyte* blockchr(ubyte *s, unsigned int c)
{
    while(*s != 0x00) {
        if (c == *s) {
            return s;
        }
        s++;
    }

    /* Check to see if they wanted the terminator itself */
    if (c == *s) {
        return s;
    }

    return NULL;
}


/* With 0x00 being the end of memory sentinel, compare the blocks of
	memory as ubytes similarly to how strcmp() does the work.
	Take care to not beyond the sentinel in any array.
*/
int blockcmp(void *vs1, void *vs2)
{
	ubyte *s1 = (ubyte*)vs1;
	ubyte *s2 = (ubyte*)vs2;
	
	while(*s1 && *s2) {
		if (*s1 < *s2) {
			return -1;
		}
		if (*s1 > *s2) {
			return 1;
		}
		s1++;
		s2++;
	}
	if (*s1 == 0x00 && *s2 != 0x00) {
		return -1;
	}
	if (*s2 == 0x00 && *s1 != 0x00) {
		return 1;
	}

	/* so s1 and s2 must be 0x00, meaning they were the same length.
		since we didn't find any problems before, they must also be equal.
	*/
	return 0;
}

/* With 0x00 being the end of a block of memory, perform a comparison up
	to the sentinel or len. Similar to strncmp().
*/
int blockncmp(void *vs1, void *vs2, int len)
{
	ubyte *s1 = (ubyte*)vs1;
	ubyte *s2 = (ubyte*)vs2;
	int index = 0;
	
	while(*s1 && *s2 && index < len) {
		if (*s1 < *s2) {
			return -1;
		}
		if (*s1 > *s2) {
			return 1;
		}
		s1++;
		s2++;
		index++;
	}

	if (index == len) {
		/* we didn't find a disparity, but ran out of bytes to check due
			processing all of the len bytes.
		*/
		return 0;
	} 

	/* if index < len, then we need to check the boundary conditions of the
		memory blocks, maybe one or both blocks was much shorter than len */

	if (*s1 == 0x00 && *s2 != 0x00) {
		return -1;
	}
	if (*s2 == 0x00 && *s1 != 0x00) {
		return 1;
	}

	/* so s1 and s2 must be 0x00, meaning they were the same length.
		since we didn't find any problems before, they must also be equal.
	*/
	return 0;
}

#ifndef IN_COMPILER
char *strupr(char *str)
{
char *strbase =str;
while(*str)
  {
  *str = toupper(*str);
  str++;
  }
return(strbase);
}

char * strrev(char *str)
{
register char *endstr,swapper;
char *strbase =str;

endstr = str;
while(*endstr) endstr++;
endstr--;

while ( endstr > str )
  {
  swapper = *str;
  *str++ = *endstr;
  *endstr-- = swapper;
  }

return(strbase);
}
#endif


char * getParam(char *arg,char *argv[],int *iPtr)
{
	while ( *arg == '-' || *arg == '/' || *arg == '=' )
	{
		arg++;
		arg = skipwhitespace(arg);
		if ( ! *arg )
		{
			(*iPtr) ++;
			arg = argv[*iPtr];
			if ( ! arg )
				return NULL;
		}
	}
return arg;
}
