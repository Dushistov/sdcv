/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
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

/*
 * implementation of methods of common for dictionaries structures
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "common.hpp"

//looks not optimal, TODO: refactor
bool DictInfo::load_from_ifo_file(const std::string& ifofilename,
				  bool istreedict)
{
	ifo_file_name=ifofilename;
	gchar *buffer;
	if (!g_file_get_contents(ifofilename.c_str(), &buffer, NULL, NULL))
		return false;

#define TREEDICT_MAGIC_DATA "StarDict's treedict ifo file\nversion=2.4.2\n"
#define DICT_MAGIC_DATA "StarDict's dict ifo file\nversion=2.4.2\n"
	const gchar *magic_data=istreedict ? TREEDICT_MAGIC_DATA : DICT_MAGIC_DATA;
	if (!g_str_has_prefix(buffer, magic_data)) {
		g_free(buffer);
		return false;
	}

	gchar *p1,*p2,*p3;

	p1 = buffer + strlen(magic_data)-1;

	p2 = strstr(p1,"\nwordcount=");
	if (!p2) {
		g_free(buffer);
		return false;
	}

	p3 = strchr(p2+ sizeof("\nwordcount=")-1,'\n');
	gchar *tmpstr = (gchar *)g_memdup(p2+sizeof("\nwordcount=")-1, p3-(p2+sizeof("\nwordcount=")-1)+1);
	tmpstr[p3-(p2+sizeof("\nwordcount=")-1)] = '\0';
	wordcount = atol(tmpstr);
	g_free(tmpstr);

	p2 = strstr(p1,"\nsynwordcount=");
	if (p2) {
		p3 = strchr(p2+ sizeof("\nsynwordcount=")-1,'\n');
		gchar *tmpstr = (gchar *)g_memdup(p2+sizeof("\nsynwordcount=")-1, p3-(p2+sizeof("\nsynwordcount=")-1)+1);
		tmpstr[p3-(p2+sizeof("\nsynwordcount=")-1)] = '\0';
		synwordcount = atol(tmpstr);
		g_free(tmpstr);
	} else {
		synwordcount = 0;
	}

	if (istreedict) {
		p2 = strstr(p1,"\ntdxfilesize=");
		if (!p2) {
			g_free(buffer);
			return false;
		}
		p3 = strchr(p2+ sizeof("\ntdxfilesize=")-1,'\n');
		tmpstr = (gchar *)g_memdup(p2+sizeof("\ntdxfilesize=")-1, p3-(p2+sizeof("\ntdxfilesize=")-1)+1);
		tmpstr[p3-(p2+sizeof("\ntdxfilesize=")-1)] = '\0';
		index_file_size = atol(tmpstr);
		g_free(tmpstr);
	} else {

		p2 = strstr(p1,"\nidxfilesize=");
		if (!p2) {
			g_free(buffer);
			return false;
		}

		p3 = strchr(p2+ sizeof("\nidxfilesize=")-1,'\n');
		tmpstr = (gchar *)g_memdup(p2+sizeof("\nidxfilesize=")-1, p3-(p2+sizeof("\nidxfilesize=")-1)+1);
		tmpstr[p3-(p2+sizeof("\nidxfilesize=")-1)] = '\0';
		index_file_size = atol(tmpstr);
		g_free(tmpstr);
	}

	p2 = strstr(p1,"\nbookname=");

	if (!p2) {
		g_free(buffer);
		return false;
	}

	p2 = p2 + sizeof("\nbookname=") -1;
	p3 = strchr(p2, '\n');
	bookname.assign(p2, p3-p2);

	p2 = strstr(p1,"\nauthor=");
	if (p2) {
		p2 = p2 + sizeof("\nauthor=") -1;
		p3 = strchr(p2, '\n');
		author.assign(p2, p3-p2);
	}

	p2 = strstr(p1,"\nemail=");
	if (p2) {
		p2 = p2 + sizeof("\nemail=") -1;
		p3 = strchr(p2, '\n');
		email.assign(p2, p3-p2);
	}

	p2 = strstr(p1,"\nwebsite=");
	if (p2) {
		p2 = p2 + sizeof("\nwebsite=") -1;
		p3 = strchr(p2, '\n');
		website.assign(p2, p3-p2);
	}

	p2 = strstr(p1,"\ndate=");
	if (p2) {
		p2 = p2 + sizeof("\ndate=") -1;
		p3 = strchr(p2, '\n');
		date.assign(p2, p3-p2);
	}

	p2 = strstr(p1,"\ndescription=");
	if (p2) {
		p2 = p2 + sizeof("\ndescription=")-1;
		p3 = strchr(p2, '\n');
		description.assign(p2, p3-p2);
	}

	p2 = strstr(p1,"\nsametypesequence=");
	if (p2) {
		p2+=sizeof("\nsametypesequence=")-1;
		p3 = strchr(p2, '\n');
		sametypesequence.assign(p2, p3-p2);
	}

	g_free(buffer);

	return true;
}

