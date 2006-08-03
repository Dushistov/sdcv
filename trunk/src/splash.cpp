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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "conf.h"
#include "utils.h"

#include "splash.h"

splash_screen stardict_splash;

// this is the callback which will destroy the splash screen window.
static void
splash_screen_cb(gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

void splash_screen::display_action(const std::string& actname)
{
	g_print("%s\n", actname.c_str());
	gtk_label_set_text(text, actname.c_str());
	gtk_progress_bar_pulse(progress);
	while(gtk_events_pending())
		gtk_main_iteration();
}

void splash_screen::show()
{
	gtk_window_set_auto_startup_notification(FALSE);

//This function never failed
	GtkWidget *image =
		gtk_image_new_from_file((gStarDictDataDir +
					 G_DIR_SEPARATOR_S "pixmaps"
					 G_DIR_SEPARATOR_S "splash.png").c_str());

	GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),image,false,false,0);
	text = GTK_LABEL(gtk_label_new(_("Loading")));
	int w = gdk_pixbuf_get_width(gtk_image_get_pixbuf(GTK_IMAGE(image)));
	gtk_widget_set_size_request(GTK_WIDGET(text), w, -1);
	gtk_label_set_line_wrap(text, TRUE);
	gtk_label_set_justify(text, GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(text),false,false,0);
	progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
	gtk_widget_set_size_request(GTK_WIDGET(progress), w, 10);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(progress),false,false,0);

	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	//gtk_widget_set_app_paintable(window, TRUE);
	gtk_window_set_title (GTK_WINDOW (window), _("StarDict"));
	gtk_window_set_position(GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	//gtk_widget_set_size_request(window, w, h);
	gtk_widget_show(window);
	// go into main loop, processing events.
	while(gtk_events_pending())
		gtk_main_iteration();

	// it will be called after gtk_main() is called.
	gtk_init_add ((GtkFunction)splash_screen_cb, (gpointer)window);

	gtk_window_set_auto_startup_notification(TRUE);
}
