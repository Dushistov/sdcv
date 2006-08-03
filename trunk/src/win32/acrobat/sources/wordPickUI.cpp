/*********************************************************************
 * 
 * This file part of WordPick - A Adobe Acrobat/Reader plugin for text
 * extraction by mouse click.
 * 2006 Dewolf Xue <deWolf_maTri_X@msn.com>
 * 2006 Hu Zheng <huzheng_001@163.com>
 *
 * This plugin is special for StarDict (http://stardict.sourceforge.net)
 *
 * Compile under Acrobat SDK 7 + M$ VS 2003.
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
 *
*********************************************************************/
//#include <windows.h>
//#include <process.h>
#include "PIHeaders.h"
#include "PIRequir.h"
#include "PIMain.h" // for gHINSTANCE
#include "wordPickUI.h"
#include "ThTypes.h"


/*-------------------------------------------------------
	Constants/Declarations
-------------------------------------------------------*/

char buffer[128];
static bool boolPickWords = false;

//extern ASAtom WordPick_K;
extern AVToolButton handButton;

static AVToolRec WordPickTool;
static AVIdleProc idleProc;
static AVPageViewCursorProc cursorProc;
//ASBool gWordPickToolSelected;
//static AVCursor WordPickCursor;
//static AVCursor orgSysCursor;
static AVToolButton wordPickToolButton;

static AVMenuItem menuItem = NULL;

// UI Callbacks
static AVExecuteProc		cbActivateTool;
static AVComputeEnabledProc	cbIsEnabled;
static AVComputeMarkedProc	cbIsMarked;

// For mac
#define CURSWordPickCursor			150
#define SICNWordPickToolIcon		150
#define cicnWordPickIcon			150

// For windows
#define IDC_CURSOR1                 101
#define IDI_ICON1                   102
#define IDB_BITMAP1                 103
#define IDB_BITMAP2                 104

const int MOUSEOVER_INTERVAL = 300;
const int WM_MY_SHOW_TRANSLATION = WM_USER + 300;
	
static AVDevCoord oldxpoint=-1, oldypoint=-1;
static AVCursor HandCursor=NULL;

/*-------------------------------------------------------
	Utility Methods
-------------------------------------------------------*/

void *GetToolButtonIcon(void)
{
	return (AVCursor)LoadBitmapA(gHINSTANCE, MAKEINTRESOURCE(IDB_BITMAP1));
}
static ACCB1 ASBool ACCB2 RedrawCurrentPage (AVDoc doc, void* clientData)
{
	AVPageView page = AVDocGetPageView(doc);
	AVPageViewInvalidateRect (page, NULL);
	AVPageViewDrawNow(page);

	return true;
}

static void RedrawAllVisibleAnnots()
{
	AVAppEnumDocs (RedrawCurrentPage, NULL);
}

/*-------------------------------------------------------
	AVTool Callbacks
-------------------------------------------------------*/
static ACCB1 ASBool ACCB2 PDTextSelectEnumTextProcCB(void* procObj, PDFont font, ASFixed size, 
											   PDColorValue color, char* text, ASInt32 textLen)
{
	if (strlen(buffer) + strlen(text) < 127)
	{
		strcat(buffer, text);
	}
	return true;
}

static ACCB1 ASBool ACCB2 IsEnabled (void *permRequired)
{
	AVDoc avDoc = AVAppGetActiveDoc();
	if (!avDoc)
		return false;
	else
	{
		PDPerms docPerms = PDDocGetPermissions(AVDocGetPDDoc(avDoc));
		return (!permRequired || (((PDPerms)permRequired & docPerms) != 0));
	}
}

static ACCB1 ASBool ACCB2 IsMarked (void *clientData)
{
	return boolPickWords;
}

static ACCB1 void ACCB2 avtiveWordPickButton(void *clientData)
{
	AVCursor tmp = AVSysGetCursor();
	AVSysSetCursor (AVSysGetStandardCursor(HAND_CURSOR));
	HandCursor = AVSysGetCursor();
	AVSysSetCursor (tmp);

	boolPickWords = !boolPickWords;
}

static void SetUpToolButton(void)
{
	void *WordPickIcon = GetToolButtonIcon();
	AVToolBar toolBar = AVAppGetToolBar();
	AVToolButton separator = AVToolBarGetButtonByName (toolBar, ASAtomFromString("Hand"));

	wordPickToolButton = AVToolButtonNew (ASAtomFromString("ADBE:WordPick"), WordPickIcon, true, false);
	AVToolButtonSetExecuteProc (wordPickToolButton, ASCallbackCreateProto(AVExecuteProc, avtiveWordPickButton), NULL);
	AVToolButtonSetComputeEnabledProc (wordPickToolButton, cbIsEnabled, (void *)pdPermEdit);
	AVToolButtonSetComputeMarkedProc (wordPickToolButton, cbIsMarked, NULL);
	AVToolButtonSetHelpText (wordPickToolButton, "Toggle StarDict");

	AVToolBarAddButton(toolBar, wordPickToolButton, true, separator);
}

void SetUpUI(void)
{
	idleProc = ASCallbackCreateProto(AVIdleProc, wordPickAVAppRegisterForPageViewIdleProc);
	AVAppRegisterIdleProc (idleProc, NULL, 30);
	
	cbIsEnabled		= ASCallbackCreateProto (AVComputeEnabledProc,	&IsEnabled);
	cbIsMarked		= ASCallbackCreateProto (AVComputeMarkedProc,	&IsMarked);

	if (ASGetConfiguration(ASAtomFromString("CanEdit")))
		SetUpToolButton();
}

void CleanUpUI(void)
{
	AVAppUnregisterIdleProc(idleProc, NULL);

	if(wordPickToolButton)
		AVToolButtonDestroy (wordPickToolButton);
}

ACCB1 void ACCB2 wordPickAVAppRegisterForPageViewIdleProc(void * clientData)
{
	if (boolPickWords)
	{
		//_beginthread(getCurWords, 0, NULL);
		getCurWords(NULL);
	}
	return;
}

static bool bIsPureEnglish(const char *str) 
{ 
	for (int i=0; str[i]!=0; i++) 
		if (!isascii(str[i]))
			return false;
	return true;	
}

void getCurWords(PVOID parm)
{
	POINT Pt;
	GetCursorPos(&Pt);
	HWND WND = WindowFromPoint(Pt);
	TCHAR wClassName[64];
	if (GetClassName(WND, wClassName, sizeof(wClassName) / sizeof(TCHAR))) 
	{
		if (strcmp(wClassName, "AVL_AVView")!=0)
			return;
	}
	AVDoc avDoc = AVAppGetActiveDoc();
	if (!avDoc)
	{
		return;
	}
	AVPageView pageView = AVDocGetPageView(avDoc);
	if (!pageView)
	{
		return;
	}
	AVCursor acur = AVSysGetCursor ();
	if (HandCursor != acur)
	{
		return;
	}
	
	AVDevCoord xpoint, ypoint;
	AVPageViewGetMousePosition (pageView, &xpoint, &ypoint);

	if (oldxpoint == xpoint && oldypoint == ypoint)
        return;
	oldxpoint = xpoint;
	oldypoint = ypoint;

	PDDoc pdDoc = AVDocGetPDDoc(avDoc);

	buffer[0] = 0;
	if (AVPageViewGetPageNum(pageView) > PDBeforeFirstPage) 
	{
		// First of all U have to capture mouse position x,y and do the following code.
		AVDevRect resultRect;
		resultRect.left   = xpoint-10;
		resultRect.right  = xpoint+10;
		resultRect.top    = ypoint;
		resultRect.bottom = ypoint-1;
		AVPageViewDrawRectOutlineWithHandles(pageView, &resultRect, true, true, NULL, NULL);

		ASFixedRect fixedRect;
		AVPageViewDeviceRectToPage(pageView,&resultRect,&fixedRect);
		PDTextSelect pdtext=PDDocCreateTextSelect (pdDoc,AVPageViewGetPageNum(pageView),&fixedRect);
		
		AVPageViewInvalidateRect (pageView, NULL);
		AVPageViewDrawNow(pageView);
		//AVDocSetSelection(avDoc,ASAtomFromString("Text"),(void *)pdtext, true);
		//AVDocShowSelection(avDoc);
		if (PDTextSelectGetRangeCount(pdtext) > 0)
		{
			PDTextSelectEnumTextProc enumProc 
				= ASCallbackCreateProto(PDTextSelectEnumTextProc, &PDTextSelectEnumTextProcCB);
			// Enumerate the text runs in PDText
			PDTextSelectEnumText(pdtext, enumProc, NULL);
			ASCallbackDestroy(enumProc);
			HWND stardictWND = FindWindow((LPCTSTR)"StarDictMouseover", NULL);
			if (stardictWND == NULL)
			{
				//didn't find the window, barf here...
				//AVAlertNote("The dict is down.\nWordPick's doing stuff.");
			}
			else
			{
				strcpy(GlobalData->CurMod.MatchedWord, buffer);
				if (bIsPureEnglish(buffer))
				{
					char *p = strchr(buffer, ' ');
					if (p)
						GlobalData->CurMod.BeginPos = p-buffer;
					else
						GlobalData->CurMod.BeginPos = 0;
				} else
				{
					GlobalData->CurMod.BeginPos = 0;
				}

				DWORD SendMsgAnswer;
				SendMessageTimeout(stardictWND, WM_MY_SHOW_TRANSLATION, 0, 0, SMTO_ABORTIFHUNG, MOUSEOVER_INTERVAL, &SendMsgAnswer);
			}
		}
	}
	return;
}