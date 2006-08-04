#include <gdk/gdkwin32.h>

#include <glib/gi18n.h>
#include "resource.h"
#include "MinimizeToTray.h"
#include "../conf.h"

#include "systray.h"

#define WM_TRAYMESSAGE WM_USER /* User defined WM Message */

enum SYSTRAY_CMND {
	SYSTRAY_CMND_MENU_QUIT=100,
	SYSTRAY_CMND_MENU_SCAN,
};

extern HINSTANCE stardictexe_hInstance;

DockLet::DockLet(GtkWidget *mainwin) : TrayIcon(mainwin)
{
	cur_state_ = NORMAL_ICON;

	systray_hwnd = systray_create_hiddenwin();
	::SetWindowLongPtr(systray_hwnd, GWL_USERDATA,
			reinterpret_cast<LONG_PTR>(this));

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
	TCHAR wname[] = TEXT("StarDictSystray");

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
	return CreateWindow(wname, "", 0, 0, 0, 0, 0, GetDesktopWindow(), NULL, stardictexe_hInstance, 0);
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

void DockLet::minimize_to_tray()
{
	stardict_systray_minimize(mainwin_);
	gtk_widget_hide(mainwin_);
}

void DockLet::maximize_from_tray()
{
	stardict_systray_maximize(mainwin_);	
}

void DockLet::on_left_btn()
{
	if (GTK_WIDGET_VISIBLE(mainwin_)) {	
		HWND hwnd = static_cast<HWND>(GDK_WINDOW_HWND(mainwin_->window));
		if (IsIconic(hwnd))	
			ShowWindow(hwnd, SW_RESTORE);		
		else
			minimize_to_tray();	
	} else
		on_maximize_.emit();	
}

LRESULT CALLBACK DockLet::systray_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static UINT taskbarRestartMsg; /* static here means value is kept across multiple calls to this func */
	DockLet *dock =
			reinterpret_cast<DockLet *>(::GetWindowLongPtr(hwnd, GWL_USERDATA));

	switch (msg) {
	case WM_CREATE:
		taskbarRestartMsg = RegisterWindowMessage("TaskbarCreated");
		break;
		
	case WM_COMMAND:
		switch(LOWORD(wparam)) {
		case SYSTRAY_CMND_MENU_SCAN:
				
			if (GetMenuState(dock->systray_menu, SYSTRAY_CMND_MENU_SCAN,
					MF_BYCOMMAND) & MF_CHECKED)				
				dock->on_change_scan_.emit(false);
			else				
				dock->on_change_scan_.emit(true);						
			break;
		case SYSTRAY_CMND_MENU_QUIT:
			dock->on_quit_.emit();
			break;
		}
		break;
	case WM_TRAYMESSAGE:
	{
		if ( lparam == WM_LBUTTONDOWN ) {
			if (GetKeyState(VK_CONTROL)<0)
				dock->on_change_scan_.emit(!conf->get_bool_at("dictionary/scan_selection"));			
		} else if ( lparam == WM_LBUTTONDBLCLK ) {
			// Only use left button will conflict with the menu.
			dock->on_left_btn();
		} else if (lparam == WM_MBUTTONDOWN) {
			dock->on_middle_button_click_.emit();			
		} else if (lparam == WM_RBUTTONUP) {
			/* Right Click */
			POINT mpoint;
			GetCursorPos(&mpoint);

			dock->systray_show_menu(mpoint.x, mpoint.y);
		}
		break;
	}
	default: 
		if (msg == taskbarRestartMsg) {
			/* explorer crashed and left us hanging... 
			   This will put the systray icon back in it's place, when it restarts */
			Shell_NotifyIcon(NIM_ADD, &(dock->stardict_nid));
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

void DockLet::set_state(State new_state)
{
	if (cur_state_ == new_state)
		return;

	switch (new_state) {
	case NORMAL_ICON:
		systray_change_icon(sysicon_normal, _("StarDict"));
		break;
	case SCAN_ICON:
		systray_change_icon(sysicon_scan, _("StarDict - Scanning"));
		break;
	case STOP_ICON:
		systray_change_icon(sysicon_stop, _("StarDict - Stopped"));
		break;
	}
	cur_state_ = new_state;
}

DockLet::~DockLet()
{
	Shell_NotifyIcon(NIM_DELETE, &stardict_nid);	
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
