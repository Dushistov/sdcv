#pragma once

#include <cstdlib>
#include <glib.h>

class EditDistance
{
public:
    EditDistance()
    {
        currentelements = 2500; // It's enough for most conditions :-)
        d = static_cast<int *>(malloc(sizeof(int) * currentelements));
    }
    ~EditDistance()
    {
        if (d != nullptr)
            free(d);
    }
    EditDistance(const EditDistance &) = delete;
    EditDistance &operator=(const EditDistance &) = delete;
    int CalEditDistance(const gunichar *s, const gunichar *t, const int limit);

private:
    int *d;
    int currentelements;
};
