#ifndef __STAR_DICT_H__
#define __STAR_DICT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#ifdef CONFIG_GNOME
# include <bonobo/bonobo-object.h>
# include <libgnome/libgnome.h>
#endif

class TrayIcon;
class Application;

#include "conf.h"
#include "lib/lib.h"
#include "skin.h"
#include "mainwin.h"
#ifdef _WIN32
#  include "win32/clipboard.h"
#  include "win32/mouseover.h"
#  include "win32/hotkey.h"
#endif
#include "floatwin.h"
#include "selection.h"
#include "dictmanagedlg.h"
#include "prefsdlg.h"
#include "readword.h"
#include "iskeyspressed.hpp"

extern Application *gpAppFrame;

//notice!!! when you change these DEFAULT const value,remember that you'd better change data/stardict.schemas.in too!

const int MAX_FUZZY_MATCH_ITEM=100;
const int MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM=5;

const int LIST_WIN_ROW_NUM = 30; //how many words show in the list win.


class Application : public sigc::trackable {
private:
	DictManageDlg *dict_manage_dlg;
	PrefsDlg *prefs_dlg;

	static int MatchWordCompare(const void * s1, const void * s2);
	static gboolean on_delete_event(GtkWidget * window, GdkEvent *event , Application *oApplication);
	static gboolean on_window_state_event(GtkWidget * window, GdkEventWindowState *event , Application *oApplication);
	static gboolean vKeyPressReleaseCallback(GtkWidget * window, GdkEventKey *event , Application *oApplication);
	void reload_dicts();
	void on_main_win_hide_list_changed(const baseconfval*);
	void on_dict_scan_select_changed(const baseconfval*);
	void on_floatwin_lock_changed(const baseconfval*);

	void on_floatwin_lock_x_changed(const baseconfval*);
	void on_floatwin_lock_y_changed(const baseconfval*);
	void on_scan_modifier_key_changed(const baseconfval*);
	void on_change_scan(bool);
	void on_tray_middle_btn();
	void on_tray_maximize();
public:
	CurrentIndex *iCurrentIndex;
	GtkWidget *window;
	GtkTooltips *tooltips;
#ifdef CONFIG_GNOME
	BonoboObject *stardict_app_server;
#endif
	ReadWord oReadWord;

	TopWin oTopWin;
	MidWin oMidWin;
	BottomWin oBottomWin;
	std::auto_ptr<TrayIcon> tray_icon_;
	Selection oSelection;
#ifdef _WIN32
	Clipboard oClipboard;
	Mouseover oMouseover;
	Hotkey oHotkey;
#endif
	FloatWin oFloatWin;	

	Libs oLibs;
	TreeDicts oTreeDicts;
	std::auto_ptr<hotkeys> unlock_keys;
	AppSkin oAppSkin;

	Application();
	~Application();
	void Init(gchar *queryword);
	void Create(gchar *queryword);
	void End();
	void Query(const gchar *word);
	void BuildResultData(const char* sWord, CurrentIndex *iIndex, const gchar *piIndexValidStr, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound, gint Method);
	void FreeResultData(gchar ***pppWord, gchar ****ppppWordData);
	bool SimpleLookupToFloat(const gchar* sWord, bool bShowIfNotFound);
#ifdef _WIN32
	bool SmartLookupToFloat(const gchar* sWord, int BeginPos, bool bShowIfNotFound);
#endif
	bool SimpleLookupToTextWin(const gchar* sWord, CurrentIndex* piIndex, const gchar *piIndexValidStr = NULL, bool bTryMoreIfNotFound = false, bool bShowNotfound = true, bool isShowFirst = false);
	void LookupDataToMainWin(const gchar *sWord);
	void LookupWithFuzzyToMainWin(const gchar* word);
	void LookupWithFuzzyToFloatWin(const gchar * word);
	void LookupWithRuleToMainWin(const gchar* word);
	void ShowDataToTextWin(gchar ***pppWord, gchar ****ppppWordData,const gchar * sOriginWord, bool isShowFirst);
	void ShowTreeDictDataToTextWin(guint32 offset, guint32 size, gint iTreeDict);
	void ShowNotFoundToTextWin(const char* sWord,const char* sReason, TextWinQueryResult query_result);
	void ShowNotFoundToFloatWin(const char* sWord,const char* sReason, gboolean fuzzy);

	void TopWinEnterWord(const gchar *text);
	void TopWinWordChange(const gchar* sWord);
	void ListWords(const gchar *sWord, CurrentIndex* iStartIndex, bool showfirst);
	
	void ListClick(const gchar *word);
	void PopupPrefsDlg();
	void PopupDictManageDlg();
	void Quit();
};

#ifdef _WIN32
extern HINSTANCE stardictexe_hInstance;
#endif

extern gchar* GetPureEnglishAlpha(gchar *str);
extern gchar* GetHeadWord(gchar *str);
extern gboolean stardict_on_enter_notify (GtkWidget * widget, GdkEventCrossing * event, gpointer data);

#endif
