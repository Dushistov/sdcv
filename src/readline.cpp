/*
 * This file part of sdcv - console version of Stardict program
 * http://sdcv.sourceforge.net
 * Copyright (C) 2005 Evgeniy <dushistov@mail.ru>
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

#include <cstdio>
#include <cstdlib>
#ifdef WITH_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif
#include <glib.h>

#include "utils.hpp"

#include "readline.hpp"

bool stdio_getline(FILE *in, std::string &str)
{
    assert(in != nullptr);
    str.clear();
    int ch;
    while ((ch = fgetc(in)) != EOF && ch != '\n')
        str += ch;

    return EOF != ch;
}

#ifndef WITH_READLINE
namespace
{
class dummy_readline : public IReadLine
{
public:
    bool read(const std::string &banner, std::string &line) override
    {
        printf("%s", banner.c_str());
        return stdio_getline(stdin, line);
    }
};
}
#else

namespace
{
std::string get_hist_file_path()
{
    const gchar *hist_file_str = g_getenv("SDCV_HISTFILE");

    if (hist_file_str != nullptr)
        return std::string(hist_file_str);
    else
        return std::string(g_get_user_data_dir()) + G_DIR_SEPARATOR + "sdcv_history";
}

class real_readline : public IReadLine
{

public:
    real_readline()
    {
        rl_readline_name = "sdcv";
        using_history();
        const std::string histname = get_hist_file_path();
        read_history(histname.c_str());
    }

    ~real_readline()
    {
        const std::string histname = get_hist_file_path();
        write_history(histname.c_str());
        const gchar *hist_size_str = g_getenv("SDCV_HISTSIZE");
        int hist_size;
        if (!hist_size_str || sscanf(hist_size_str, "%d", &hist_size) < 1)
            hist_size = 2000;
        history_truncate_file(histname.c_str(), hist_size);
    }

    bool read(const std::string &banner, std::string &line) override
    {
        char *phrase = nullptr;
        phrase = readline(banner.c_str());
        if (phrase) {
            line = phrase;
            free(phrase);
            return true;
        }
        return false;
    }

    void add_to_history(const std::string &phrase) override
    {
        add_history(phrase.c_str());
    }
};
}
#endif //WITH_READLINE

IReadLine *create_readline_object()
{
#ifdef WITH_READLINE
    return new real_readline;
#else
    return new dummy_readline;
#endif
}
