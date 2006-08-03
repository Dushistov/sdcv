#ifndef __SD_LIB_H__
#define __SD_LIB_H__

#include "common.hpp"
#include "data.hpp"
#include "stddict.hpp"
#include "treedict.hpp"

static inline gchar* stardict_datadup(gconstpointer mem)
{
        return (gchar *)g_memdup(mem, sizeof(guint32) + *reinterpret_cast<const guint32 *>(mem));
}

typedef enum {
	qtSIMPLE, qtREGEXP, qtFUZZY, qtDATA
} query_t;
	
extern query_t analyse_query(const char *s, std::string& res); 

#endif//!__SD_LIB_H__
