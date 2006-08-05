#ifndef TRAYICON_HPP
#define TRAYICON_HPP

#include <gtk/gtk.h>
#include <sigc++/sigc++.h>


class TrayIcon {
public:
	enum State {
			NORMAL_ICON,
			SCAN_ICON,
			STOP_ICON,
	};
	sigc::signal<void> on_quit_;
	sigc::signal<void, bool> on_change_scan_;
	sigc::signal<void> on_middle_button_click_;
	sigc::signal<void> on_maximize_;

	TrayIcon(GtkWidget *mainwin) : mainwin_(mainwin) {}
	virtual ~TrayIcon() {}
	virtual void set_state(State) = 0;
	virtual void minimize_to_tray() = 0;		
	virtual void maximize_from_tray() = 0;
protected:
	GtkWidget *mainwin_;
};

#endif//!TRAYICON_HPP
