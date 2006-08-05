#ifndef __SD_DOCKLET_H__
#define __SD_DOCKLET_H__

#include <gtk/gtk.h>

#include "utils.h"

#include "trayicon.hpp"

//forward declarations to speedup building
class AppSkin;
struct EggTrayIcon;

class DockLet : public TrayIcon{
public:
	DockLet(GtkWidget *, bool, GtkTooltips *, const AppSkin&);
	~DockLet();

	void set_scan_mode(bool);
	void hide_state();
	void minimize_to_tray();
	void maximize_from_tray();
private:
	EggTrayIcon *docklet_;
	GtkWidget *box_;
	GtkWidget *image_; //icon image.
	typedef  ResourceWrapper<GtkWidget, GtkWidget, gtk_widget_destroy> GMenu;
	GMenu menu_;
	GtkWidget *scan_menuitem_;
	bool cur_state_;
	GdkPixbuf *normal_icon_, *stop_icon_, *scan_icon_;
	GtkTooltips *tooltips_;
	bool embedded_;
	bool hide_state_;

	static void on_embedded(GtkWidget *, gpointer);
	static void on_destroy(GtkWidget *, DockLet *);
	static gboolean on_btn_press(GtkWidget *, GdkEventButton *, DockLet *);
	static void on_menu_scan(GtkCheckMenuItem *, gpointer);
	static void on_quit(GtkMenuItem *, gpointer);
	static gboolean docklet_create(gpointer);

	void popup_menu(GdkEventButton *);
	void create_docklet(bool);
};


#endif//!__SD_DOCKLET_H__
