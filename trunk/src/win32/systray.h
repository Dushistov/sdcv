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

#include "../trayicon.hpp"

class DockLet : public TrayIcon {
public:	
	DockLet(GtkWidget *, bool);
	~DockLet();	
	void set_scan_mode(bool);
	void minimize_to_tray();
	void maximize_from_tray();		
	void hide_state();
private:
	bool cur_state_;
	bool hide_state_;

	HWND hwnd_;
	HMENU menu_; // gtk menu don't work fine here.
	NOTIFYICONDATA nid_;
	HICON normal_icon_;
	HICON scan_icon_;
	HICON stop_icon_;
	
	HWND create_hiddenwin();
	void create_menu();
	void show_menu(int x, int y);
	void init_icon(HWND hWnd, HICON icon, const char *text);
	void change_icon(HICON icon, const char* text);
	static LRESULT CALLBACK mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);		
	void on_left_btn();
};

#endif /* _SYSTRAY_H_ */
