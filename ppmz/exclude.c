#include <stdlib.h>
#include <limits.h>
#include <crblib/inc.h>
#include <crblib/memutil.h>

#define excludeType ubyte
#define resetVal 0x100

typedef struct exclusionStruct
  {
  int numChars;
  int counter;
  int tableLongLen;
  excludeType * table;
  } exclusion;


exclusion * newExclusion(int numChars);
void clearExclusion(exclusion * E);
void setExclude(exclusion * E,int sym);
bool isExcluded(exclusion * E,int sym);
void freeExclusion(exclusion * E);
void resetExclusion(exclusion * E);


exclusion * newExclusion(int numChars)
{
exclusion * E;
E = new(exclusion);
if ( E == NULL ) return(NULL);
E->tableLongLen = PaddedSize(sizeof(excludeType)*numChars);
if ( (E->table = malloc(E->tableLongLen*4)) == NULL )
  { free(E); return(NULL); }
E->numChars = numChars;
resetExclusion(E);
return(E);
}

void resetExclusion(exclusion * E)
{
int i;
MemClearFast(E->table,E->tableLongLen);
E->counter = 1;
}

extern void freeExclusion(exclusion * E)
{
free(E->table);
free(E);
}

void clearExclusion(exclusion * E)
{
E->counter ++;
if ( E->counter == resetVal ) resetExclusion(E);
}

void setExclude(exclusion * E,int sym)
{
E->table[sym] = E->counter;
}

bool isExcluded(exclusion * E,int sym)
{
return( (bool) ( E->table[sym] == E->counter ) );
}
 
