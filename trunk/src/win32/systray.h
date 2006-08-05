/*
 *  systray.h
 *
 *  Author: Herman Bloggs <hermanator12002@yahoo.com>
 *  Date: November, 2002
 *  Description: Gaim systray functionality
 *
 * Come from gaim. changed by Hu Zheng <huzheng_001@163.com> for StarDict.
 * http://stardict.sourceforge.net 2003.09.22
 *
 * Modified 2006 by Evgeniy Dushistov <dushistov@mail.ru to avoid
 * dependicies from stardict.h
 */



#ifndef _SYSTRAY_H_
#define _SYSTRAY_H_

#include <windows.h>
#include <gtk/gtk.h>

#include "trayicon.hpp"

class DockLet : public TrayIcon {
public:	
	DockLet(GtkWidget *mainwin);
	~DockLet();
	void set_state(State);
	void minimize_to_tray();
	void maximize_from_tray();		
private:
	State cur_state_;
	HWND systray_hwnd;
	HMENU systray_menu; // gtk menu don't work fine here.
	NOTIFYICONDATA stardict_nid;
	HICON sysicon_normal;
	HICON sysicon_scan;
	HICON sysicon_stop;
	
	HWND systray_create_hiddenwin();
	void systray_create_menu();
	void systray_show_menu(int x, int y);
	void systray_init_icon(HWND hWnd, HICON icon);
	void systray_change_icon(HICON icon, char* text);
	static LRESULT CALLBACK systray_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static void stardict_systray_minimize( GtkWidget* );
	static void stardict_systray_maximize( GtkWidget* );
	void on_left_btn();
};

#endif /* _SYSTRAY_H_ */
