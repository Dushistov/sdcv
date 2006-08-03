#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>

#include "conf.h"
#include "stardict.h"

#include "docklet.h"

/*
// for my_gtk_window_get_active()
#include <X11/Xlib.h>
#include <X11/Xatom.h>
*/

DockLet::DockLet()
{
	menu = NULL;
	image = NULL;
	embedded = false;	
	current_icon = DOCKLET_NORMAL_ICON;
}

void DockLet::Create(DockLetIconType iconType)
{	
	docklet = egg_tray_icon_new("StarDict");
	box = gtk_event_box_new();
	if (iconType == DOCKLET_NORMAL_ICON) {
		gtk_tooltips_set_tip(gpAppFrame->tooltips, box,_("StarDict"),NULL);
		image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.docklet_normal_icon.get());
	} else if (iconType == DOCKLET_SCAN_ICON) {
		gtk_tooltips_set_tip(gpAppFrame->tooltips, box,_("StarDict - Scanning"),NULL);
		image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.docklet_scan_icon.get());
	} else {
		gtk_tooltips_set_tip(gpAppFrame->tooltips, box,_("StarDict - Stopped"),NULL);
		image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.docklet_stop_icon.get());
	}
	current_icon = iconType;

	g_signal_connect(G_OBJECT(docklet), "embedded", G_CALLBACK(EmbeddedCallback), NULL);
	g_signal_connect(G_OBJECT(docklet), "destroy", G_CALLBACK(DestroyedCallback), this);
	g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(ButtonPressCallback), this);

	gtk_container_add(GTK_CONTAINER(box), image);
	gtk_container_add(GTK_CONTAINER(docklet), box);
	gtk_widget_show_all(GTK_WIDGET(docklet));

	/* ref the docklet before we bandy it about the place */
	g_object_ref(G_OBJECT(docklet));	
}

void DockLet::SetIcon(DockLetIconType icon_type)
{
	if (!image)
		return;
	if (current_icon == icon_type)
		return;
	GdkPixbuf *p;
	switch (icon_type) {
		case DOCKLET_SCAN_ICON:
			gtk_tooltips_set_tip(gpAppFrame->tooltips, box,_("StarDict - Scanning"),NULL);
			p = gpAppFrame->oAppSkin.docklet_scan_icon.get();
			break;
		case DOCKLET_STOP_ICON:
			gtk_tooltips_set_tip(gpAppFrame->tooltips, box,_("StarDict - Stopped"),NULL);
			p = gpAppFrame->oAppSkin.docklet_stop_icon.get();
			break;
		default:
			gtk_tooltips_set_tip(gpAppFrame->tooltips, box,_("StarDict"),NULL);
			p = gpAppFrame->oAppSkin.docklet_normal_icon.get();
			break;
	}
	gtk_image_set_from_pixbuf(GTK_IMAGE(image),p);
	current_icon = icon_type;
}

void DockLet::End()
{
	while (g_source_remove_by_user_data(&docklet)) {
	}
	
	g_signal_handlers_disconnect_by_func(G_OBJECT(docklet), (void *)(DestroyedCallback), this);
	gtk_widget_destroy(GTK_WIDGET(docklet));
	g_object_unref(G_OBJECT(docklet));
	
	if (menu)
		gtk_widget_destroy(menu);
}

void DockLet::DestroyedCallback(GtkWidget *widget, DockLet *oDockLet)
{
	oDockLet->embedded = false;
	oDockLet->image = NULL;
	/*if (!(GTK_WIDGET_VISIBLE(gpAppFrame->oAppCore.window)))
		gtk_widget_show(gpAppFrame->oAppCore.window); // it is better not to use gtk_window_present().
	*/
	while (g_source_remove_by_user_data(&(oDockLet->docklet))) {
	}
	g_object_unref(G_OBJECT(oDockLet->docklet));

	g_idle_add(oDockLet->docklet_create, (gpointer)(oDockLet->current_icon)); //when user add Nofification area applet again,it will show icon again.
}

void DockLet::MenuScanCallback(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
  conf->set_bool("/apps/stardict/preferences/dictionary/scan_selection",
								 gtk_check_menu_item_get_active(checkmenuitem));
}

void DockLet::MenuQuitCallback(GtkMenuItem *menuitem, gpointer user_data)
{
	gpAppFrame->Quit();
}

void DockLet::PopupMenu(GdkEventButton *event)
{
	if (!menu) {	
		menu = gtk_menu_new();
	
		scan_menuitem = gtk_check_menu_item_new_with_mnemonic(_("_Scan"));		
		g_signal_connect(G_OBJECT(scan_menuitem), "toggled", G_CALLBACK(MenuScanCallback), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), scan_menuitem);
	
		GtkWidget *menuitem;
		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
		GtkWidget *image;
		image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(MenuQuitCallback), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		gtk_widget_show_all(menu);
	}
	bool scan_selection=conf->get_bool("/apps/stardict/preferences/dictionary/scan_selection");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(scan_menuitem),
																 scan_selection);
	
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

gboolean DockLet::docklet_create(gpointer data)
{
	gpAppFrame->oDockLet.Create((DockLetIconType)(GPOINTER_TO_INT(data)));
	return false; /* for when we're called by the glib idle handler */
}

void DockLet::EmbeddedCallback(GtkWidget *widget, gpointer data)
{
	gpAppFrame->oDockLet.embedded = true;	
}

/*
static gboolean my_gtk_window_get_active(GtkWidget *main_window)
{
	// this don't perform very well, i hope gtk can support it some days.
	// code come from libwnck/ screen.c/ update_active_window().
	static Atom active_window_atom = None;
	
	if (active_window_atom == None)
		active_window_atom = XInternAtom (gdk_display, "_NET_ACTIVE_WINDOW", FALSE);
	
	Atom type;
	int format;
	gulong nitems;
	gulong bytes_after;
	Window *w;
	Window active_window;

	int err, result;
	gdk_error_trap_push ();
	type = None;
	result = XGetWindowProperty (gdk_display,
			       GDK_ROOT_WINDOW(),
			       active_window_atom,
			       0, G_MAXLONG,
			       False, XA_WINDOW, &type, &format, &nitems,
			       &bytes_after, (guchar **)&w);		
	XSync (gdk_display, False);
	err = gdk_error_trap_pop ();
	if (err != Success || result != Success) {
    	return FALSE;
	}

	if (type != XA_WINDOW)
    {
      XFree (w);
      return FALSE;
    }
	active_window = *w;
	XFree (w);
		
	if (active_window == GDK_WINDOW_XID(main_window->window))
		return true;
	else
		return false;
}
*/

gboolean DockLet::ButtonPressCallback(GtkWidget *button, GdkEventButton *event, DockLet *oDockLet)
{
	if (event->type != GDK_BUTTON_PRESS)
		return false;

	if (event->button ==1) {
		
		if ((event->state & GDK_CONTROL_MASK) && 
				!(event->state & GDK_MOD1_MASK) && 
				!(event->state & GDK_SHIFT_MASK)) {
      conf->set_bool("/apps/stardict/preferences/dictionary/scan_selection",
										 !conf->get_bool("/apps/stardict/preferences/dictionary/scan_selection"));
			return true;
    } else {			
			if (GTK_WIDGET_VISIBLE(gpAppFrame->window)) {
				//if (GTK_WINDOW(gpAppFrame->window)->is_active) {
				//if (my_gtk_window_get_active(gpAppFrame->window)) {
				gtk_widget_hide(gpAppFrame->window);
      }	else {
				gtk_window_present(GTK_WINDOW(gpAppFrame->window));
				if (gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry))[0]) {
				  gtk_widget_grab_focus(gpAppFrame->oMidWin.oTextWin.view->Widget()); //so user can input word directly.
				} else {
					gtk_widget_grab_focus(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry); //this won't change selection text.
				}
			}
		}		
  } else if (event->button ==2) {
		if (conf->get_bool("/apps/stardict/preferences/notification_area_icon/query_in_floatwin")) {
			gpAppFrame->oSelection.LastClipWord.clear();
			gtk_selection_convert(gpAppFrame->oSelection.selection_widget, GDK_SELECTION_PRIMARY, gpAppFrame->oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
    } else {
			gtk_window_present(GTK_WINDOW(gpAppFrame->window));
			gtk_selection_convert (gpAppFrame->oMidWin.oTextWin.view->Widget(), GDK_SELECTION_PRIMARY, gpAppFrame->oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
		}
		return true;
  } else if (event->button ==3) {
		oDockLet->PopupMenu(event);
		return true;
	}
	return false;
}
