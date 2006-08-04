#include <gdk/gdkwin32.h>

#include <glib/gi18n.h>
#include "resource.h"
#include "MinimizeToTray.h"
#include "../stardict.h"
#include "../conf.h"


#include "systray.h"

#define WM_TRAYMESSAGE WM_USER /* User defined WM Message */

enum SYSTRAY_CMND {
	SYSTRAY_CMND_MENU_QUIT=100,
	SYSTRAY_CMND_MENU_SCAN,
};


DockLet::DockLet()
{
	current_icon = DOCKLET_NORMAL_ICON;
}

void DockLet::init()
{
	systray_hwnd = systray_create_hiddenwin();

	systray_create_menu();

	/* Load icons, and init systray notify icon */
	sysicon_normal = (HICON)LoadImage(stardictexe_hInstance, MAKEINTRESOURCE(STARDICT_NORMAL_TRAY_ICON), IMAGE_ICON, 16, 16, 0);
	sysicon_scan = (HICON)LoadImage(stardictexe_hInstance, MAKEINTRESOURCE(STARDICT_SCAN_TRAY_ICON), IMAGE_ICON, 16, 16, 0);
	sysicon_stop = (HICON)LoadImage(stardictexe_hInstance, MAKEINTRESOURCE(STARDICT_STOP_TRAY_ICON), IMAGE_ICON, 16, 16, 0);

	/* Create icon in systray */
	systray_init_icon(systray_hwnd, sysicon_normal);
}

/* Create hidden window to process systray messages */
HWND DockLet::systray_create_hiddenwin()
{
	WNDCLASSEX wcex;
	TCHAR wname[32];

	strcpy(wname, "StarDictSystray");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)systray_mainmsg_handler;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= stardictexe_hInstance;
	wcex.hIcon		= NULL;
	wcex.hCursor		= NULL,
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= wname;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	// Create the window
	return (CreateWindow(wname, "", 0, 0, 0, 0, 0, GetDesktopWindow(), NULL, stardictexe_hInstance, 0));
}

void DockLet::systray_create_menu()
{
	char* locenc=NULL;

	/* create popup menu */
	if((systray_menu = CreatePopupMenu())) {
		AppendMenu(systray_menu, MF_CHECKED, SYSTRAY_CMND_MENU_SCAN,
			       (locenc=g_locale_from_utf8(_("Scan"), -1, NULL, NULL, NULL)));
		g_free(locenc);
		AppendMenu(systray_menu, MF_SEPARATOR, 0, 0);
		AppendMenu(systray_menu, MF_STRING, SYSTRAY_CMND_MENU_QUIT,
			       (locenc=g_locale_from_utf8(_("Quit"), -1, NULL, NULL, NULL)));
		g_free(locenc);
	}
}

void DockLet::systray_show_menu(int x, int y)
{
	/* need to call this so that the menu disappears if clicking outside
           of the menu scope */
	SetForegroundWindow(systray_hwnd);

  if (conf->get_bool_at("dictionary/scan_selection"))
		CheckMenuItem(systray_menu, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(systray_menu, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_UNCHECKED);

	TrackPopupMenu(systray_menu,         // handle to shortcut menu
		       TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
		       x,                   // horizontal position, in screen coordinates
		       y,                   // vertical position, in screen coordinates
		       0,                   // reserved, must be zero
		       systray_hwnd,        // handle to owner window
		       NULL                 // ignored
		       );
}

LRESULT CALLBACK DockLet::systray_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static UINT taskbarRestartMsg; /* static here means value is kept across multiple calls to this func */

	switch (msg) {
	case WM_CREATE:
		taskbarRestartMsg = RegisterWindowMessage("TaskbarCreated");
		break;
		
	case WM_COMMAND:
		switch(LOWORD(wparam)) {
		case SYSTRAY_CMND_MENU_SCAN:
			if (GetMenuState(gpAppFrame->oDockLet.systray_menu, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND) & MF_CHECKED) {
				conf->set_bool_at("dictionary/scan_selection", FALSE);
			} else {
				conf->set_bool_at("dictionary/scan_selection", TRUE);
			}				
			break;
		case SYSTRAY_CMND_MENU_QUIT:
			gpAppFrame->Quit();
			break;
		}
		break;
	case WM_TRAYMESSAGE:
	{
		if ( lparam == WM_LBUTTONDOWN ) {
			if (GetKeyState(VK_CONTROL)<0) {
				conf->set_bool_at("dictionary/scan_selection", !conf->get_bool_at("dictionary/scan_selection"));
			}
		} else if ( lparam == WM_LBUTTONDBLCLK ) {
			// Only use left button will conflict with the menu.
			if (GTK_WIDGET_VISIBLE(gpAppFrame->window)) {
				HWND main_window = static_cast<HWND>(GDK_WINDOW_HWND(gpAppFrame->window->window));
				if (IsIconic(main_window)) {
					ShowWindow(main_window,SW_RESTORE);
				//} else if (GetForegroundWindow() != main_window) {
					//SetForegroundWindow(main_window);
				} else {
					stardict_systray_minimize(gpAppFrame->window);
					gtk_widget_hide(gpAppFrame->window);
				}
			} else {
				stardict_systray_maximize(gpAppFrame->window);
				gtk_window_present(GTK_WINDOW(gpAppFrame->window));
				if (gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry))[0]) {
					gtk_widget_grab_focus(gpAppFrame->oMidWin.oTextWin.view->Widget()); //so user can input word directly.
				} else {
					gtk_widget_grab_focus(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry); //this won't change selection text.
				}
			}
		} else if (lparam == WM_MBUTTONDOWN) {
			if (conf->get_bool_at("notification_area_icon/query_in_floatwin")) {
				gpAppFrame->oSelection.LastClipWord.clear();
				gtk_selection_convert (gpAppFrame->oSelection.selection_widget, GDK_SELECTION_PRIMARY, gpAppFrame->oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
			} else {
				stardict_systray_maximize(gpAppFrame->window);
				gtk_window_present(GTK_WINDOW(gpAppFrame->window));
				gtk_selection_convert (gpAppFrame->oMidWin.oTextWin.view->Widget(), GDK_SELECTION_PRIMARY, gpAppFrame->oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
			}	
		} else if (lparam == WM_RBUTTONUP) {
			/* Right Click */
			POINT mpoint;
			GetCursorPos(&mpoint);

			gpAppFrame->oDockLet.systray_show_menu(mpoint.x, mpoint.y);
		}
		break;
	}
	default: 
		if (msg == taskbarRestartMsg) {
			/* explorer crashed and left us hanging... 
			   This will put the systray icon back in it's place, when it restarts */
			Shell_NotifyIcon(NIM_ADD,&(gpAppFrame->oDockLet.stardict_nid));
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void DockLet::systray_init_icon(HWND hWnd, HICON icon)
{
	char* locenc=NULL;

	ZeroMemory(&stardict_nid,sizeof(stardict_nid));
	stardict_nid.cbSize=sizeof(NOTIFYICONDATA);
	stardict_nid.hWnd=hWnd;
	stardict_nid.uID=0;
	stardict_nid.uFlags=NIF_ICON | NIF_MESSAGE | NIF_TIP;
	stardict_nid.uCallbackMessage=WM_TRAYMESSAGE;
	stardict_nid.hIcon=icon;
	locenc=g_locale_from_utf8(_("StarDict"), -1, NULL, NULL, NULL);
	strcpy(stardict_nid.szTip, locenc);
	g_free(locenc);
	Shell_NotifyIcon(NIM_ADD,&stardict_nid);
}

void DockLet::systray_change_icon(HICON icon, char* text)
{
	char *locenc=NULL;
	stardict_nid.hIcon = icon;
	locenc = g_locale_from_utf8(text, -1, NULL, NULL, NULL);
	lstrcpy(stardict_nid.szTip, locenc);
	g_free(locenc);
	Shell_NotifyIcon(NIM_MODIFY,&stardict_nid);
}

void DockLet::SetIcon(DockLetIconType icon_type)
{
	if (current_icon == icon_type)
		return;
	switch (icon_type) {
		case DOCKLET_NORMAL_ICON:
			systray_change_icon(sysicon_normal, _("StarDict"));
			break;
		case DOCKLET_SCAN_ICON:
			systray_change_icon(sysicon_scan, _("StarDict - Scanning"));
			break;
		case DOCKLET_STOP_ICON:
			systray_change_icon(sysicon_stop, _("StarDict - Stopped"));
			break;
	}
	current_icon = icon_type;
}

void DockLet::cleanup()
{
	Shell_NotifyIcon(NIM_DELETE,&stardict_nid);	
	DestroyMenu(systray_menu);
	DestroyWindow(systray_hwnd);
}

void DockLet::stardict_systray_minimize( GtkWidget *window )
{
	MinimizeWndToTray((HWND)(GDK_WINDOW_HWND(window->window)));
}

void DockLet::stardict_systray_maximize( GtkWidget *window )
{
	RestoreWndFromTray((HWND)(GDK_WINDOW_HWND(window->window)));
}
