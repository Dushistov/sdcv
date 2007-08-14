#ifndef DISTANCE_H
#define DISTANCE_H

#include <glib.h>

class EditDistance {
private:
    int *d;
    int currentelements;
    /*Gets the minimum of three values */
    inline int minimum( const int a, const int b, const int c )
    {
        int min = a;
        if ( b < min )
              min = b;
        if ( c < min )
              min = c;
          return min;
    };
public:
    EditDistance(  );
    ~EditDistance(  );
    int CalEditDistance( const gunichar *s, const gunichar *t, const int limit );
};

#endif
