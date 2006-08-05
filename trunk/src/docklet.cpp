#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>

#include "conf.h"
#include "eggtrayicon.h"
#include "skin.h"

#include "docklet.h"


void DockLet::create_docklet(State state)
{
	StateMap::const_iterator it = state_map_.find(state);
	g_assert(it != state_map_.end());
	docklet_ = egg_tray_icon_new("StarDict");
	box_ = gtk_event_box_new();

	gtk_tooltips_set_tip(tooltips_, box_, it->second.first.c_str(), NULL);
	image_ = gtk_image_new_from_pixbuf(it->second.second);
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

DockLet::DockLet(GtkWidget *win, GtkTooltips *tooltips, const AppSkin& skin,
		 State state) : TrayIcon(win)
{
	image_ = NULL;
	embedded_ = false;

	tooltips_ = tooltips;

	state_map_[NORMAL_ICON] =
		std::make_pair(_("StarDict"),
			       skin.get_image("docklet_normal_icon"));

	state_map_[SCAN_ICON] =
		std::make_pair(_("StarDict - Scanning"),
			       skin.get_image("docklet_scan_icon"));

	state_map_[STOP_ICON] =
		std::make_pair(_("StarDict - Stopped"),
			       skin.get_image("docklet_stop_icon"));
	create_docklet(state);
}

void DockLet::set_state(State new_state)
{
	if (!image_ || new_state == cur_state_)
		return;
	StateMap::const_iterator it = state_map_.find(new_state);
	g_assert(it != state_map_.end());

	gtk_tooltips_set_tip(tooltips_, box_, it->second.first.c_str(), NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(image_), it->second.second);
	cur_state_ = new_state;
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
	bool scan_selection=conf->get_bool("/apps/stardict/preferences/dictionary/scan_selection");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(scan_menuitem_),
				       scan_selection);

	gtk_menu_popup(GTK_MENU(menu_.get()), NULL, NULL, NULL, NULL,
		       event->button, event->time);
}

gboolean DockLet::docklet_create(gpointer data)
{
	DockLet *d = static_cast<DockLet *>(data);
	g_assert(d);
	d->create_docklet(d->cur_state_);
	return FALSE; /* for when we're called by the glib idle handler */
}

void DockLet::on_embedded(GtkWidget *widget, gpointer data)
{
	static_cast<DockLet *>(data)->embedded_ = true;
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
			bool scan_select =
				conf->get_bool_at("dictionary/scan_selection");
			oDockLet->on_change_scan_.emit(!scan_select);
			return TRUE;
		} else {
			if (GTK_WIDGET_VISIBLE(oDockLet->mainwin_))	
				gtk_widget_hide(oDockLet->mainwin_);
			else
				oDockLet->on_maximize_.emit();
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


#if 0
gboolean DockLet::ButtonPressCallback(GtkWidget *button, GdkEventButton *event, DockLet *oDockLet)
{
	if (event->type != GDK_BUTTON_PRESS)
		return false;

	if (event->button ==1) {
		
		if ((event->state & GDK_CONTROL_MASK) && 
		    !(event->state & GDK_MOD1_MASK) && 
		    !(event->state & GDK_SHIFT_MASK)) {
			conf->set_bool_at("dictionary/scan_selection",
					  !conf->get_bool_at("dictionary/scan_selection"));
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
		if (conf->get_bool_at("notification_area_icon/query_in_floatwin")) {
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
#endif
