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
#include <string>
#include <vector>
#include <memory>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "libwrapper.hpp"
#include "readline.hpp"
#include "utils.hpp"

static const char gVersion[] = VERSION;

namespace {
    static void free_str_array(gchar **arr)
    {
        gchar **p;

        for (p = arr; *p; ++p)
            g_free(*p);
        g_free(arr);
    }
}
namespace glib {
	typedef ResourceWrapper<gchar *, gchar *, free_str_array> StrArr;
}

int main(int argc, char *argv[]) try {
	setlocale(LC_ALL, "");
#if ENABLE_NLS
	bindtextdomain("sdcv",
                   //"./locale"//< for testing
                   GETTEXT_TRANSLATIONS_PATH//< should be
        );
	textdomain("sdcv");
#endif

	gboolean show_version = FALSE;
	gboolean show_list_dicts = FALSE;
	glib::StrArr use_dict_list;
	gboolean non_interactive = FALSE;
	gboolean utf8_output = FALSE;
	gboolean utf8_input = FALSE;
	glib::CharStr opt_data_dir;
    gboolean colorize = FALSE;

	const GOptionEntry entries[] = {
		{"version", 'v', 0, G_OPTION_ARG_NONE, &show_version, 
		 _("display version information and exit"), nullptr },
		{"list-dicts", 'l', 0, G_OPTION_ARG_NONE, &show_list_dicts, 
		 _("display list of available dictionaries and exit"), nullptr},
		{"use-dict", 'u', 0, G_OPTION_ARG_STRING_ARRAY, get_addr(use_dict_list),
		 _("for search use only dictionary with this bookname"),
		 _("bookname")},
		{"non-interactive", 'n', 0, G_OPTION_ARG_NONE, &non_interactive, 
		 _("for use in scripts"), nullptr},
		{"utf8-output", '0', 0, G_OPTION_ARG_NONE, &utf8_output, 
		 _("output must be in utf8"), nullptr},
		{"utf8-input", '1', 0, G_OPTION_ARG_NONE, &utf8_input,
		 _("input of sdcv in utf8"), nullptr},
		{"data-dir", '2', 0, G_OPTION_ARG_STRING, get_addr(opt_data_dir), 
		 _("use this directory as path to stardict data directory"),
		 _("path/to/dir")},
		{"color", 'c', 0, G_OPTION_ARG_NONE, &colorize, 
		 _("colorize the output"), nullptr },
		{ nullptr },
	};

	glib::Error error;
	GOptionContext *context = g_option_context_new(_(" words"));
    g_option_context_set_help_enabled(context, TRUE);
    g_option_context_add_main_entries(context, entries, nullptr);
    const gboolean parse_res = g_option_context_parse(context, &argc, &argv, get_addr(error));
    g_option_context_free(context);
    if (!parse_res) {
        fprintf(stderr, _("Invalid command line arguments: %s\n"),
                error->message);		 
        return EXIT_FAILURE;
    }

	 if (show_version) {
		printf(_("Console version of Stardict, version %s\n"), gVersion);
		return EXIT_SUCCESS;
	 }


	const gchar *stardict_data_dir = g_getenv("STARDICT_DATA_DIR");
	std::string data_dir;
	if (!opt_data_dir) {
		if (stardict_data_dir)
			data_dir = stardict_data_dir;
		else
			data_dir = "/usr/share/stardict/dic";
	} else {
		data_dir = get_impl(opt_data_dir);
	}

	const char *homedir = g_getenv ("HOME");
	if (!homedir)
		homedir = g_get_home_dir ();

	const std::list<std::string> dicts_dir_list = {
        std::string(homedir) + G_DIR_SEPARATOR + ".stardict" + G_DIR_SEPARATOR + "dic",
        data_dir
    };

	if (show_list_dicts) {
		printf(_("Dictionary's name   Word count\n"));
		std::list<std::string> order_list, disable_list;
		for_each_file(dicts_dir_list, ".ifo",  order_list, 
                      disable_list, [](const std::string& filename, bool) -> void {
                          		DictInfo dict_info;
                                if (dict_info.load_from_ifo_file(filename, false)) {
                                    const std::string bookname = utf8_to_locale_ign_err(dict_info.bookname);
                                    printf("%s    %d\n", bookname.c_str(), dict_info.wordcount);
                                }
                      });
    
		return EXIT_SUCCESS;
	}

	std::list<std::string> disable_list;
  
	if (use_dict_list) {
		std::list<std::string> empty_list;

		for_each_file(dicts_dir_list, ".ifo", empty_list, empty_list,
                      [&disable_list, &use_dict_list](const std::string& filename, bool) -> void {
                          DictInfo dict_info;
                          const bool load_ok = dict_info.load_from_ifo_file(filename, false);
                          if (!load_ok)
                              return;
                          
                          for (gchar **p = get_impl(use_dict_list); *p != nullptr; ++p)
                              if (strcmp(*p, dict_info.bookname.c_str()) == 0)
                                  return;
                          disable_list.push_back(dict_info.ifo_file_name);
                      });
	}

	const std::string conf_dir = std::string(g_get_home_dir()) + G_DIR_SEPARATOR + ".stardict";
	if (g_mkdir(conf_dir.c_str(), S_IRWXU) == -1 && errno!=EEXIST)
		fprintf(stderr, _("g_mkdir failed: %s\n"), strerror(errno));

  
	Library lib(utf8_input, utf8_output, colorize);
	std::list<std::string> empty_list;
	lib.load(dicts_dir_list, empty_list, disable_list);

	std::unique_ptr<IReadLine> io(create_readline_object());
	if (optind < argc) {
		for(int i = optind; i < argc; ++i)
			if (!lib.process_phrase(argv[i], *io, non_interactive))
				return EXIT_FAILURE;
	} else if (!non_interactive) {

        std::string phrase;
		while (io->read(_("Enter word or phrase: "), phrase)) {
			if (!lib.process_phrase(phrase.c_str(), *io))
				return EXIT_FAILURE;
			phrase.clear();
		}

		putchar('\n');
	} else
		fprintf(stderr, _("There are no words/phrases to translate.\n"));
    
	return EXIT_SUCCESS;
} catch (const std::exception& ex) {
    fprintf(stderr, "Internal error: %s\n", ex.what());
    exit(EXIT_FAILURE);
}
