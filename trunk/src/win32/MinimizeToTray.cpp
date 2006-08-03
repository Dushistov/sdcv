/* MinimizeToTray
 *
 * A couple of routines to show how to make it produce a custom caption
 * animation to make it look like we are minimizing to and maximizing 
 * from the system tray
 *
 * These routines are public domain, but it would be nice if you dropped
 * me a line if you use them!
 *
 * 1.0 29.06.2000 Initial version
 * 1.1 01.07.2000 The window retains it's place in the Z-order of windows
 *     when minimized/hidden. This means that when restored/shown, it doen't
 *     always appear as the foreground window unless we call SetForegroundWindow
 *
 * Copyright 2000 Matthew Ellis <m.t.ellis@bigfoot.com>
 */
#include "stdafx.h"

#ifndef IDANI_OPEN
#define IDANI_OPEN 1
#endif
#ifndef IDANI_CLOSE
#define IDANI_CLOSE 2
#endif
#ifndef IDANI_CAPTION
#define IDANI_CAPTION 3
#endif

#define DEFAULT_RECT_WIDTH 150
#define DEFAULT_RECT_HEIGHT 30

static void GetTrayWndRect(LPRECT lpTrayRect)
{
  APPBARDATA appBarData;
  HWND hShellTrayWnd=FindWindowEx(NULL,NULL,TEXT("Shell_TrayWnd"),NULL);

  if(hShellTrayWnd)
  {
    HWND hTrayNotifyWnd=FindWindowEx(hShellTrayWnd,NULL,TEXT("TrayNotifyWnd"),NULL);
    if(hTrayNotifyWnd)
    {
      GetWindowRect(hTrayNotifyWnd,lpTrayRect);
      return;
    }
  }

  appBarData.cbSize=sizeof(appBarData);
  if(SHAppBarMessage(ABM_GETTASKBARPOS,&appBarData))
  {
    switch(appBarData.uEdge)
    {
      case ABE_LEFT:
      case ABE_RIGHT:
	lpTrayRect->top=appBarData.rc.bottom-100;
	lpTrayRect->bottom=appBarData.rc.bottom-16;
	lpTrayRect->left=appBarData.rc.left;
	lpTrayRect->right=appBarData.rc.right;
	break;

      case ABE_TOP:
      case ABE_BOTTOM:
	lpTrayRect->top=appBarData.rc.top;
	lpTrayRect->bottom=appBarData.rc.bottom;
	lpTrayRect->left=appBarData.rc.right-100;
	lpTrayRect->right=appBarData.rc.right-16;
	break;
    }

    return;
  }

  hShellTrayWnd=FindWindowEx(NULL,NULL,TEXT("Shell_TrayWnd"),NULL);
  if(hShellTrayWnd)
  {
    GetWindowRect(hShellTrayWnd,lpTrayRect);
    if(lpTrayRect->right-lpTrayRect->left>DEFAULT_RECT_WIDTH)
      lpTrayRect->left=lpTrayRect->right-DEFAULT_RECT_WIDTH;
    if(lpTrayRect->bottom-lpTrayRect->top>DEFAULT_RECT_HEIGHT)
      lpTrayRect->top=lpTrayRect->bottom-DEFAULT_RECT_HEIGHT;

    return;
  }

  SystemParametersInfo(SPI_GETWORKAREA,0,lpTrayRect,0);
  lpTrayRect->left=lpTrayRect->right-DEFAULT_RECT_WIDTH;
  lpTrayRect->top=lpTrayRect->bottom-DEFAULT_RECT_HEIGHT;
}

static int GetDoAnimateMinimize(void)
{
  ANIMATIONINFO ai;

  ai.cbSize=sizeof(ai);
  SystemParametersInfo(SPI_GETANIMATION,sizeof(ai),&ai,0);

  return ai.iMinAnimate?TRUE:FALSE;
}

void MinimizeWndToTray(HWND hWnd)
{
  if(!IsWindowVisible(hWnd)) 
    return;
  if(GetDoAnimateMinimize())
  {
    RECT rcFrom,rcTo;

    GetWindowRect(hWnd,&rcFrom);
    GetTrayWndRect(&rcTo);

    DrawAnimatedRects(hWnd,IDANI_CAPTION,&rcFrom,&rcTo);
  }

  ShowWindow(hWnd,SW_HIDE);
}

void RestoreWndFromTray(HWND hWnd)
{
  if(IsWindowVisible(hWnd)) 
    return;
  if(GetDoAnimateMinimize())
  {
    RECT rcFrom,rcTo;
    GetTrayWndRect(&rcFrom);
    GetWindowRect(hWnd,&rcTo);

    DrawAnimatedRects(hWnd,IDANI_CAPTION,&rcFrom,&rcTo);
  }

  ShowWindow(hWnd,SW_SHOW);
  SetActiveWindow(hWnd);
  SetForegroundWindow(hWnd);
}



