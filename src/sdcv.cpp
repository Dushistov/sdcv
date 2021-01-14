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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "libwrapper.hpp"
#include "readline.hpp"
#include "utils.hpp"

static const char gVersion[] = VERSION;

namespace
{
static void free_str_array(gchar **arr)
{
    gchar **p;

    for (p = arr; *p; ++p)
        g_free(*p);
    g_free(arr);
}
} // namespace
namespace glib
{
using StrArr = ResourceWrapper<gchar *, gchar *, free_str_array>;
}

static void list_dicts(const std::list<std::string> &dicts_dir_list, bool use_json);

int main(int argc, char *argv[])
try {
    setlocale(LC_ALL, "");
#if ENABLE_NLS
    bindtextdomain("sdcv",
                   //"./locale"//< for testing
                   GETTEXT_TRANSLATIONS_PATH //< should be
    );
    textdomain("sdcv");
#endif

    gboolean show_version = FALSE;
    gboolean show_list_dicts = FALSE;
    glib::StrArr use_dict_list;
    gboolean non_interactive = FALSE;
    gboolean json_output = FALSE;
    gboolean no_fuzzy = FALSE;
    gboolean utf8_output = FALSE;
    gboolean utf8_input = FALSE;
    glib::CharStr opt_data_dir;
    gboolean only_data_dir = FALSE;
    gboolean colorize = FALSE;
    glib::StrArr word_list;

    const GOptionEntry entries[] = {
        { "version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
          _("display version information and exit"), nullptr },
        { "list-dicts", 'l', 0, G_OPTION_ARG_NONE, &show_list_dicts,
          _("display list of available dictionaries and exit"), nullptr },
        { "use-dict", 'u', 0, G_OPTION_ARG_STRING_ARRAY, get_addr(use_dict_list),
          _("for search use only dictionary with this bookname"),
          _("bookname") },
        { "non-interactive", 'n', 0, G_OPTION_ARG_NONE, &non_interactive,
          _("for use in scripts"), nullptr },
        { "json-output", 'j', 0, G_OPTION_ARG_NONE, &json_output,
          _("print the result formatted as JSON"), nullptr },
        { "exact-search", 'e', 0, G_OPTION_ARG_NONE, &no_fuzzy,
          _("do not fuzzy-search for similar words, only return exact matches"), nullptr },
        { "utf8-output", '0', 0, G_OPTION_ARG_NONE, &utf8_output,
          _("output must be in utf8"), nullptr },
        { "utf8-input", '1', 0, G_OPTION_ARG_NONE, &utf8_input,
          _("input of sdcv in utf8"), nullptr },
        { "data-dir", '2', 0, G_OPTION_ARG_STRING, get_addr(opt_data_dir),
          _("use this directory as path to stardict data directory"),
          _("path/to/dir") },
        { "only-data-dir", 'x', 0, G_OPTION_ARG_NONE, &only_data_dir,
          _("only use the dictionaries in data-dir, do not search in user and system directories"), nullptr },
        { "color", 'c', 0, G_OPTION_ARG_NONE, &colorize,
          _("colorize the output"), nullptr },
        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, get_addr(word_list),
          _("search terms"), _(" words") },
        {},
    };

    glib::Error error;
    GOptionContext *context = g_option_context_new(nullptr);
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
        if (!only_data_dir) {
            if (stardict_data_dir)
                data_dir = stardict_data_dir;
            else
                data_dir = "/usr/share/stardict/dic";
        }
    } else {
        data_dir = get_impl(opt_data_dir);
    }

    std::string conf_dir = std::string(g_get_home_dir()) + G_DIR_SEPARATOR + ".stardict";
    if (!g_file_test(conf_dir.c_str(), G_FILE_TEST_IS_DIR))
        conf_dir = std::string(g_get_user_data_dir()) + G_DIR_SEPARATOR + "stardict";

    std::list<std::string> dicts_dir_list;
    if (!only_data_dir)
        dicts_dir_list.push_back(conf_dir + G_DIR_SEPARATOR + "dic");
    dicts_dir_list.push_back(data_dir);
    if (show_list_dicts) {
        list_dicts(dicts_dir_list, json_output);
        return EXIT_SUCCESS;
    }

    std::list<std::string> disable_list;

    std::map<std::string, std::string> bookname_to_ifo;
    for_each_file(dicts_dir_list, ".ifo", std::list<std::string>(), std::list<std::string>(),
                  [&bookname_to_ifo](const std::string &fname, bool) {
                      DictInfo dict_info;
                      const bool load_ok = dict_info.load_from_ifo_file(fname, false);
                      if (!load_ok)
                          return;
                      bookname_to_ifo[dict_info.bookname] = dict_info.ifo_file_name;
                  });

    std::list<std::string> order_list;
    if (use_dict_list != nullptr) {
        for (auto &&x : bookname_to_ifo) {
            gchar **p = get_impl(use_dict_list);
            for (; *p != nullptr; ++p)
                if (x.first.compare(*p) == 0) {
                    break;
                }
            if (*p == nullptr) {
                disable_list.push_back(x.second);
            }
        }

        // add bookname to list
        gchar **p = get_impl(use_dict_list);
        while (*p) {
            order_list.push_back(bookname_to_ifo.at(*p));
            ++p;
        }
    } else {
        std::string ordering_cfg_file = std::string(g_get_user_config_dir()) + G_DIR_SEPARATOR_S "sdcv_ordering";
        FILE *ordering_file = fopen(ordering_cfg_file.c_str(), "r");
        if (ordering_file == nullptr) {
            ordering_cfg_file = std::string(g_get_home_dir()) + G_DIR_SEPARATOR_S ".sdcv_ordering";
            ordering_file = fopen(ordering_cfg_file.c_str(), "r");
        }
        if (ordering_file != nullptr) {
            std::string line;
            while (stdio_getline(ordering_file, line)) {
                order_list.push_back(bookname_to_ifo.at(line));
            }
            fclose(ordering_file);
        }
    }

    if (g_mkdir(conf_dir.c_str(), S_IRWXU) == -1 && errno != EEXIST) {
        fprintf(stderr, _("g_mkdir failed: %s\n"), strerror(errno));
    }

    Library lib(utf8_input, utf8_output, colorize, json_output, no_fuzzy);
    lib.load(dicts_dir_list, order_list, disable_list);

    std::unique_ptr<IReadLine> io(create_readline_object());
    if (word_list != nullptr) {
        search_result rval = SEARCH_SUCCESS;
        gchar **p = get_impl(word_list);
        while (*p) {
            if ((rval = lib.process_phrase(*p++, *io, non_interactive)) != SEARCH_SUCCESS) {
                return rval;
            }
        }
    } else if (!non_interactive) {

        std::string phrase;
        while (io->read(_("Enter word or phrase: "), phrase)) {
            if (lib.process_phrase(phrase.c_str(), *io) == SEARCH_FAILURE)
                return EXIT_FAILURE;
            phrase.clear();
        }

        putchar('\n');
    } else {
        fprintf(stderr, _("There are no words/phrases to translate.\n"));
    }
    return EXIT_SUCCESS;
} catch (const std::exception &ex) {
    fprintf(stderr, "Internal error: %s\n", ex.what());
    exit(EXIT_FAILURE);
}

static void list_dicts(const std::list<std::string> &dicts_dir_list, bool use_json)
{
    bool first_entry = true;
    if (!use_json)
        printf(_("Dictionary's name   Word count\n"));
    else
        fputc('[', stdout);
    std::list<std::string> order_list, disable_list;
    for_each_file(dicts_dir_list, ".ifo", order_list,
                  disable_list, [use_json, &first_entry](const std::string &filename, bool) -> void {
                      DictInfo dict_info;
                      if (dict_info.load_from_ifo_file(filename, false)) {
                          const std::string bookname = utf8_to_locale_ign_err(dict_info.bookname);
                          if (use_json) {
                              if (first_entry) {
                                  first_entry = false;
                              } else {
                                  fputc(',', stdout); // comma between entries
                              }
                              printf("{\"name\": \"%s\", \"wordcount\": \"%d\"}", json_escape_string(bookname).c_str(), dict_info.wordcount);
                          } else {
                              printf("%s    %d\n", bookname.c_str(), dict_info.wordcount);
                          }
                      }
                  });
    if (use_json)
        fputs("]\n", stdout);
}
