#ifndef TRAYICON_HPP
#define TRAYICON_HPP

#include <gtk/gtk.h>
#include <sigc++/sigc++.h>

/**
 * Base class for platform specific implementation of tray
 */
class TrayIcon {
public:
	sigc::signal<void> on_quit_;//!< On quit menu choise
	sigc::signal<void, bool> on_change_scan_;//!< On scan mode choise
	sigc::signal<void> on_middle_button_click_;//!< On middle button click

	/** 
	 * A constructor
	 * @param mainwin - window widget which we should control
	 */
	TrayIcon(GtkWidget *mainwin) : mainwin_(mainwin) {}
	virtual ~TrayIcon() {}
	/**
	 * Change state of object
	 * @param is_on - turn on or turn off scan mode
	 */
	virtual void set_scan_mode(bool is_on) = 0;
	/**
	 * Noramlly it is called when you do not want show current state
	 */
	virtual void hide_state() = 0;
	//! Minimize controlled widget to tray
	virtual void minimize_to_tray() = 0;	
	//! Maximize controlled widget from tray
	virtual void maximize_from_tray() = 0;
	/**
	 * Connect slot with signal which happend when window maximized by user
	 * @param s - slot
	 */
	void connect_on_maximize(const sigc::slot<void>& s) {
		on_maximize_.connect(s);
	}
protected:
	GtkWidget *mainwin_;//!< Window widget which we should control
	sigc::signal<void> on_maximize_;
};

#endif//!TRAYICON_HPP
