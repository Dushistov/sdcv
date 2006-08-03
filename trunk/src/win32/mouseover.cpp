/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 *
 * Copyright (C) 2006 Hu Zheng <huzheng_001@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "../stardict.h"

#include "mouseover.h"
#include "ThTypes.h"

// StarDict's Mouseover feature get the example delphi source code from Mueller Electronic Dicionary.
// Homepage: http://vertal1.narod.ru/mueldic.html E-mail: svv_soft@mail.ru

const int WM_MY_SHOW_TRANSLATION = WM_USER + 300;

void Mouseover::NeedSpyDll()
{
	if (fSpyDLL == 0) {
		fSpyDLL = LoadLibrary((gStarDictDataDir+ G_DIR_SEPARATOR_S "TextOutSpy.dll").c_str());
		if (fSpyDLL==0) {
			fSpyDLL = (HINSTANCE)-1;
		} else {
			ActivateSpy_func = (ActivateSpy_func_t)GetProcAddress(fSpyDLL, "ActivateTextOutSpying");
		}
	}
}

HWND Mouseover::Create_hiddenwin()
{
	WNDCLASSEX wcex;
	TCHAR wname[32];

	strcpy(wname, "StarDictMouseover");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)mouseover_mainmsg_handler;
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

void Mouseover::ShowTranslation()
{
	if (!conf->get_bool("/apps/stardict/preferences/dictionary/scan_selection")) // Needed by acrobat plugin.
		return;
	if (conf->get_bool("/apps/stardict/preferences/dictionary/only_scan_while_modifier_key")) {
    	bool do_scan = gpAppFrame->unlock_keys->is_pressed();
		if (!do_scan)
			return;
	}
	if (g_utf8_validate(GlobalData->CurMod.MatchedWord, -1, NULL)) {
		gpAppFrame->SmartLookupToFloat(GlobalData->CurMod.MatchedWord, GlobalData->CurMod.BeginPos, true);
	} else {
		char *str1 = g_locale_to_utf8(GlobalData->CurMod.MatchedWord, GlobalData->CurMod.BeginPos, NULL, NULL, NULL);
		if (!str1)
			return;
		char *str2 = g_locale_to_utf8(GlobalData->CurMod.MatchedWord + GlobalData->CurMod.BeginPos, -1, NULL, NULL, NULL);
		if (!str2) {
			g_free(str1);
			return;
		}
		GlobalData->CurMod.BeginPos = strlen(str1);
		char *str = g_strdup_printf("%s%s", str1, str2);
		g_free(str1);
		g_free(str2);
		gpAppFrame->SmartLookupToFloat(str, GlobalData->CurMod.BeginPos, true);
		g_free(str);
	}
}

LRESULT CALLBACK Mouseover::mouseover_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
		case WM_MY_SHOW_TRANSLATION:
			ShowTranslation();
			break;
		default:
			/*nothing*/;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

Mouseover::Mouseover()
{
	fSpyDLL = 0;
	ActivateSpy_func = NULL;
}

void Mouseover::Init()
{
	ThTypes_Init();
	ZeroMemory(GlobalData, sizeof(TGlobalDLLData));
	strcpy(GlobalData->LibName, (gStarDictDataDir+ G_DIR_SEPARATOR_S "TextOutHook.dll").c_str());
	GlobalData->ServerWND = Create_hiddenwin();
}

void Mouseover::End()
{
	if ((fSpyDLL!=0)&&(fSpyDLL!=(HINSTANCE)-1)) {
		stop();
		FreeLibrary(fSpyDLL);
	}
	DestroyWindow(GlobalData->ServerWND);
	Thtypes_End();
}

void Mouseover::start()
{
	NeedSpyDll();
	if (ActivateSpy_func)
		ActivateSpy_func(true);
}

void Mouseover::stop()
{
	if (ActivateSpy_func)
		ActivateSpy_func(false);
}
