#ifndef __SD_HOTKEY_H__
#define __SD_HOTKEY_H__

#include <windows.h>

class Hotkey
{
private:
	bool ScanHotkeyRegistered;
	bool MainWindowHotkeyRegistered;
	int ScanHotKeyId;
	int MainWindowHotKeyId;
	HWND ServerWND;
	HWND Create_hiddenwin();
	static void ToggleScan();
	static void ShowMainwindow();
	static LRESULT CALLBACK hotkey_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	Hotkey();
	void Init();
	void End();
	void start_scan();
	void stop_scan();
	void start_mainwindow();
	void stop_mainwindow();
};

#endif
