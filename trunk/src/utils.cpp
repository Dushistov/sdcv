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

#include "utils.hpp"

bool stdio_getline(FILE *in, std::string & str)
{
  str.clear();
  int ch;
  while((ch=fgetc(in))!=EOF && ch!='\n')
    str+=ch;
  if(EOF==ch)
    return false;
  return true;
}

std::string utf8_to_locale_ign_err(const std::string& utf8_str)
{
	gsize bytes_read, bytes_written;
	GError *err=NULL;
	std::string res;
  
	const char * charset;
	if (g_get_charset(&charset))
		res=utf8_str;
	else {
		gchar *tmp=g_convert_with_fallback(utf8_str.c_str(), -1, charset, "UTF-8", NULL, 
						   &bytes_read, &bytes_written, &err);
		if (NULL==tmp){
			fprintf(stderr, _("Can not convert %s to current locale.\n"), utf8_str.c_str());
			fprintf(stderr, "%s\n", err->message);
			g_error_free(err);
			exit(EXIT_FAILURE);
		}
		res=tmp;
		g_free(tmp);
	}

	return res;
}

char *locale_to_utf8(const char *loc_str)
{
	if(NULL==loc_str)
		return NULL;
	gsize bytes_read;
	gsize bytes_written;
	GError *err=NULL;
	gchar *str=NULL;
	str=g_locale_to_utf8(loc_str, -1, &bytes_read, &bytes_written, &err);
	if(NULL==str){
		fprintf(stderr, _("Can not convert %s to utf8.\n"), loc_str);
		fprintf(stderr, "%s\n", err->message);
		g_error_free(err);
		exit(EXIT_FAILURE);
	}

	return str;
}

