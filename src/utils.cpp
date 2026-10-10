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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <dirent.h>
#endif
#include <glib.h>
#include <glib/gi18n.h>
#include <iomanip>
#include <sstream>

#include "utils.hpp"

std::string utf8_to_locale_ign_err(const std::string &utf8_str)
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
        if (nullptr == get_impl(tmp)) {
            fprintf(stderr, _("Can not convert %s to current locale.\n"), utf8_str.c_str());
            fprintf(stderr, "%s\n", err->message);
            exit(EXIT_FAILURE);
        }
        res = get_impl(tmp);
    }

    return res;
}

static void __for_each_file(const std::string &dirname, const std::string &suff,
                            const std::list<std::string> &order_list, const std::list<std::string> &disable_list,
                            const std::function<void(const std::string &, bool)> &f)
{
    bool has_dict = false;
    std::string res_dir;
    const auto visit = [&](const gchar *filename, const std::string &fullfilename, bool is_dir) {
        if (is_dir) {
            if (strcmp(filename, "res") == 0)
                res_dir = fullfilename; // decided below, once we know if this is a dictionary
            else
                __for_each_file(fullfilename, suff, order_list, disable_list, f);
        } else if (g_str_has_suffix(filename, suff.c_str())) {
            has_dict = true;
            if (std::find(order_list.begin(), order_list.end(), fullfilename) == order_list.end()) {
                const bool disable = std::find(disable_list.begin(),
                                               disable_list.end(),
                                               fullfilename)
                                     != disable_list.end();
                f(fullfilename, disable);
            }
        }
    };

#ifndef _WIN32
    // readdir() rather than g_dir_read_name(): d_type tells directories from
    // files without a stat() per entry. With a cold cache every stat() is a
    // disk read; on FAT it is also a linear scan of the parent directory, and
    // a stat() of a directory reads all of it. A dictionary's res/ folder can
    // hold thousands of files, so this made the scan take seconds.
    DIR *dir = opendir(dirname.c_str());
    if (!dir)
        return;
    const struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        const gchar *filename = entry->d_name;
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
            continue;
        const std::string fullfilename(dirname + G_DIR_SEPARATOR_S + filename);
#ifdef DT_DIR
        bool is_dir;
        if (entry->d_type == DT_DIR)
            is_dir = true;
        else if (entry->d_type == DT_REG)
            is_dir = false;
        else // DT_UNKNOWN, or a symlink that may point to a directory
            is_dir = g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR);
#else
        const bool is_dir = g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR);
#endif
        visit(filename, fullfilename, is_dir);
    }
    closedir(dir);
#else
    GDir *dir = g_dir_open(dirname.c_str(), 0, nullptr);
    if (!dir)
        return;
    const gchar *filename;
    while ((filename = g_dir_read_name(dir)) != nullptr) {
        const std::string fullfilename(dirname + G_DIR_SEPARATOR_S + filename);
        visit(filename, fullfilename, g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR));
    }
    g_dir_close(dir);
#endif

    // The res/ folder next to a dictionary holds its resources (images,
    // sounds), never dictionaries, and can contain thousands of files.
    if (!res_dir.empty() && !has_dict)
        __for_each_file(res_dir, suff, order_list, disable_list, f);
}

void for_each_file(const std::list<std::string> &dirs_list, const std::string &suff,
                   const std::list<std::string> &order_list, const std::list<std::string> &disable_list,
                   const std::function<void(const std::string &, bool)> &f)
{
    for (const std::string &item : order_list) {
        const bool disable = std::find(disable_list.begin(), disable_list.end(), item) != disable_list.end();
        f(item, disable);
    }
    for (const std::string &item : dirs_list)
        __for_each_file(item, suff, order_list, disable_list, f);
}

// based on https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c/33799784#33799784
std::string json_escape_string(const std::string &s)
{
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"':
            o << "\\\"";
            break;
        case '\\':
            o << "\\\\";
            break;
        case '\b':
            o << "\\b";
            break;
        case '\f':
            o << "\\f";
            break;
        case '\n':
            o << "\\n";
            break;
        case '\r':
            o << "\\r";
            break;
        case '\t':
            o << "\\t";
            break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}
