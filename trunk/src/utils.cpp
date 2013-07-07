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
#include <algorithm>

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

static void __for_each_file(const std::string& dirname, const std::string& suff,
                            const std::list<std::string>& order_list, const std::list<std::string>& disable_list, 
                            const std::function<void (const std::string&, bool)>& f)
{
	GDir *dir = g_dir_open(dirname.c_str(), 0, nullptr);	
    if (dir) {
		const gchar *filename;

        while ((filename = g_dir_read_name(dir))!=nullptr) {	
			const std::string fullfilename(dirname+G_DIR_SEPARATOR_S+filename);
			if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR))
				__for_each_file(fullfilename, suff, order_list, disable_list, f);
            else if (g_str_has_suffix(filename, suff.c_str()) &&
                     std::find(order_list.begin(), order_list.end(), 
                               fullfilename)==order_list.end()) { 
                const bool disable = std::find(disable_list.begin(), 
                                         disable_list.end(), 
                                         fullfilename)!=disable_list.end();
                f(fullfilename, disable);
			}
		}
		g_dir_close(dir);
	}
}


void for_each_file(const std::list<std::string>& dirs_list, const std::string& suff,
                   const std::list<std::string>& order_list, const std::list<std::string>& disable_list, 
                   const std::function<void (const std::string&, bool)>& f)
{
	for (const std::string & item : order_list) {
		const bool disable = std::find(disable_list.begin(), disable_list.end(), item) != disable_list.end();
		f(item, disable);
	}
	for (const std::string& item : dirs_list)
		__for_each_file(item, suff, order_list, disable_list, f);			
}
