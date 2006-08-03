#ifndef __SD_MOUSEOVER_H__
#define __SD_MOUSEOVER_H__


#include <windows.h>

class Mouseover
{
private:
	typedef void (*ActivateSpy_func_t)(bool);
	ActivateSpy_func_t ActivateSpy_func;
	HINSTANCE  fSpyDLL;
	void NeedSpyDll();
	HWND Create_hiddenwin();
	static void ShowTranslation();
	static LRESULT CALLBACK mouseover_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	Mouseover();
	void Init();
	void End();
	void start();
	void stop();
};

#endif
