/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Changed by Evgeniy <dushistov@mail.ru> on 2006.08
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

#include "eggtrayicon.h"
#include "skin.h"

#include "docklet.h"


void DockLet::create_docklet(bool state)
{
	docklet_ = egg_tray_icon_new("StarDict");
	box_ = gtk_event_box_new();

	if (hide_state_) {
		gtk_tooltips_set_tip(tooltips_, box_, _("StarDict"), NULL);
		image_ = gtk_image_new_from_pixbuf(normal_icon_);
	} else if (state) {
		gtk_tooltips_set_tip(tooltips_, box_, _("StarDict - Scanning"),
				     NULL);
		image_ = gtk_image_new_from_pixbuf(scan_icon_);
	} else {
		gtk_tooltips_set_tip(tooltips_, box_, _("StarDict - Stopped"),
				     NULL);
		image_ = gtk_image_new_from_pixbuf(stop_icon_);
	}

	cur_state_ = state;

	g_signal_connect(G_OBJECT(docklet_), "embedded",
			 G_CALLBACK(on_embedded), this);
	g_signal_connect(G_OBJECT(docklet_), "destroy", G_CALLBACK(on_destroy),
			 this);
	g_signal_connect(G_OBJECT(box_), "button-press-event",
			 G_CALLBACK(on_btn_press), this);

	gtk_container_add(GTK_CONTAINER(box_), image_);
	gtk_container_add(GTK_CONTAINER(docklet_), box_);
	gtk_widget_show_all(GTK_WIDGET(docklet_));

	/* ref the docklet before we bandy it about the place */
	g_object_ref(G_OBJECT(docklet_));
}

DockLet::DockLet(GtkWidget *win, bool state, GtkTooltips *tooltips,
		 const AppSkin& skin) : TrayIcon(win)
{
	image_ = NULL;
	embedded_ = false;
	hide_state_ = false;

	tooltips_ = tooltips;

	normal_icon_ = skin.get_image("docklet_normal_icon");
	scan_icon_ =  skin.get_image("docklet_scan_icon");
	stop_icon_ = skin.get_image("docklet_stop_icon");

	create_docklet(state);
}

void DockLet::set_scan_mode(bool is_on)
{
	if (!image_ || (!hide_state_ && is_on == cur_state_))
		return;

	hide_state_ = false;

	if (is_on) {
		gtk_tooltips_set_tip(tooltips_, box_, _("StarDict - Scanning"),
				     NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(image_), scan_icon_);
	} else {
		gtk_tooltips_set_tip(tooltips_, box_, _("StarDict - Stopped"),
				     NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(image_), stop_icon_);
	}

	cur_state_ = is_on;
}

DockLet::~DockLet()
{
	while (g_source_remove_by_user_data(&docklet_))
		;

	g_signal_handlers_disconnect_by_func(G_OBJECT(docklet_),
					     (void *)(on_destroy), this);
	gtk_widget_destroy(GTK_WIDGET(docklet_));
	g_object_unref(G_OBJECT(docklet_));
}

void DockLet::on_destroy(GtkWidget *widget, DockLet *oDockLet)
{
	oDockLet->embedded_ = false;
	oDockLet->image_ = NULL;

	while (g_source_remove_by_user_data(&oDockLet->docklet_))
		;
	g_object_unref(G_OBJECT(oDockLet->docklet_));

	//when user add Nofification area applet again,it will show icon again.
	g_idle_add(oDockLet->docklet_create, gpointer(oDockLet));
}

void DockLet::on_menu_scan(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
	static_cast<DockLet *>(user_data)->on_change_scan_.emit(
		gtk_check_menu_item_get_active(checkmenuitem));
}

void DockLet::on_quit(GtkMenuItem *menuitem, gpointer user_data)
{
	static_cast<DockLet *>(user_data)->on_quit_.emit();
}

void DockLet::popup_menu(GdkEventButton *event)
{
	if (!menu_.get()) {
		menu_.reset(gtk_menu_new());

		scan_menuitem_ = gtk_check_menu_item_new_with_mnemonic(_("_Scan"));
		g_signal_connect(G_OBJECT(scan_menuitem_), "toggled",
				 G_CALLBACK(on_menu_scan), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_.get()), scan_menuitem_);

		GtkWidget *menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_.get()), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
		GtkWidget *image;
		image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_quit), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_.get()), menuitem);

		gtk_widget_show_all(menu_.get());
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(scan_menuitem_),
				       cur_state_);

	gtk_menu_popup(GTK_MENU(menu_.get()), NULL, NULL, NULL, NULL,
		       event->button, event->time);
}

gboolean DockLet::docklet_create(gpointer data)
{
	g_assert(data);
	DockLet *d = static_cast<DockLet *>(data);
	d->create_docklet(d->cur_state_);
	return FALSE; /* for when we're called by the glib idle handler */
}

void DockLet::on_embedded(GtkWidget *widget, gpointer data)
{
	static_cast<DockLet *>(data)->embedded_ = true;
}

void DockLet::hide_state()
{
	if (hide_state_)
		return;
	gtk_tooltips_set_tip(tooltips_, box_, _("StarDict"), NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(image_), normal_icon_);
	hide_state_ = true;
}

gboolean DockLet::on_btn_press(GtkWidget *button, GdkEventButton *event,
			       DockLet *oDockLet)
{
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	switch (event->button) {
	case 1:
		if ((event->state & GDK_CONTROL_MASK) &&
		    !(event->state & GDK_MOD1_MASK) &&
		    !(event->state & GDK_SHIFT_MASK)) {
			oDockLet->on_change_scan_.emit(!oDockLet->cur_state_);
			return TRUE;
		} else {
			if (GTK_WIDGET_VISIBLE(oDockLet->mainwin_))	
				gtk_widget_hide(oDockLet->mainwin_);
			else {
				oDockLet->maximize_from_tray();
				oDockLet->on_maximize_.emit();
			}
		}	
		break;
	case 2:
		oDockLet->on_middle_button_click_.emit();
		return TRUE;
	case 3:
		oDockLet->popup_menu(event);
		return TRUE;
	default:
		/* nothing */;
		break;
	}
	return FALSE;
}

void DockLet::minimize_to_tray()
{
	if (embedded_)
		gtk_widget_hide(mainwin_);
	else
		on_quit_.emit();
}

void DockLet::maximize_from_tray() 
{
	gtk_window_present(GTK_WINDOW(mainwin_));
}
