#include "../stardict.h"
#include "hotkey.h"

HWND Hotkey::Create_hiddenwin()
{
	WNDCLASSEX wcex;
	TCHAR wname[32];

	strcpy(wname, "StarDictHotkey");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)hotkey_mainmsg_handler;
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

void Hotkey::ToggleScan()
{
	conf->set_bool("/apps/stardict/preferences/dictionary/scan_selection",
					!conf->get_bool("/apps/stardict/preferences/dictionary/scan_selection"));
}

void Hotkey::ShowMainwindow()
{
#ifdef _WIN32
	if (!GTK_WIDGET_VISIBLE(gpAppFrame->window))
		gpAppFrame->oDockLet.stardict_systray_maximize(gpAppFrame->window);
#endif
	gtk_window_present(GTK_WINDOW(gpAppFrame->window));
	if (gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry))[0]) {
		gtk_widget_grab_focus(gpAppFrame->oMidWin.oTextWin.view->Widget()); //so user can input word directly.
	} else {
		gtk_widget_grab_focus(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry); //this won't change selection text.
	}
}

LRESULT CALLBACK Hotkey::hotkey_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
		case WM_HOTKEY:
		{
			UINT fuModifiers = (UINT) LOWORD(lparam);
			UINT uVirtKey = (UINT) HIWORD(lparam);
			if ((fuModifiers == MOD_CONTROL | MOD_ALT)&&(uVirtKey == VK_F1)) {
				ToggleScan();
			}
			else if ((fuModifiers == MOD_CONTROL | MOD_ALT)&&(uVirtKey == 'Z')) {
				ShowMainwindow();
			}
			break;
		}
		default:
			/*nothing*/;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

Hotkey::Hotkey()
{
	ScanHotkeyRegistered = false;
	MainWindowHotkeyRegistered = false;
}

void Hotkey::Init()
{
	ServerWND = Create_hiddenwin();
	ScanHotKeyId = GlobalAddAtom("StarDictScanHotKey");
	MainWindowHotKeyId = GlobalAddAtom("StarDictMainWindowHotKey");
}

void Hotkey::End()
{
	stop_scan();
	GlobalDeleteAtom(ScanHotKeyId);
	stop_mainwindow();
	GlobalDeleteAtom(MainWindowHotKeyId);
	DestroyWindow(ServerWND);
}

void Hotkey::start_scan()
{
	if (!ScanHotkeyRegistered) {
		ScanHotkeyRegistered = RegisterHotKey(ServerWND, ScanHotKeyId - 0xC000, MOD_CONTROL | MOD_ALT, VK_F1);
	}
}

void Hotkey::stop_scan()
{
	if (ScanHotkeyRegistered) {
		UnregisterHotKey(ServerWND, ScanHotKeyId - 0xC000);
		ScanHotkeyRegistered = false;
	}
}

void Hotkey::start_mainwindow()
{
	if (!MainWindowHotkeyRegistered) {
		MainWindowHotkeyRegistered = RegisterHotKey(ServerWND, MainWindowHotKeyId - 0xC000, MOD_CONTROL | MOD_ALT, 'Z');
	}
}

void Hotkey::stop_mainwindow()
{
	if (MainWindowHotkeyRegistered) {
		UnregisterHotKey(ServerWND, MainWindowHotKeyId - 0xC000);
		MainWindowHotkeyRegistered = false;
	}
}
