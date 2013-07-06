/*
    writer : Opera Wang
    E-Mail : wangvisual AT sohu DOT com
    License: GPL
*/

/* filename: distance.cc */
/*
http://www.merriampark.com/ld.htm
What is Levenshtein Distance?

Levenshtein distance (LD) is a measure of the similarity between two strings, 
which we will refer to as the source string (s) and the target string (t). 
The distance is the number of deletions, insertions, or substitutions required
 to transform s into t. For example,

    * If s is "test" and t is "test", then LD(s,t) = 0, because no transformations are needed. 
    The strings are already identical.
    * If s is "test" and t is "tent", then LD(s,t) = 1, because one substitution
     (change "s" to "n") is sufficient to transform s into t.

The greater the Levenshtein distance, the more different the strings are.

Levenshtein distance is named after the Russian scientist Vladimir Levenshtein,
 who devised the algorithm in 1965. If you can't spell or pronounce Levenshtein,
 the metric is also sometimes called edit distance.

The Levenshtein distance algorithm has been used in:

    * Spell checking
    * Speech recognition
    * DNA analysis
    * Plagiarism detection 
*/


#include <cstdlib>
#include <cstring>

#include "distance.h"

/*
Cover transposition, in addition to deletion,
insertion and substitution. This step is taken from:
Berghel, Hal ; Roach, David : "An Extension of Ukkonen's 
Enhanced Dynamic Programming ASM Algorithm"
(http://www.acm.org/~hlb/publications/asm/asm.html)
*/
#define COVER_TRANSPOSITION

/****************************************/
/*Implementation of Levenshtein distance*/
/****************************************/

/*Gets the minimum of three values */
static inline int minimum(const int a, const int b, const int c)
{
    int min = a;
    if ( b < min )
        min = b;
    if ( c < min )
        min = c;
    return min;
}

int EditDistance::CalEditDistance(const gunichar *s,const gunichar *t,const int limit)
/*Compute levenshtein distance between s and t, this is using QUICK algorithm*/
{
    int n=0,m=0,iLenDif,k,i,j,cost;
    // Remove leftmost matching portion of strings
    while ( *s && (*s==*t) )
    {
        s++;
		t++;
    }

	while (s[n])
	{
		n++;
	}
	while (t[m])
	{
		m++;
	}
	
    // Remove rightmost matching portion of strings by decrement n and m.
    while ( n && m && (*(s+n-1)==*(t+m-1)) )
    {
        n--;m--;
    }
    if ( m==0 || n==0 || d==nullptr )
        return (m+n);
    if ( m < n )
    {
        const gunichar * temp = s;
        int itemp = n;
        s = t;
        t = temp;
        n = m;
        m = itemp;
    }
    iLenDif = m - n;
    if ( iLenDif >= limit )
        return iLenDif;
    // step 1
    n++;m++;
//    d=(int*)malloc(sizeof(int)*m*n);
    if ( m*n > currentelements )
    {
        currentelements = m*n*2;    // double the request
        d = static_cast<int*>(realloc(d, sizeof(int) * currentelements));
        if ( nullptr == d )
            return (m+n);
    }
    // step 2, init matrix
    for (k=0;k<n;k++)
        d[k] = k;
    for (k=1;k<m;k++)
        d[k*n] = k;
    // step 3
    for (i=1;i<n;i++)
    {
        // first calculate column, d(i,j)
        for ( j=1;j<iLenDif+i;j++ )
        {
            cost = s[i-1]==t[j-1]?0:1;
            d[j*n+i] = minimum(d[(j-1)*n+i]+1,d[j*n+i-1]+1,d[(j-1)*n+i-1]+cost);
#ifdef COVER_TRANSPOSITION
            if ( i>=2 && j>=2 && (d[j*n+i]-d[(j-2)*n+i-2]==2)
                 && (s[i-2]==t[j-1]) && (s[i-1]==t[j-2]) )
                d[j*n+i]--;
#endif
        }
        // second calculate row, d(k,j)
        // now j==iLenDif+i;
        for ( k=1;k<=i;k++ )
        {
            cost = s[k-1]==t[j-1]?0:1;
            d[j*n+k] = minimum(d[(j-1)*n+k]+1,d[j*n+k-1]+1,d[(j-1)*n+k-1]+cost);
#ifdef COVER_TRANSPOSITION
            if ( k>=2 && j>=2 && (d[j*n+k]-d[(j-2)*n+k-2]==2)
                 && (s[k-2]==t[j-1]) && (s[k-1]==t[j-2]) )
                d[j*n+k]--;
#endif
        }
        // test if d(i,j) limit gets equal or exceed
        if ( d[j*n+i] >= limit )
        {
            return d[j*n+i];
        }
    }
    // d(n-1,m-1)
    return d[n*m-1];
}
