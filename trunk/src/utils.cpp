/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <cstdlib>

#ifdef CONFIG_GNOME
#  include <libgnome/libgnome.h>
#  include <libgnomeui/libgnomeui.h>
#elif defined(_WIN32)
#  include <gdk/gdkwin32.h>
#endif

#include "conf.h"
#include "stardict.h"

#include "utils.h"

#if defined(CONFIG_GTK) || defined(CONFIG_GPE)
static void spawn_command(const gchar *exe, const gchar *arg)
{
  gchar *cmd=g_strdup_printf("%s '%s'", exe, arg);
  g_spawn_command_line_async(cmd, NULL);
  g_free(cmd);
}
#endif

void play_wav_file(const std::string& filename)
{
#ifdef _WIN32
  PlaySound(filename.c_str(), 0, SND_ASYNC | SND_FILENAME);
#elif defined(CONFIG_GNOME)
  gnome_sound_play(filename.c_str());
#else
	const std::string &playcmd=
		conf->get_string("/apps/stardict/preferences/dictionary/play_command");
  spawn_command(playcmd.c_str(), filename.c_str());
#endif
}

void show_help(const gchar *section)
{
#ifdef _WIN32
  gchar *filename = g_strdup_printf(_("%s\\help\\stardict.chm"), gStarDictDataDir.c_str());
  ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)), "OPEN", filename, NULL, NULL, SW_SHOWNORMAL);
  g_free(filename);
#elif defined(CONFIG_GNOME)
  gnome_help_display ("stardict.xml", section, NULL);
#endif
}

void show_url(const gchar *url)
{
#ifdef _WIN32
	ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)), "OPEN", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(CONFIG_GNOME)
	gnome_url_show(url, NULL);
#elif defined(CONFIG_GPE)
	gchar *command = g_strdup_printf("gpe-mini-browser %s", url);
	system(command);
	g_free(command);
#endif
}

void ProcessGtkEvent()
{
  while (gtk_events_pending())
    gtk_main_iteration();
}

std::string get_user_config_dir()
{
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict";
	return res;
#else
	std::string res;
	gchar *tmp = g_build_filename(g_get_home_dir(), ".stardict", NULL);
	res=tmp;
	g_free(tmp);
	return res;
#endif
}

std::string combnum2str(gint comb_code)
{
  switch (comb_code) {
#ifdef _WIN32
  case 0:
    return "Shift";
  case 1:
    return "Alt";
  case 2:
    return "Ctrl";
  case 3:
    return "Ctrl+Alt";
#else
  case 0:
    return "Win";
  case 1:
    return "Shift";
  case 2:
    return "Alt";
  case 3:
    return "Ctrl";
  case 4:
    return "Ctrl+Alt";
  case 5:
    return "Ctrl+e";
  case 6:
    return "F1";
  case 7:
    return "F2";
  case 8:
    return "F3";
  case 9:
    return "F4";
#endif
  default:
    return "";
  }
}

std::vector<std::string> split(const std::string& str, char sep)
{
	std::vector<std::string> res;
	std::string::size_type prev_pos=0, pos = 0;
	while ((pos=str.find(sep, prev_pos))!=std::string::npos) {
		res.push_back(std::string(str, prev_pos, pos-prev_pos));
		prev_pos=pos+1;
	}
	res.push_back(std::string(str, prev_pos, str.length()-prev_pos));

	return res;
}

GdkPixbuf *load_image_from_file(const std::string& filename)
{
	GError *err=NULL;
	GdkPixbuf *res=gdk_pixbuf_new_from_file(filename.c_str(), &err);
	if (!res) {		
		 GtkWidget *message_dlg = 
			 gtk_message_dialog_new(
															NULL,
															(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
															GTK_MESSAGE_ERROR,
															GTK_BUTTONS_OK,
															_("Can not load image. %s"), err->message);
    
    gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
    
    gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
    
    gtk_dialog_run(GTK_DIALOG(message_dlg));
    gtk_widget_destroy(message_dlg);
		g_error_free(err);
		exit(EXIT_FAILURE);
	}

	return res;
}

void play_sound_on_event(const gchar *eventname)
{
	if (conf->get_bool("/apps/stardict/preferences/dictionary/enable_sound_event"))
	  play_wav_file(gStarDictDataDir+ G_DIR_SEPARATOR_S "sounds" G_DIR_SEPARATOR_S +eventname+".wav");
}
