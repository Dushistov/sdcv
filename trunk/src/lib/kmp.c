#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kmp.h"

static int* prefix = NULL;
static int max_size = 0;

static void GetPrefixValue(const char* strPattern, int iPatternLen)
{
		int i, j; /* i runs through the string, j counts the hits*/

    if (iPatternLen>max_size) {
        prefix = (int*)realloc(prefix, iPatternLen*sizeof(int));
        max_size = iPatternLen;
    }
   
    i = 1; j = 0;
    prefix[0] = 0;
   
    while(i<iPatternLen)
    {
        if(strPattern[i] == strPattern[j])
        {
            prefix[i] = ++j;
        }
        else
        {
            j = 0;
            prefix[i] = j;
        }
       
        i++;
    }
}

static int KMPStringMatch(const char* strPattern, int iPatternLen, const char* strTarget, int iTargetLen)
{
	int j =0;
	int i;
	for (i=0;i<iPatternLen;i++) {
		while ((strPattern[i] != strTarget[j]) && (j > 0))
			j = prefix[j-1];
		if (strPattern[i] == strTarget[j])
			j++;
		if (j == iTargetLen)
			return i - j + 1;
	}
	return -1;
}

int KMP(const char* strPattern, int len, const char* strTarget)
{
	GetPrefixValue(strPattern, len);
	return KMPStringMatch(strPattern, len, strTarget, strlen(strTarget));
}

void KMP_end()
{
	free(prefix);
	prefix=NULL;
	max_size=0;
}
