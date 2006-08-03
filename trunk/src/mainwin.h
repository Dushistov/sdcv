#ifndef __SD_MAINWIN_H__
#define __SD_MAINWIN_H__

#include <gtk/gtk.h>
#include <string>

#include "articleview.h"

const guint MAX_HISTORY_WORD_ITEM_NUM=20;
const guint MAX_BACK_WORD_ITEM_NUM=20;


const int MIN_WINDOW_WIDTH=200;
const int MIN_WINDOW_HEIGHT=100;

enum TextWinQueryResult
{
	TEXT_WIN_FOUND,
	TEXT_WIN_NOT_FOUND,
	TEXT_WIN_SHOW_FIRST,
	TEXT_WIN_FUZZY_NOT_FOUND,
	TEXT_WIN_PATTERN_NOT_FOUND,
	TEXT_WIN_TIPS, // when the word entry is empty,it will show tip
	TEXT_WIN_INFO,
	TEXT_WIN_TREEDICT,
};

enum ListWinListWordType {
	LIST_WIN_NORMAL_LIST,
	LIST_WIN_FUZZY_LIST,
	LIST_WIN_DATA_LIST,
	LIST_WIN_PATTERN_LIST,
	LIST_WIN_EMPTY
};

struct BackListData {
	gchar *word;
	gdouble adjustment_value;
};

class TopWin {
private:
	gboolean enable_change_cb;
	GList *HisList;
	GSList *BackList;
	GtkWidget *HistoryMenu;
	GtkWidget *BackMenu;

	static gint HisCompareFunc(gconstpointer a,gconstpointer b);
	static gint BackListDataCompareFunc(gconstpointer a,gconstpointer b);

	static void ClearCallback(GtkWidget *widget, TopWin *oTopWin);
	static void GoCallback(GtkWidget *widget, TopWin *oTopWin);
	static void BackCallback(GtkWidget *widget, TopWin *oTopWin);
	static void PreviousCallback(GtkWidget *widget, TopWin *oTopWin);
	static void NextCallback(GtkWidget *widget, TopWin *oTopWin);
	static void MenuCallback(GtkWidget *widget, TopWin *oTopWin);
	static gboolean on_back_button_press(GtkWidget * widget, GdkEventButton * event , TopWin *oTopWin);
	static void on_back_menu_item_activate(GtkMenuItem *menuitem, gint index);

	static void on_entry_changed(GtkEntry *entry, TopWin *oTopWin);
	static void on_entry_activate(GtkEntry *entry, TopWin *oTopWin);
	
	static void on_main_menu_preferences_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_dictmanage_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_newversion_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_help_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_about_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_quit_activate(GtkMenuItem *menuitem, TopWin *oTopWin);

	static void ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data);

	void LoadHistory(void);
	void SaveHistory(void);
public:
	GtkWidget* WordCombo;
	GtkWidget* MainMenu;

	TopWin();
	~TopWin();

	void Create(GtkWidget *vbox);
	void Destroy(void);
	void SetText(const gchar *word, bool notify=true);
	//void SetText_without_notify(const gchar *word);
	const gchar* GetText();
	void TextSelectAll();
	void GrabFocus();
	void InsertHisList(const gchar *word);
	void InsertBackList(const gchar *word = NULL);
	void do_back();
	void do_previous();
	void do_next();
	void do_menu();

	gboolean TextSelected();
	bool HasFocus() {
    return GTK_WIDGET_HAS_FOCUS(GTK_COMBO(WordCombo)->entry);
  }

};

class ListWin
{
private:
	GtkListStore *list_model;
	GtkTreeStore *tree_model;
	bool nowIsList;
	static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event, ListWin *oListWin);
	static void on_selection_changed(GtkTreeSelection *selection, ListWin *oListWin);

public:
	ListWinListWordType list_word_type;
	GtkWidget *treeview;

	ListWin();
	void Create(GtkWidget *notebook);
	void SetModel(bool isListModel);
	void Destroy();
	void Clear();
	void ReScroll();
	void InsertLast(const gchar *word);
	void SetTreeModel(std::vector<gchar *> *reslist);
};

class TreeWin {
public:
	GtkWidget *treeview;

	TreeWin() {}
	bool Create(GtkWidget *notebook);
private:
	static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event, TreeWin *oTreeWin);
	static void on_selection_changed(GtkTreeSelection *selection, TreeWin *oTreeWin);
};

class ResultWin {
public:
	GtkWidget *treeview;
	void Create(GtkWidget *notebook);
	void InsertLast(const gchar *word, const gchar *mark);
	void Clear();
private:
	static void on_selection_changed(GtkTreeSelection *selection, ResultWin *oResultWin);
};

class IndexWin {
public:
	GtkWidget *vbox;
	GtkWidget *notebook;

	ListWin oListWin;
	ResultWin oResultWin;
	TreeWin oTreeWin;

	IndexWin();
	void Create(GtkWidget *hpaned);
private:
	static void on_wazard_button_toggled(GtkToggleButton *button, IndexWin *oIndexWin);
	static void on_appendix_button_toggled(GtkToggleButton *button, IndexWin *oIndexWin);
	static void on_result_button_toggled(GtkToggleButton *button, IndexWin *oIndexWin);
};

class ToolWin {
private:

	static void ShowListCallback(GtkWidget *widget, gpointer data);
	static void HideListCallback(GtkWidget *widget, gpointer data);
	static void CopyCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void PlayCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void SaveCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void PrintCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void SearchCallback(GtkWidget *widget, ToolWin *oToolWin);
public:
	GtkWidget* ShowListButton;
	GtkWidget* HideListButton;
	GtkWidget* PronounceWordButton;

	ToolWin() {}
	~ToolWin() {}
	void Create(GtkWidget *vbox);
	void do_search();
	void do_save();
};

class TextWin {
public:
  std::string queryWord;
  std::string pronounceWord;
  TextWinQueryResult query_result;
  std::auto_ptr<ArticleView> view;
	gboolean search_from_beginning;
	std::string find_text;
	GtkEntry *eSearch;

  TextWin() {}
  ~TextWin() {}

  void Create(GtkWidget *vbox);
  void ShowTips();
  void ShowInfo();
  void ShowInitFailed();
  void Show(const gchar *str);
  void Show(gchar ***Word, gchar ****WordData);
  void ShowTreeDictData(gchar *data);
  gboolean Find (const gchar *text, gboolean start);
	bool IsSearchPanelHasFocus() { return GTK_WIDGET_HAS_FOCUS(eSearch); } 

	void ShowSearchPanel();

private:
  GtkButton *btClose;
  GtkButton *btFind;
  GtkWidget *hbSearchPanel;
	

	void HideSearchPanel(void) 
	{
		gtk_widget_hide_all(hbSearchPanel);
	}


  static void SelectionCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, TextWin *oTextWin);
  static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event, TextWin *oTextWin);
  static bool find_first_tag(gchar *str, gchar * & beg, gchar * & end);
	static void OnCloseSearchPanel(GtkWidget *widget, TextWin *oTextWin);
	static gboolean OnSearchKeyPress(GtkWidget *widget, GdkEventKey *event,
																	 TextWin *oTextWin);
	static void OnFindSearchPanel(GtkWidget *widget, TextWin *oTextWin);
};

class MidWin {
public:
	GtkWidget* hpaned;

	IndexWin oIndexWin;
	ToolWin oToolWin;
	TextWin oTextWin;

	MidWin() {}
	void Create(GtkWidget *vbox);
};

class BottomWin {
private:
	static void ScanCallback(GtkToggleButton *button, gpointer data);
	static void AboutCallback(GtkButton *button, gpointer data);
	static void QuitCallback(GtkButton *button, gpointer data);
	static void InternetSearchCallback(GtkButton *button, BottomWin *oBottomWin);
	static void NewVersionCallback(GtkButton *button, BottomWin *oBottomWin);
	static void DictManageCallback(GtkButton *button, BottomWin *oBottomWin);
	static void PreferenceCallback(GtkButton *button, BottomWin *oBottomWin);
	static gboolean on_internetsearch_button_press(GtkWidget * widget, GdkEventButton * event , BottomWin *oBottomWin);
	static void on_internetsearch_menu_item_activate(GtkMenuItem *menuitem, const gchar *website);

public:
	GtkWidget* ScanSelectionCheckButton;
	GtkWidget* SearchWebsiteMenu;

	BottomWin();
	void Create(GtkWidget *vbox);
};

#endif
