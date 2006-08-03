#include "my_global.h"
#include "m_ctype.h"

#include <string.h>
#include <glib.h>

#include "collation.h"

static CHARSET_INFO *get_cs(CollateFunctions func)
{
	CHARSET_INFO *cs;
	if (func == UTF8_GENERAL_CI)
		cs = &my_charset_utf8_general_ci;
	else if (func == UTF8_BIN)
		cs = &my_charset_utf8_bin;
	else if (func == UTF8_UNICODE_CI)
		cs = &my_charset_utf8_general_uca_ci;
	else if (func == UTF8_ICELANDIC_CI)
		cs = &my_charset_utf8_icelandic_uca_ci;
	else if (func == UTF8_LATVIAN_CI)
		cs = &my_charset_utf8_latvian_uca_ci;
	else if (func == UTF8_ROMANIAN_CI)
		cs = &my_charset_utf8_romanian_uca_ci;
	else if (func == UTF8_SLOVENIAN_CI)
		cs = &my_charset_utf8_slovenian_uca_ci;
	else if (func == UTF8_POLISH_CI)
		cs = &my_charset_utf8_polish_uca_ci;
	else if (func == UTF8_ESTONIAN_CI)
		cs = &my_charset_utf8_estonian_uca_ci;
	else if (func == UTF8_SPANISH_CI)
		cs = &my_charset_utf8_spanish_uca_ci;
	else if (func == UTF8_SWEDISH_CI)
		cs = &my_charset_utf8_swedish_uca_ci;
	else if (func == UTF8_TURKISH_CI)
		cs = &my_charset_utf8_turkish_uca_ci;
	else if (func == UTF8_CZECH_CI)
		cs = &my_charset_utf8_czech_uca_ci;
	else if (func == UTF8_DANISH_CI)
		cs = &my_charset_utf8_danish_uca_ci;
	else if (func == UTF8_LITHUANIAN_CI)
		cs = &my_charset_utf8_lithuanian_uca_ci;
	else if (func == UTF8_SLOVAK_CI)
		cs = &my_charset_utf8_slovak_uca_ci;
	else if (func == UTF8_SPANISH2_CI)
		cs = &my_charset_utf8_spanish2_uca_ci;
	else if (func == UTF8_ROMAN_CI)
		cs = &my_charset_utf8_roman_uca_ci;
	else if (func == UTF8_PERSIAN_CI)
		cs = &my_charset_utf8_persian_uca_ci;
	else if (func == UTF8_ESPERANTO_CI)
		cs = &my_charset_utf8_esperanto_uca_ci;
	else if (func == UTF8_HUNGARIAN_CI)
		cs = &my_charset_utf8_hungarian_uca_ci;
	else
		cs = NULL;
	return cs;
}

static GSList *my_once_root_block = NULL;

static void *my_once_alloc(uint size)
{
	void *mem = g_malloc(size);
	my_once_root_block = g_slist_prepend (my_once_root_block, mem);
	return mem;
}

static void my_once_free()
{
	GSList *list = my_once_root_block;
	while (list) {
		g_free(list->data);
		list = list->next;
	}
	g_slist_free(my_once_root_block);
}

int utf8_collate_init(CollateFunctions func)
{
	CHARSET_INFO *cs = get_cs(func);
	if (cs) {
		if ((cs->cset->init && cs->cset->init(cs, my_once_alloc)) ||
        		(cs->coll->init && cs->coll->init(cs, my_once_alloc)))
			return TRUE;
		else
			return FALSE;
	} else
		return TRUE;
}

int utf8_collate(const char *str1, const char *str2, CollateFunctions func)
{
	CHARSET_INFO *cs = get_cs(func);
	if (cs) {
		return cs->coll->strnncoll(cs, (const uchar*)str1, strlen(str1), (const uchar*)str2, strlen(str2), 0);
	} else
		return 0; //Should never happen.
}

void utf8_collate_end()
{
	my_once_free();
}
