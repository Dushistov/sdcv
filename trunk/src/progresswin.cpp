/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "utils.h"

#include "progresswin.hpp"

progress_win::progress_win()
{
	win = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_title(GTK_WINDOW(win), _("Loading"));
	gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	text = GTK_LABEL(gtk_label_new(_("Loading")));
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(text),false,false,0);
	progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(progress),false,false,0);
	gtk_widget_show_all(win);
	ProcessGtkEvent();
}

void progress_win::display_action(const std::string& actname)
{
	gtk_label_set_text(text, actname.c_str());
	gtk_progress_bar_pulse(progress);
	ProcessGtkEvent();
}
