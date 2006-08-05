#ifndef __SD_DOCKLET_H__
#define __SD_DOCKLET_H__

#include <gtk/gtk.h>
#include <map>
#include <utility>
#include <string>

#include "utils.h"

#include "trayicon.hpp"

//forward declarations to speedup building
class AppSkin;
struct EggTrayIcon;

class DockLet : public TrayIcon{
public:
	DockLet(GtkWidget *, GtkTooltips *, const AppSkin&,
		State state = NORMAL_ICON);
	~DockLet();
	bool is_embedded() const { return embedded_; }
	void set_state(State);
	void minimize_to_tray();
	void maximize_from_tray() {}
private:
	EggTrayIcon *docklet_;
	GtkWidget *box_;
	GtkWidget *image_; //icon image.
	typedef  ResourceWrapper<GtkWidget, GtkWidget, gtk_widget_destroy> Menu;
	Menu menu_;
	GtkWidget *scan_menuitem_;
	State cur_state_;
	typedef std::map<State, std::pair<std::string, GdkPixbuf *> > StateMap;
	StateMap state_map_;
	GtkTooltips *tooltips_;
	bool embedded_;

	static void on_embedded(GtkWidget *, gpointer);
	static void on_destroy(GtkWidget *, DockLet *);
	static gboolean on_btn_press(GtkWidget *, GdkEventButton *, DockLet *);
	static void on_menu_scan(GtkCheckMenuItem *, gpointer);
	static void on_quit(GtkMenuItem *, gpointer);
	static gboolean docklet_create(gpointer);

	void popup_menu(GdkEventButton *);
	void create_docklet(State);
};


#endif
