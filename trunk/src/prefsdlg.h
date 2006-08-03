#ifndef __SD_PREFS_DLG_H__
#define __SD_PREFS_DLG_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <list>

class PrefsDlg {
private:
  GtkWidget *window;
  GtkWidget *notebook;
#ifndef CONFIG_GPE
  GtkWidget *categories_tree;
  GtkTreeModel *categories_tree_model;
#endif
  GtkWidget *custom_font_hbox;
  GtkWidget *scan_modifier_key_vbox;
  GtkWidget *collation_hbox;
  GtkWidget *tts_textview;
  GtkWidget *searchwebsite_treeview;

  GtkWindow *parent_window;
#ifndef CONFIG_GPE
  GdkPixbuf *stardict_logo;
#endif
  GtkEntry *eExportFile;
#if defined(CONFIG_GTK) || defined(CONFIG_GPE)
  GtkEntry *ePlayCommand;
#endif
#ifndef CONFIG_GPE
  GtkWidget *categories_window;
#endif
	const std::list<std::string>& key_combs;

#ifndef CONFIG_GPE
  GtkTreeModel* create_categories_tree_model ();		
  void create_categories_tree(void);
  void setup_logo_page();
#endif
  void setup_dictionary_scan_page();
  void setup_dictionary_font_page();
  void setup_dictionary_cache_page();
  void setup_dictionary_export_page();
  void setup_dictionary_sound_page();
  void setup_mainwin_input_page();
  void setup_mainwin_options_page();
  void setup_mainwin_searchwebsite_page();
  void setup_NotificationAreaIcon_options_page();
  void setup_floatwin_options_page();
#ifndef CONFIG_GPE
  void setup_floatwin_size_page();
#endif
  GtkWidget* create_notebook ();
  
  void write_mainwin_searchwebsite_list();
  
  static void response_handler (GtkDialog *dialog, gint res_id, PrefsDlg *oPrefsDlg);
#ifndef CONFIG_GPE
  static void categories_tree_selection_cb (GtkTreeSelection *selection, PrefsDlg *oPrefsDlg);
  static gboolean selection_init (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, PrefsDlg *oPrefsDlg);
  static void categories_tree_realize (GtkWidget *widget, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_dictionary_font_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_font_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_scan_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#ifdef _WIN32
  static void on_setup_dictionary_scan_clipboard_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_use_scan_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_dictionary_scan_optionmenu_changed(GtkOptionMenu *option_menu, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_scan_hide_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_cache_CreateCacheFile_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_cache_EnableCollation_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_collation_optionmenu_changed(GtkOptionMenu *option_menu, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_cache_cleanbutton_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_export_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_export_browse_button_clicked(GtkButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_sound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchWhileTyping_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_showfirstWhenNotfound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_startup_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#ifdef _WIN32
  static void on_setup_mainwin_autorun_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_use_mainwindow_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_mainwin_searchwebsite_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_moveup_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_movedown_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_add_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_remove_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_NotificationAreaIcon_QueryInFloatWin_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_floatwin_pronounce_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
	static void on_setup_show_float_if_not_found(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#ifndef CONFIG_GPE
  static void on_setup_floatwin_size_max_width_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_floatwin_size_max_height_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg);

  void resize_categories_tree(void);
#endif

  void change_font_for_all_widgets(const std::string& fontname);
public:
  PrefsDlg(GtkWindow *parent, GdkPixbuf *logo, const std::list<std::string>& key_combs);
  bool ShowModal();
  void Close();
};

#endif
