#include <gdk/gdkwin32.h>

#include <glib/gi18n.h>
#include "resource.h"
#include "MinimizeToTray.h"

#include "systray.h"

#define WM_TRAYMESSAGE WM_USER /* User defined WM Message */

enum SYSTRAY_CMND {
	SYSTRAY_CMND_MENU_QUIT=100,
	SYSTRAY_CMND_MENU_SCAN,
};

extern HINSTANCE stardictexe_hInstance;

DockLet::DockLet(GtkWidget *mainwin, bool is_scan_on) : TrayIcon(mainwin)
{
	cur_state_ = is_scan_on;
	hide_state_ = false;

	hwnd_ = create_hiddenwin();
	::SetWindowLongPtr(hwnd_, GWL_USERDATA,
			   reinterpret_cast<LONG_PTR>(this));

	create_menu();

	/* Load icons, and init systray notify icon */
	normal_icon_ = (HICON)LoadImage(stardictexe_hInstance, MAKEINTRESOURCE(STARDICT_NORMAL_TRAY_ICON), IMAGE_ICON, 16, 16, 0);
	scan_icon_ = (HICON)LoadImage(stardictexe_hInstance, MAKEINTRESOURCE(STARDICT_SCAN_TRAY_ICON), IMAGE_ICON, 16, 16, 0);
	stop_icon_ = (HICON)LoadImage(stardictexe_hInstance, MAKEINTRESOURCE(STARDICT_STOP_TRAY_ICON), IMAGE_ICON, 16, 16, 0);

	/* Create icon in systray */
	if (is_scan_on)
		init_icon(hwnd_, scan_icon_, _("StarDict - Scanning"));
	else
		init_icon(hwnd_, stop_icon_, _("StarDict - Stopped"));
}

/* Create hidden window to process systray messages */
HWND DockLet::create_hiddenwin()
{
	WNDCLASSEX wcex;
	TCHAR wname[] = TEXT("StarDictSystray");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)mainmsg_handler;
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

void DockLet::create_menu()
{
	char* locenc = NULL;

	/* create popup menu */
	if ((menu_ = CreatePopupMenu())) {
		AppendMenu(menu_, MF_CHECKED, SYSTRAY_CMND_MENU_SCAN,
			   (locenc = g_locale_from_utf8(_("Scan"), -1, NULL, NULL, NULL)));
		g_free(locenc);
		AppendMenu(menu_, MF_SEPARATOR, 0, 0);
		AppendMenu(menu_, MF_STRING, SYSTRAY_CMND_MENU_QUIT,
			   (locenc = g_locale_from_utf8(_("Quit"), -1, NULL, NULL, NULL)));
		g_free(locenc);
	}
}

void DockLet::show_menu(int x, int y)
{
	/* need to call this so that the menu disappears if clicking outside
           of the menu scope */
	SetForegroundWindow(hwnd_);

	if (cur_state_)
		CheckMenuItem(menu_, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(menu_, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_UNCHECKED);

	TrackPopupMenu(menu_,         // handle to shortcut menu
		       TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
		       x,                   // horizontal position, in screen coordinates
		       y,                   // vertical position, in screen coordinates
		       0,                   // reserved, must be zero
		       hwnd_,        // handle to owner window
		       NULL                 // ignored
		);
}

void DockLet::minimize_to_tray()
{	
	MinimizeWndToTray((HWND)(GDK_WINDOW_HWND(mainwin_->window)));
	gtk_widget_hide(mainwin_);
}

void DockLet::maximize_from_tray()
{
	if (!GTK_WIDGET_VISIBLE(mainwin_))		
		RestoreWndFromTray((HWND)(GDK_WINDOW_HWND(mainwin_->window)));
	gtk_window_present(GTK_WINDOW(mainwin_));
}

void DockLet::on_left_btn()
{
	if (GTK_WIDGET_VISIBLE(mainwin_)) {
		HWND hwnd = static_cast<HWND>(GDK_WINDOW_HWND(mainwin_->window));
		if (IsIconic(hwnd))
			ShowWindow(hwnd, SW_RESTORE);
		else
			minimize_to_tray();
	} else {
		maximize_from_tray();
		on_maximize_.emit();
	}
}

LRESULT CALLBACK DockLet::mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

			if (GetMenuState(dock->menu_, SYSTRAY_CMND_MENU_SCAN,
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
				dock->on_change_scan_.emit(!dock->cur_state_);
		} else if ( lparam == WM_LBUTTONDBLCLK ) {
			// Only use left button will conflict with the menu.
			dock->on_left_btn();
		} else if (lparam == WM_MBUTTONDOWN) {
			dock->on_middle_button_click_.emit();
		} else if (lparam == WM_RBUTTONUP) {
			/* Right Click */
			POINT mpoint;
			GetCursorPos(&mpoint);

			dock->show_menu(mpoint.x, mpoint.y);
		}
		break;
	}
	default:
		if (msg == taskbarRestartMsg) {
			/* explorer crashed and left us hanging...
			   This will put the systray icon back in it's place, when it restarts */
			Shell_NotifyIcon(NIM_ADD, &(dock->nid_));
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void DockLet::init_icon(HWND hWnd, HICON icon, const char *text)
{
	char* locenc=NULL;

	ZeroMemory(&nid_, sizeof(nid_));
	nid_.cbSize = sizeof(NOTIFYICONDATA);
	nid_.hWnd = hWnd;
	nid_.uID = 0;
	nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid_.uCallbackMessage = WM_TRAYMESSAGE;
	nid_.hIcon = icon;
	locenc = g_locale_from_utf8(text, -1, NULL, NULL, NULL);
	strcpy(nid_.szTip, locenc);
	g_free(locenc);
	Shell_NotifyIcon(NIM_ADD, &nid_);
}

void DockLet::change_icon(HICON icon, const char* text)
{
	char *locenc = NULL;
	nid_.hIcon = icon;
	locenc = g_locale_from_utf8(text, -1, NULL, NULL, NULL);
	lstrcpy(nid_.szTip, locenc);
	g_free(locenc);
	Shell_NotifyIcon(NIM_MODIFY, &nid_);
}

void DockLet::set_scan_mode(bool is_on)
{	
	if (!hide_state_ && is_on == cur_state_)
		return;

	hide_state_ = false;

	if (is_on)	
		change_icon(scan_icon_, _("StarDict - Scanning"));
	else
		change_icon(stop_icon_, _("StarDict - Stopped"));
	
	cur_state_ = is_on;
}

void DockLet::hide_state()
{
	if (hide_state_)
		return;
	change_icon(normal_icon_, _("StarDict"));
	hide_state_ = true;
}

DockLet::~DockLet()
{
	Shell_NotifyIcon(NIM_DELETE, &nid_);
	DestroyMenu(menu_);
	DestroyWindow(hwnd_);
}