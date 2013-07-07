/*
 * This file part of sdcv - console version of Stardict program
 * http://sdcv.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <cstdlib>
#include <cstdio>

#include "utils.hpp"

std::string utf8_to_locale_ign_err(const std::string& utf8_str)
{
	std::string res;

	const char *charset;
	if (g_get_charset(&charset))
		res = utf8_str;
	else {
        gsize bytes_read, bytes_written;
        glib::Error err;
        glib::CharStr tmp(g_convert_with_fallback(utf8_str.c_str(), -1, charset, "UTF-8", nullptr,
                                                  &bytes_read, &bytes_written, get_addr(err)));
		if (nullptr == get_impl(tmp)){
			fprintf(stderr, _("Can not convert %s to current locale.\n"), utf8_str.c_str());
			fprintf(stderr, "%s\n", err->message);
			exit(EXIT_FAILURE);
		}
		res = get_impl(tmp);
	}

	return res;
}

char *locale_to_utf8(const char *loc_str)
{
	if (nullptr == loc_str)
		return nullptr;
	gsize bytes_read, bytes_written;
    glib::Error err;
	gchar *str = g_locale_to_utf8(loc_str, -1, &bytes_read, &bytes_written, get_addr(err));
	if (nullptr == str){
		fprintf(stderr, _("Can not convert %s to utf8.\n"), loc_str);
		fprintf(stderr, "%s\n", err->message);
		exit(EXIT_FAILURE);
	}

	return str;
}
