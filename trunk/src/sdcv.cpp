/* 
 * This file part of sdcv - console version of Stardict program
 * http://sdcv.sourceforge.net
 * Copyright (C) 2003-2006 Evgeniy <dushistov@mail.ru>
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

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <clocale>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <getopt.h>
#include <string>
#include <vector>
#include <memory>

#include "libwrapper.hpp"
#include "readline.hpp"
#include "utils.hpp"

const char gVersion[] = VERSION;


struct option longopts[] ={
	{"version", no_argument, NULL, 'v' },
	{"help", no_argument, NULL, 'h' },
	{"list-dicts", no_argument, NULL, 'l'},
	{"use-dict", required_argument, NULL, 'u'},
	{"non-interactive", no_argument, NULL, 'n'},
	{"utf8-output", no_argument, NULL, 0},
	{"utf8-input", no_argument, NULL, 1},
	{"data-dir", required_argument, NULL, 2},
	{ NULL, 0, NULL, 0 }
};

struct PrintDictInfo {
	void operator()(const std::string& filename, bool) {
		DictInfo dict_info;
		if (dict_info.load_from_ifo_file(filename, false)) {
			string bookname=utf8_to_locale_ign_err(dict_info.bookname);
			printf("%s    %d\n", bookname.c_str(), dict_info.wordcount);
		}
	}
};

struct CreateDisableList {
	CreateDisableList(const strlist_t& enable_list_, strlist_t& disable_list_) :
		enable_list(enable_list_), disable_list(disable_list_)
	{}
	void operator()(const std::string& filename, bool) {
		DictInfo dict_info;
		if (dict_info.load_from_ifo_file(filename, false) &&
		    std::find(enable_list.begin(), enable_list.end(), 
			      dict_info.bookname)==enable_list.end())
			disable_list.push_back(dict_info.ifo_file_name);		
	}
private:
	const strlist_t& enable_list;
	strlist_t& disable_list;
};

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
#if ENABLE_NLS
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif	 
	int optc;
	bool h = false, v = false, show_list_dicts=false, 
		use_book_name=false, non_interactive=false, 
		utf8_output=false, utf8_input=false;
	strlist_t enable_list;
	string data_dir;
	int option_index = 0;
	while ((optc = getopt_long (argc, argv, "hvu:ln", longopts, 
				    &option_index))!=-1)
		switch (optc){
		case 0:
			utf8_output=true;
			break;
		case 1:   
			utf8_input=true;
			break;
		case 2:
			data_dir=optarg;
			break;
		case 'v':
			v = true;
			break;
		case 'h':
			h = true;
			break;
		case 'l':
			show_list_dicts=true;
			break;
		case 'u':
			use_book_name=true;
			enable_list.push_back(locale_to_utf8(optarg));
			break;
		case 'n':
			non_interactive=true;
			break;
		case '?':
			fprintf(stderr, 
				_("Unknown option.\nTry '%s --help' for more information.\n"), 
				argv[0]);
			return EXIT_FAILURE;
		}
  
	if (h) {
		printf("sdcv - console version of StarDict.\n");
		printf(_("Usage: %s [OPTIONS] words\n"), argv[0]);
		printf(_("-h, --help               display this help and exit\n"));
		printf(_("-v, --version            display version information and exit\n"));
		printf(_("-l, --list-dicts         display list of available dictionaries and exit\n"));
		printf(_("-u, --use-dict bookname  for search use only dictionary with this bookname\n"));
		printf(_("-n, --non-interactive    for use in scripts\n"));
		printf(_("--utf8-output            output must be in utf8\n"));
		printf(_("--utf8-input             input of sdcv in utf8\n"));
		printf(_("--data-dir path/to/dir   use this directory as path to stardict data directory\n"));

		return EXIT_SUCCESS;
	}

	if (v) {
		printf(_("Console version of Stardict, version %s\n"), gVersion);
		return EXIT_SUCCESS;
	}

	const gchar *stardict_data_dir=g_getenv("STARDICT_DATA_DIR");
	if (data_dir.empty()) {
		if (stardict_data_dir)
			data_dir=stardict_data_dir;
		else
			data_dir="/usr/share/stardict/dic";
	}



	strlist_t dicts_dir_list;

	dicts_dir_list.push_back(std::string(g_get_home_dir())+G_DIR_SEPARATOR+
				 ".stardict"+G_DIR_SEPARATOR+"dic");
	dicts_dir_list.push_back(data_dir);   

	if (show_list_dicts) {
		printf(_("Dictionary's name   Word count\n"));
		PrintDictInfo print_dict_info;
		strlist_t order_list, disable_list;
		for_each_file(dicts_dir_list, ".ifo",  order_list, 
			      disable_list, print_dict_info); 
    
		return EXIT_SUCCESS;
	}

	strlist_t disable_list;
	//DictInfoList  dict_info_list;
  
	if (use_book_name) {
		strlist_t empty_list;
		CreateDisableList create_disable_list(enable_list, disable_list);
		for_each_file(dicts_dir_list, ".ifo", empty_list, 
			      empty_list, create_disable_list);
	}

    
	string conf_dir = string(g_get_home_dir())+G_DIR_SEPARATOR+".stardict";
	if (g_mkdir(conf_dir.c_str(), S_IRWXU)==-1 && errno!=EEXIST)
		fprintf(stderr, _("g_mkdir failed: %s\n"), strerror(errno));

  
	Library lib(utf8_input, utf8_output);
	strlist_t empty_list;
	lib.load(dicts_dir_list, empty_list, disable_list);


	std::auto_ptr<read_line> io(create_readline_object());
	if (optind < argc) {
		for(int i=optind; i<argc; ++i)
			if (!lib.process_phrase(argv[i], *io, non_interactive))
				return EXIT_FAILURE;
	} else if (!non_interactive) {

		string phrase;
		while (io->read(_("Enter word or phrase: "), phrase)) {
			if (!lib.process_phrase(phrase.c_str(), *io))
				return EXIT_FAILURE;
			phrase.clear();
		}

		putchar('\n');
	} else
		fprintf(stderr, _("There are no words/phrases to translate.\n"));
	
    
	return EXIT_SUCCESS;
}

