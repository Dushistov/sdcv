/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
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

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#endif

#include "stardict.h"
#include "conf.h"
#include "utils.h"
#include "iskeyspressed.hpp"

#include "prefsdlg.h"

#ifndef CONFIG_GPE
#define LOGO								0
#define	DICTIONARY_SCAN_SETTINGS 					1
#define	DICTIONARY_FONT_SETTINGS 					2
#define DICTIONARY_CACHE_SETTINGS					3
#define DICTIONARY_EXPORT_SETTINGS					4
#define	DICTIONARY_SOUND_SETTINGS 					5
#define	MAINWIN_INPUT_SETTINGS 						6
#define	MAINWIN_OPTIONS_SETTINGS 					7
#define MAINWIN_SEARCH_WEBSITE_SETTINGS					8
#define NOTIFICATION_AREA_ICON_OPITIONS_SETTINGS			9
#define FLOATWIN_OPTIONS_SETTINGS					10
#define FLOATWIN_SIZE_SETTINGS						11

enum
{
	CATEGORY_COLUMN = 0,
	PAGE_NUM_COLUMN,
	NUM_COLUMNS
};


struct CategoriesTreeItem {
  gchar			*category;
	
  CategoriesTreeItem 	*children;
  
  gint			notebook_page;
};

static CategoriesTreeItem dictionary_behavior [] =
{
	{N_("Scan Selection"), NULL, DICTIONARY_SCAN_SETTINGS},
	{N_("Font"), NULL, DICTIONARY_FONT_SETTINGS},
	{N_("Cache"), NULL, DICTIONARY_CACHE_SETTINGS},
	{N_("Export"), NULL, DICTIONARY_EXPORT_SETTINGS},
	{N_("Sound"), NULL, DICTIONARY_SOUND_SETTINGS},
	
	{ NULL }
};

static CategoriesTreeItem mainwin_behavior [] =
{
	{N_("Input"), NULL, MAINWIN_INPUT_SETTINGS},
	{N_("Options"), NULL, MAINWIN_OPTIONS_SETTINGS},
	{N_("Search website"), NULL, MAINWIN_SEARCH_WEBSITE_SETTINGS},
	
	{ NULL }
};

static CategoriesTreeItem NotificationAreaIcon_behavior [] =
{
	{N_("Options"), NULL, NOTIFICATION_AREA_ICON_OPITIONS_SETTINGS},
	
	{ NULL }
};

static CategoriesTreeItem floatwin_behavior [] =
{
	{N_("Options"), NULL, FLOATWIN_OPTIONS_SETTINGS},
	{N_("Size"), NULL, FLOATWIN_SIZE_SETTINGS},
	
	{ NULL }
};

static CategoriesTreeItem toplevel [] =
{
	{N_("Dictionary"), dictionary_behavior, LOGO},
	
	{N_("Main window"), mainwin_behavior, LOGO},
	
	{N_("Notification area icon"), NotificationAreaIcon_behavior, LOGO},
	
	{N_("Floating window"), floatwin_behavior, LOGO},

	{ NULL }
};

static gint last_selected_page_num = DICTIONARY_SCAN_SETTINGS;
#endif

void PrefsDlg::response_handler (GtkDialog *dialog, gint res_id, PrefsDlg *oPrefsDlg)
{
  if (res_id==GTK_RESPONSE_HELP)
    show_help("stardict-prefs");
}

#ifndef CONFIG_GPE
GtkTreeModel* PrefsDlg::create_categories_tree_model ()
{
	GtkTreeStore *model;
	GtkTreeIter iter;
	CategoriesTreeItem *category = toplevel;

	model = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	while (category->category) {
		CategoriesTreeItem *sub_category = category->children;		
		gtk_tree_store_append (model, &iter, NULL);
		gtk_tree_store_set (model, &iter, CATEGORY_COLUMN, gettext (category->category), PAGE_NUM_COLUMN, category->notebook_page, -1);

		while (sub_category->category) {
	  		GtkTreeIter child_iter;
	  		gtk_tree_store_append (model, &child_iter, &iter);
			gtk_tree_store_set (model, &child_iter,
				CATEGORY_COLUMN, gettext (sub_category->category),
				PAGE_NUM_COLUMN, sub_category->notebook_page,
			      -1);
	  		sub_category++;
		}      
		category++;
	}
	return GTK_TREE_MODEL (model);	
}

void PrefsDlg::categories_tree_selection_cb (GtkTreeSelection *selection, PrefsDlg *oPrefsDlg)
{
 	GtkTreeIter iter;
	GValue value = {0, };

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get_value (oPrefsDlg->categories_tree_model, &iter,
			    PAGE_NUM_COLUMN,
			    &value);

	last_selected_page_num = g_value_get_int (&value);

	if (oPrefsDlg->notebook != NULL)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (oPrefsDlg->notebook), 
					       last_selected_page_num);      	
	g_value_unset (&value);
}

gboolean PrefsDlg::selection_init (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, PrefsDlg *oPrefsDlg)
{
	GValue value = {0, };
	gint page_num;

	gtk_tree_model_get_value (oPrefsDlg->categories_tree_model, iter,
			    PAGE_NUM_COLUMN,
			    &value);

	page_num = g_value_get_int (&value);

	g_value_unset (&value);

	if (page_num == last_selected_page_num)
	{
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->categories_tree));

		gtk_tree_selection_select_iter (selection, iter);

		gtk_notebook_set_current_page (GTK_NOTEBOOK (oPrefsDlg->notebook), page_num);
    	
		return TRUE;
	}
	return FALSE;
}

void PrefsDlg::categories_tree_realize (GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	gtk_tree_view_expand_all(GTK_TREE_VIEW(widget));

	gtk_tree_model_foreach(oPrefsDlg->categories_tree_model, 
			       GtkTreeModelForeachFunc(selection_init),
			       oPrefsDlg);
}

void PrefsDlg::create_categories_tree(void)
{
  GtkWidget *sw;	
  GtkTreeModel *model;
  GtkWidget *treeview;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;
  gint col_offset;
  
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
				       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  
  gtk_widget_set_size_request (sw, 140, 240);
  
  model = create_categories_tree_model ();
  
  treeview = gtk_tree_view_new_with_model (model);
  g_object_unref (G_OBJECT (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
  
  categories_tree = treeview;
  categories_tree_model = model;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  
  gtk_tree_selection_set_mode (selection,
			       GTK_SELECTION_SINGLE);
  
  /* add column for category */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  
  col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
							    -1, _("Categories"),
							    renderer, "text",
							    CATEGORY_COLUMN,
							    NULL);
  
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
  
  g_signal_connect (selection, "changed", 
		    G_CALLBACK (categories_tree_selection_cb), 
		    this);
  
  gtk_container_add (GTK_CONTAINER (sw), treeview);
  
  g_signal_connect (G_OBJECT (treeview), "realize", 
		    G_CALLBACK (categories_tree_realize), 
		    this);
  
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  
  categories_window=sw;
}

void PrefsDlg::setup_logo_page()
{
  GtkWidget *image = gtk_image_new_from_pixbuf(stardict_logo);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),image,NULL);
}
#endif

void PrefsDlg::on_setup_dictionary_scan_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(oPrefsDlg->scan_modifier_key_vbox,b);
	conf->set_bool_at("dictionary/only_scan_while_modifier_key", b);
}

#ifdef _WIN32
void PrefsDlg::on_setup_dictionary_scan_clipboard_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	if (b) {
		if (conf->get_bool_at("dictionary/scan_selection"))
			gpAppFrame->oClipboard.start();
	} else {
		if (conf->get_bool_at("dictionary/scan_selection"))
			gpAppFrame->oClipboard.stop();
	}
	conf->set_bool_at("dictionary/scan_clipboard", b);
}

void PrefsDlg::on_setup_dictionary_use_scan_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	if (b)
		gpAppFrame->oHotkey.start_scan();
	else
		gpAppFrame->oHotkey.stop_scan();
	conf->set_bool_at("dictionary/use_scan_hotkey", b);
}
#endif

void PrefsDlg::on_setup_dictionary_scan_optionmenu_changed(GtkOptionMenu *option_menu, PrefsDlg *oPrefsDlg)
{	
  gint key = gtk_option_menu_get_history(option_menu);
  conf->set_int_at("dictionary/scan_modifier_key", key);
}

void PrefsDlg::on_setup_dictionary_scan_hide_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean hide = gtk_toggle_button_get_active(button);
  conf->set_bool_at("dictionary/hide_floatwin_when_modifier_key_released", hide);
}

void PrefsDlg::setup_dictionary_scan_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	GtkWidget *nb_label = gtk_label_new(_("Scan Selection"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_CONVERT,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Scan Selection</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	vbox1 = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Only scan while the modifier key is being pressed."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	bool only_scan_while_modifier_key=
	conf->get_bool_at("dictionary/only_scan_while_modifier_key");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
															 only_scan_while_modifier_key);
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_dictionary_scan_ckbutton_toggled), this);
	
	scan_modifier_key_vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox1), scan_modifier_key_vbox,
										 FALSE, FALSE, 12);
	gtk_widget_set_sensitive(scan_modifier_key_vbox,
													 only_scan_while_modifier_key);

	check_button = gtk_check_button_new_with_mnemonic(_("H_ide floating window when modifier key released."));
	gtk_box_pack_start(GTK_BOX(scan_modifier_key_vbox),check_button,false,false,0);	

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
															 conf->get_bool_at("dictionary/hide_floatwin_when_modifier_key_released"));
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_scan_hide_ckbutton_toggled), this);		

	hbox = gtk_hbox_new(false, 12);		
	gtk_box_pack_start(GTK_BOX(scan_modifier_key_vbox), hbox,false,false,0);
	label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Scan modifier _key:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);		
	GtkWidget *option_menu = gtk_option_menu_new();
	GtkWidget *menu = gtk_menu_new();


	for (std::list<std::string>::const_iterator p=key_combs.begin();
	     p!=key_combs.end(); ++p) {
	  GtkWidget *menuitem=gtk_menu_item_new_with_mnemonic(p->c_str());
	  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
	int scan_modifier_key=
		conf->get_int_at("dictionary/scan_modifier_key");

	gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), scan_modifier_key);
	
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), option_menu);
	gtk_box_pack_start(GTK_BOX(hbox), option_menu, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (option_menu), "changed", G_CALLBACK (on_setup_dictionary_scan_optionmenu_changed), this);	

#ifdef _WIN32
	check_button = gtk_check_button_new_with_mnemonic(_("_Scan clipboard."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("dictionary/scan_clipboard"));
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_dictionary_scan_clipboard_ckbutton_toggled), this);

	check_button = gtk_check_button_new_with_mnemonic(_("_Use scan hotkey: Ctrl+Alt+F1."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("dictionary/use_scan_hotkey"));
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_dictionary_use_scan_hotkey_ckbutton_toggled), this);
#endif
}

void PrefsDlg::change_font_for_all_widgets(const std::string& fontname)
{
	gchar *aa =
		g_strdup_printf("style \"custom-font\" { font_name= \"%s\" }\n"
										"class \"GtkWidget\" style \"custom-font\"\n", fontname.c_str());
	gtk_rc_parse_string(aa);
	g_free(aa);
	GdkScreen *screen = gtk_window_get_screen(parent_window);
	GtkSettings *settings=gtk_settings_get_for_screen(screen);
	gtk_rc_reset_styles(settings);
#ifndef CONFIG_GPE
	resize_categories_tree();
#endif
}

void PrefsDlg::on_setup_dictionary_font_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  gboolean b = gtk_toggle_button_get_active(button);
  gtk_widget_set_sensitive(oPrefsDlg->custom_font_hbox, b);
  conf->set_bool_at("dictionary/use_custom_font", b);
	if (b) {
		const std::string &custom_font=
			conf->get_string_at("dictionary/custom_font");
		oPrefsDlg->change_font_for_all_widgets(custom_font);
	} else
		oPrefsDlg->change_font_for_all_widgets("");
}

void PrefsDlg::on_setup_dictionary_font_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{		
  GtkWidget *dlg = gtk_font_selection_dialog_new(_("Choose dictionary font"));
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (oPrefsDlg->window));
  const gchar *text = gtk_button_get_label(GTK_BUTTON(widget));
  if (strcmp(text,_("Choose")))
    gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dlg), text);
  gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(dlg),_("Dictionary font"));
  gint result = gtk_dialog_run (GTK_DIALOG (dlg));
  if (result==GTK_RESPONSE_OK) {
    gchar *font_name = 
      gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dlg));
    if (font_name) {
      gtk_button_set_label(GTK_BUTTON(widget),font_name);
      conf->set_string_at("dictionary/custom_font", std::string(font_name));
    }
    if (font_name && font_name[0]) {
			oPrefsDlg->change_font_for_all_widgets(font_name);
    }
  }
  gtk_widget_destroy (dlg);
}

void PrefsDlg::setup_dictionary_font_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Font"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Font</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Use custom font."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	bool use_custom_font=
		conf->get_bool_at("dictionary/use_custom_font");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
															 use_custom_font);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_font_ckbutton_toggled), this);		
	custom_font_hbox = gtk_hbox_new(false, 12);	
	gtk_box_pack_start(GTK_BOX(vbox1),custom_font_hbox,false,false,0);
	gtk_widget_set_sensitive(custom_font_hbox, use_custom_font);
	label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Dictionary _font:"));
	gtk_box_pack_start(GTK_BOX(custom_font_hbox),label,false,false,0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);	
	GtkWidget *button;
	const std::string &custom_font=
		conf->get_string_at("dictionary/custom_font");

	if (!custom_font.empty())
		button = gtk_button_new_with_label(custom_font.c_str());
	else
		button=gtk_button_new_with_label(_("Choose"));	

	gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);
	gtk_box_pack_start(GTK_BOX(custom_font_hbox),button,false,false,0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_setup_dictionary_font_button_clicked), this);
}

void PrefsDlg::on_setup_dictionary_cache_CreateCacheFile_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/create_cache_file",enable);
}

void PrefsDlg::on_setup_dictionary_cache_EnableCollation_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(oPrefsDlg->collation_hbox, enable);
	conf->set_bool_at("dictionary/enable_collation",enable);
}

void PrefsDlg::on_setup_dictionary_collation_optionmenu_changed(GtkOptionMenu *option_menu, PrefsDlg *oPrefsDlg)
{
	gint key = gtk_option_menu_get_history(option_menu);
	conf->set_int_at("dictionary/collate_function", key);
}

static void clean_dir(const gchar *dirname)
{
	GDir *dir = g_dir_open(dirname, 0, NULL);
	if (dir) {
		const gchar *filename;
		gchar fullfilename[256];
		while ((filename = g_dir_read_name(dir))!=NULL) {
			sprintf(fullfilename, "%s" G_DIR_SEPARATOR_S "%s", dirname, filename);
			if (g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) {
				clean_dir(fullfilename);
			} else if (g_str_has_suffix(filename,".oft") || g_str_has_suffix(filename,".clt")) {
				g_unlink(fullfilename);
			}
		}
		g_dir_close(dir);
	}
}

void PrefsDlg::on_setup_dictionary_cache_cleanbutton_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	std::string dirname = gStarDictDataDir+ G_DIR_SEPARATOR_S "dic";
	clean_dir(dirname.c_str());
	dirname = g_get_user_cache_dir();
	dirname += G_DIR_SEPARATOR_S "stardict";
	clean_dir(dirname.c_str());
	g_rmdir(dirname.c_str());
}

void PrefsDlg::setup_dictionary_cache_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Cache"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_HARDDISK, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Cache</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	vbox1 = gtk_vbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("Create c_ache files to speed up loading."));
	bool enable = conf->get_bool_at("dictionary/create_cache_file");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_cache_CreateCacheFile_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	check_button = gtk_check_button_new_with_mnemonic(_("_Sort word list by collate function."));
	enable = conf->get_bool_at("dictionary/enable_collation");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_cache_EnableCollation_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	collation_hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),collation_hbox,false,false,0);
	label=gtk_label_new(NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("\tCollate _function:"));
	gtk_box_pack_start(GTK_BOX(collation_hbox),label,false,false,0);
	GtkWidget *option_menu = gtk_option_menu_new();
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *menuitem;
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__general__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__unicode__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__bin");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__czech__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__danish__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__esperanto__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__estonian__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__hungarian__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__icelandic__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__latvian__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__lithuanian__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__persian__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__polish__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__roman__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__romanian__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__slovak__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__slovenian__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__spanish__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__spanish2__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__swedish__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("utf8__turkish__ci");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
	int collate_function = conf->get_int_at("dictionary/collate_function");
	gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), collate_function);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), option_menu);
	gtk_box_pack_start(GTK_BOX(collation_hbox), option_menu, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (option_menu), "changed", G_CALLBACK (on_setup_dictionary_collation_optionmenu_changed), this);
	gtk_widget_set_sensitive(collation_hbox, enable);

	label = gtk_label_new(_("After enabled collation, when load the dictionaries for the first time, it will take some time for sorting, please wait for a moment."));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox1),label,false,false,0);

	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *button = gtk_button_new_with_mnemonic(_("C_lean all cache files"));
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_setup_dictionary_cache_cleanbutton_clicked), this);
}



void PrefsDlg::on_setup_dictionary_export_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/only_export_word", enable);
}

void PrefsDlg::on_setup_dictionary_export_browse_button_clicked(GtkButton *button, PrefsDlg *oPrefsDlg)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new (_("Open file..."),
			GTK_WINDOW(oPrefsDlg->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (dialog), gtk_entry_get_text(oPrefsDlg->eExportFile));
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gtk_entry_set_text(oPrefsDlg->eExportFile, filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

void PrefsDlg::setup_dictionary_export_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Export"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_SAVE,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Export</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	vbox1 = gtk_vbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);

	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("_Only export word."));
	bool enable= conf->get_bool_at("dictionary/only_export_word");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_export_ckbutton_toggled), this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);

	GtkWidget *hbox1 = gtk_hbox_new(FALSE, 6);
	label=gtk_label_new(_("File name:"));
	gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, FALSE, 0);
	GtkWidget *e = gtk_entry_new();
	const std::string &exportfile= conf->get_string_at("dictionary/export_file");
	gtk_entry_set_text(GTK_ENTRY(e), exportfile.c_str());
	gtk_box_pack_start(GTK_BOX(hbox1), e, TRUE, TRUE, 0);
	eExportFile=GTK_ENTRY(e);

	GtkWidget *button;
	button = gtk_button_new_with_mnemonic(_("_Browse..."));
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_dictionary_export_browse_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
}

void PrefsDlg::on_setup_dictionary_sound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  gboolean enable = gtk_toggle_button_get_active(button);
  conf->set_bool_at("dictionary/enable_sound_event",enable);
}

void PrefsDlg::setup_dictionary_sound_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Sound"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_YES,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Sound</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	vbox1 = gtk_vbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);

	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("_Enable sound event."));
	bool enable=
		conf->get_bool_at("dictionary/enable_sound_event");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_sound_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
#if defined(CONFIG_GTK) || defined(CONFIG_GPE)
	GtkWidget *hbox2 = gtk_hbox_new(FALSE, 6);
	label=gtk_label_new(_("Command for playing wav files:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	GtkWidget *e = gtk_entry_new();
	gtk_widget_set_size_request(e, 50, -1);
	const std::string &playcmd=
		conf->get_string_at("dictionary/play_command");
	gtk_entry_set_text(GTK_ENTRY(e), playcmd.c_str());
	gtk_box_pack_start(GTK_BOX(hbox2), e, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(hbox2, enable);  
	ePlayCommand=GTK_ENTRY(e);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox2, FALSE, FALSE, 0);
#endif

	label = gtk_label_new(_("RealPeopleTTS search path:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
	gtk_box_pack_start(GTK_BOX(vbox1),label,false,false,0);
	tts_textview = gtk_text_view_new();
	gtk_widget_set_size_request(tts_textview, -1, 70);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tts_textview), GTK_WRAP_CHAR);
	const std::string &ttspath = conf->get_string_at("dictionary/tts_path");
	GtkTextBuffer *text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tts_textview));
	gtk_text_buffer_set_text(text_view_buffer, ttspath.c_str(), -1);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_ETCHED_IN);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), tts_textview);
	gtk_box_pack_start(GTK_BOX(vbox1),scrolled_window,false,false,0);
}

void PrefsDlg::on_setup_mainwin_searchWhileTyping_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("main_window/search_while_typing", 
								 gtk_toggle_button_get_active(button));
}

void PrefsDlg::on_setup_mainwin_showfirstWhenNotfound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("main_window/showfirst_when_notfound", 
								 gtk_toggle_button_get_active(button));
}

void PrefsDlg::setup_mainwin_input_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Input"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_EDIT,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Input</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);

	vbox1 = gtk_vbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);	
	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("_Search while typing."));
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("main_window/search_while_typing"));
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_mainwin_searchWhileTyping_ckbutton_toggled), this);
	check_button = gtk_check_button_new_with_mnemonic(_("Show the _first word when not found."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("main_window/showfirst_when_notfound"));
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_mainwin_showfirstWhenNotfound_ckbutton_toggled), this);
}

void PrefsDlg::on_setup_mainwin_startup_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("main_window/hide_on_startup", 
								 gtk_toggle_button_get_active(button));
}

#ifdef _WIN32
void PrefsDlg::on_setup_mainwin_autorun_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	HKEY hKEY;
	LONG lRet;
	if (b) {
		lRet =RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_ALL_ACCESS,&hKEY);
		if(lRet==ERROR_SUCCESS) {
			std::string path = gStarDictDataDir+ G_DIR_SEPARATOR_S "stardict.exe";
			RegSetValueEx(hKEY, "StarDict", 0, REG_SZ, (const BYTE*)path.c_str(), path.length()+1);
			RegCloseKey(hKEY);
		}
	} else {
		lRet =RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_ALL_ACCESS,&hKEY);
		if(lRet==ERROR_SUCCESS) {
			RegDeleteValue(hKEY, "StarDict");
			RegCloseKey(hKEY);
		}
	}
}

void PrefsDlg::on_setup_mainwin_use_mainwindow_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	if (b)
		gpAppFrame->oHotkey.start_mainwindow();
	else
		gpAppFrame->oHotkey.stop_mainwindow();
	conf->set_bool_at("dictionary/use_mainwindow_hotkey", b);
}
#endif

void PrefsDlg::setup_mainwin_options_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Main window"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Options</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);

	vbox1 = gtk_vbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);	

	GtkWidget *check_button;
#ifdef _WIN32
	check_button = gtk_check_button_new_with_mnemonic(_("_Auto run StarDict after boot."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	gboolean autorun;

	HKEY hKEY;
	LONG lRet =RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&hKEY);
	if(lRet!=ERROR_SUCCESS) {
		autorun = false;
	} else {
		char owner_Get[80];
		DWORD cbData_1=80;
		DWORD type_1=REG_SZ;
		lRet=RegQueryValueEx(hKEY,"StarDict",NULL,&type_1,(LPBYTE)owner_Get,&cbData_1);
		RegCloseKey(hKEY);
		if((lRet!=ERROR_SUCCESS)||(cbData_1 > 80)) {
			autorun = false;
		} else {
			std::string path = gStarDictDataDir+ G_DIR_SEPARATOR_S "stardict.exe";
			if (strcmp(path.c_str(), owner_Get)==0)
				autorun = true;
			else
				autorun = false;
		}
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), autorun);
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_mainwin_autorun_ckbutton_toggled), this);
#endif

	check_button = gtk_check_button_new_with_mnemonic(_("Hide main window when _starting StarDict."));
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);
	bool hide=
		conf->get_bool_at("main_window/hide_on_startup");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), hide);
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_mainwin_startup_ckbutton_toggled), this);

#ifdef _WIN32
	check_button = gtk_check_button_new_with_mnemonic(_("_Use open main window hotkey: Ctrl+Alt+Z."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("dictionary/use_mainwindow_hotkey"));
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_mainwin_use_mainwindow_hotkey_ckbutton_toggled), this);
#endif

}

void PrefsDlg::write_mainwin_searchwebsite_list()
{
	GtkTreeIter iter;
	gboolean have_iter;
	gchar  *website_name, *website_link, *website_searchlink;
	std::list<std::string> searchwebsite_list;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW (searchwebsite_treeview));
	
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &website_name, 1, &website_link, 2, &website_searchlink, -1);
		std::string website(std::string(website_name)+'\t'+website_link+'\t'+website_searchlink);
		g_free(website_name);
		g_free(website_link);
		g_free(website_searchlink);
		searchwebsite_list.push_back(website);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	conf->set_strlist_at("main_window/search_website_list", searchwebsite_list);
}

void PrefsDlg::on_setup_mainwin_searchwebsite_moveup_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			GtkTreeIter prev;
			gtk_tree_model_get_iter(model, &prev, path);
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &prev);	
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview), path, NULL, false, 0, 0);
			oPrefsDlg->write_mainwin_searchwebsite_list();
		}		
		gtk_tree_path_free(path);		
	}	
}

void PrefsDlg::on_setup_mainwin_searchwebsite_movedown_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_next(path);
		GtkTreeIter next;
		if (gtk_tree_model_get_iter(model, &next, path)) {
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &next);	
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview), path, NULL, false, 0, 0);
			oPrefsDlg->write_mainwin_searchwebsite_list();
		}		
		gtk_tree_path_free(path);		
	}	
}

void PrefsDlg::on_setup_mainwin_searchwebsite_add_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
  GtkWidget *searchwebsite_add_dialog;
  GtkWidget *searchwebsite_add_dialog_name_entry;
  GtkWidget *searchwebsite_add_dialog_link_entry;
  GtkWidget *searchwebsite_add_dialog_searchlink_entry;

	searchwebsite_add_dialog = 
		gtk_dialog_new_with_buttons (_("Add"),
																 GTK_WINDOW (oPrefsDlg->window),
																 GTK_DIALOG_DESTROY_WITH_PARENT,
																 GTK_STOCK_CANCEL,
																 GTK_RESPONSE_CANCEL,
																 GTK_STOCK_OK,
																 GTK_RESPONSE_OK,
																 NULL);
	GtkWidget *table = gtk_table_new(3, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(searchwebsite_add_dialog)->vbox), table);
#ifndef CONFIG_GPE
	gtk_container_set_border_width(GTK_CONTAINER(table), 6);
#endif
	GtkWidget *label = gtk_label_new_with_mnemonic(_("Website Name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
	searchwebsite_add_dialog_name_entry = gtk_entry_new ();
#ifdef CONFIG_GPE
	gtk_widget_set_size_request(searchwebsite_add_dialog_name_entry, 100, -1);
#endif
	gtk_entry_set_activates_default(GTK_ENTRY(searchwebsite_add_dialog_name_entry), TRUE);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), searchwebsite_add_dialog_name_entry);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, (GtkAttachOptions)0, 6, 4);
	gtk_table_attach(GTK_TABLE(table), searchwebsite_add_dialog_name_entry, 1, 2, 0, 1, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
	
	
	label = gtk_label_new_with_mnemonic(_("Website link"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	searchwebsite_add_dialog_link_entry = gtk_entry_new ();
#ifdef CONFIG_GPE
	 gtk_widget_set_size_request(searchwebsite_add_dialog_link_entry, 100, -1);
#endif
	gtk_entry_set_activates_default (GTK_ENTRY (searchwebsite_add_dialog_link_entry), TRUE);    
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), searchwebsite_add_dialog_link_entry);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, (GtkAttachOptions)0, 6, 4);
	gtk_table_attach(GTK_TABLE(table), searchwebsite_add_dialog_link_entry, 1, 2, 1, 2, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
	
	label = gtk_label_new_with_mnemonic(_("Website search link"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	searchwebsite_add_dialog_searchlink_entry = gtk_entry_new ();
#ifdef CONFIG_GPE
	gtk_widget_set_size_request(searchwebsite_add_dialog_searchlink_entry, 100, -1);
#endif
	gtk_entry_set_activates_default (GTK_ENTRY (searchwebsite_add_dialog_searchlink_entry), TRUE);    
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), searchwebsite_add_dialog_searchlink_entry);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, (GtkAttachOptions)0, 6, 4);
	gtk_table_attach(GTK_TABLE(table), searchwebsite_add_dialog_searchlink_entry, 1, 2, 2, 3, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
	
	gtk_dialog_set_default_response(GTK_DIALOG(searchwebsite_add_dialog), GTK_RESPONSE_OK);		    		
	gtk_window_set_resizable(GTK_WINDOW(searchwebsite_add_dialog), FALSE);
	
	gtk_widget_show_all(GTK_WIDGET(searchwebsite_add_dialog));
	while (gtk_dialog_run(GTK_DIALOG(searchwebsite_add_dialog))==GTK_RESPONSE_OK) {
		gchar *error_msg = NULL;
		const gchar *website_name = gtk_entry_get_text(GTK_ENTRY(searchwebsite_add_dialog_name_entry));
		const gchar *website_link = gtk_entry_get_text(GTK_ENTRY(searchwebsite_add_dialog_link_entry));
		const gchar *website_searchlink = gtk_entry_get_text(GTK_ENTRY(searchwebsite_add_dialog_searchlink_entry));
		if (!website_name[0])
			error_msg = _("Please input the website name.");
		else if (!website_link[0])
			error_msg = _("Please input the website link.");
		else if (!website_searchlink[0])
			error_msg = _("Please input the website search link.");
		else if (!strstr(website_searchlink, "%s")) {
			error_msg = _("The website search link should contains a \"%%s\" string for querying a word.");
		}
		
		if (error_msg) {
			GtkWidget *message_dlg = 
				gtk_message_dialog_new(
															 GTK_WINDOW(searchwebsite_add_dialog),
																 (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
															 GTK_MESSAGE_INFO,	GTK_BUTTONS_OK,
															 error_msg);
			
			gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
			gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
			
			gtk_dialog_run(GTK_DIALOG(message_dlg));
			gtk_widget_destroy(message_dlg);
			continue;
		}			
		GtkListStore *model = 
			GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(oPrefsDlg->searchwebsite_treeview)));
		GtkTreeIter iter;
		gtk_list_store_prepend(model, &iter);
		gtk_list_store_set(model, &iter, 
											 0, website_name, 
											 1, website_link, 
											 2, website_searchlink, 
											 3, TRUE, 
											 -1);			
		oPrefsDlg->write_mainwin_searchwebsite_list();
		break;
	}
	gtk_widget_destroy(searchwebsite_add_dialog);
}

void PrefsDlg::on_setup_mainwin_searchwebsite_remove_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		if (gtk_list_store_remove(GTK_LIST_STORE(model), &iter)) {
			GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview), path, NULL, false, 0, 0);
			gtk_tree_path_free(path);
		}
		oPrefsDlg->write_mainwin_searchwebsite_list();
	}
}

void PrefsDlg::on_setup_mainwin_searchwebsite_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, PrefsDlg *oPrefsDlg)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;

	gint column;
	column = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (cell), "column"));
	gtk_tree_model_get_iter (model, &iter, path);

	switch (column) {
	case 0:
	case 1:
		if (new_text[0]) {
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);
			oPrefsDlg->write_mainwin_searchwebsite_list();
		}
		break;
	case 2:
		if (new_text[0]) {
			if (!strstr(new_text, "%s")) {
				GtkWidget *message_dlg;

				message_dlg = gtk_message_dialog_new (
					GTK_WINDOW (oPrefsDlg->window),
					(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					_("The website search link should contains a \"%%s\" string for quering word."));

				gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

				gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

				gtk_dialog_run (GTK_DIALOG (message_dlg));
				gtk_widget_destroy (message_dlg);
			}
			else {
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, 2, new_text, -1);
				oPrefsDlg->write_mainwin_searchwebsite_list();
			}
		}
		break;

    }

	gtk_tree_path_free (path);
}

void PrefsDlg::setup_mainwin_searchwebsite_page()
{
	GtkWidget *vbox;
#ifdef CONFIG_GPE
	vbox = gtk_vbox_new(false,0);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Search website"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	vbox = gtk_vbox_new(false,12);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Search Website</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	GtkWidget *vbox2;
	vbox2 = gtk_vbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox), vbox2, true, true,0);
	
	GtkListStore *model;
	model = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

	const std::list<std::string> &web_list=
		conf->get_strlist_at("main_window/search_website_list");

	GtkTreeIter iter;
	for (std::list<std::string>::const_iterator wit=web_list.begin();
			 wit!=web_list.end(); ++wit) {
		std::vector<std::string> l=split(*wit, '\t');
		if (l.size()==3) {
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter, 
												 0, l[0].c_str(), 
												 1, l[1].c_str(), 
												 2, l[2].c_str(), 
												 3, TRUE, 
												 -1);
		}
	}
	
	GtkWidget *sw;	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
      	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	
	gtk_widget_set_size_request (sw, 300, 180);

	searchwebsite_treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	g_object_unref (G_OBJECT (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (searchwebsite_treeview), TRUE);
	
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (searchwebsite_treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
		
	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (renderer, "edited", G_CALLBACK (on_setup_mainwin_searchwebsite_cell_edited), this);
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  	
	g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER(0));
	column = gtk_tree_view_column_new_with_attributes (_("Website Name"), renderer, "text", 0, "editable", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(searchwebsite_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (renderer, "edited", G_CALLBACK (on_setup_mainwin_searchwebsite_cell_edited), this);
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  	
	g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER(1));
	column = gtk_tree_view_column_new_with_attributes (_("Website link"), renderer, "text", 1, "editable", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(searchwebsite_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (renderer, "edited", G_CALLBACK (on_setup_mainwin_searchwebsite_cell_edited), this);
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  	
	g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER(2));
	column = gtk_tree_view_column_new_with_attributes (_("Website search link"), renderer, "text", 2, "editable", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(searchwebsite_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	
	gtk_container_add (GTK_CONTAINER (sw), searchwebsite_treeview);
	gtk_box_pack_start (GTK_BOX (vbox2), sw, TRUE, TRUE, 0);

	GtkWidget *hbox1;
	hbox1 = gtk_hbox_new(false,6);
	GtkWidget *button;
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), image);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_moveup_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), image);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_movedown_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_remove_button_clicked), this);
	gtk_box_pack_end (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	
/*	button = gtk_button_new();	
	GtkWidget *align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);      
	gtk_container_add (GTK_CONTAINER (button), align);
	GtkWidget *hbox2 = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (align), hbox2);
	label = gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("_Modify"));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);	
	image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (hbox2), image, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox2), label, FALSE, FALSE, 0);      
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_edit_button_clicked), this);
	gtk_box_pack_end (GTK_BOX (hbox1), button, FALSE, FALSE, 0);*/
	
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_add_button_clicked), this);
	gtk_box_pack_end (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, false, false, 0);
}

void PrefsDlg::on_setup_NotificationAreaIcon_QueryInFloatWin_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean queryin = gtk_toggle_button_get_active(button);
  conf->set_bool_at("notification_area_icon/query_in_floatwin",
								 queryin);
}

void PrefsDlg::setup_NotificationAreaIcon_options_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Notification area icon"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_DND,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Options</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	GtkWidget *hbox1;
	hbox1 = gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox1,false,false,0);
	
	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("_Query in the floating window when middle mouse\nbutton is clicked."));
	bool query_in_floatwin=
	conf->get_bool_at("notification_area_icon/query_in_floatwin");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
															 query_in_floatwin);
	g_signal_connect(G_OBJECT(check_button), "toggled", 
									 G_CALLBACK(on_setup_NotificationAreaIcon_QueryInFloatWin_ckbutton_toggled), this);
	gtk_box_pack_start(GTK_BOX(hbox1),check_button,false,false,0);
}

void PrefsDlg::on_setup_floatwin_pronounce_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("floating_window/pronounce_when_popup",
								 gtk_toggle_button_get_active(button));
}

void PrefsDlg::on_setup_show_float_if_not_found(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("floating_window/show_if_not_found",
								 gtk_toggle_button_get_active(button));
}

void PrefsDlg::setup_floatwin_options_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
#ifdef CONFIG_GPE
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(_("Floating window"));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox, nb_label);
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
#endif
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_DND,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Options</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	vbox1 = gtk_vbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Pronouce the word when it pops up."));
	bool pronounce_when_popup=
		conf->get_bool_at("floating_window/pronounce_when_popup");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
															 pronounce_when_popup);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(on_setup_floatwin_pronounce_ckbutton_toggled), this);
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);

	check_button = gtk_check_button_new_with_mnemonic(_("_Show floating window if word not found."));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("floating_window/show_if_not_found"));
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(on_setup_show_float_if_not_found), this);	
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);
}

#ifndef CONFIG_GPE
void PrefsDlg::on_setup_floatwin_size_max_width_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg)
{
	gint width = gtk_spin_button_get_value_as_int(button);
  conf->set_int_at("floating_window/max_window_width", width);
}

void PrefsDlg::on_setup_floatwin_size_max_height_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg)
{
	gint height = gtk_spin_button_get_value_as_int(button);
  conf->set_int_at("floating_window/max_window_height", height);
}

void PrefsDlg::setup_floatwin_size_page()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,12);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,NULL);
	GtkWidget *vbox1;
	vbox1 = gtk_vbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false,6);
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,6);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_ZOOM_FIT,GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox),image,false,false,0);
	GtkWidget *label;
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<span weight=\"bold\" size=\"x-large\">Max window size</span>"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *hseparator;
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,false,false,0);
	
	GtkWidget *table;
	table = gtk_table_new(3, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_box_pack_start(GTK_BOX(vbox),table,false,false,0);
	
	int max_width=
		conf->get_int_at("floating_window/max_window_width");
	int max_height=
		conf->get_int_at("floating_window/max_window_height");

	GdkScreen *screen = gtk_window_get_screen(parent_window);
	gint screen_width = gdk_screen_get_width(screen);
	gint screen_height = gdk_screen_get_height(screen);

	label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Max window _width:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	GtkWidget *spin_button;
	spin_button = gtk_spin_button_new_with_range(MIN_MAX_FLOATWIN_WIDTH,screen_width,1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spin_button);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), max_width);
	g_signal_connect(G_OBJECT(spin_button), "value-changed", 
									 G_CALLBACK(on_setup_floatwin_size_max_width_spinbutton_changed), this);
	gtk_table_attach(GTK_TABLE(table), spin_button, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	label=gtk_label_new(_("(default:320)"));
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Max window hei_ght:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	spin_button = gtk_spin_button_new_with_range(MIN_MAX_FLOATWIN_HEIGHT,screen_height,1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spin_button);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), max_height);
	g_signal_connect (G_OBJECT (spin_button), "value-changed", G_CALLBACK (on_setup_floatwin_size_max_height_spinbutton_changed), (gpointer)this);
	gtk_table_attach(GTK_TABLE(table), spin_button, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	label=gtk_label_new(_("(default:240)"));
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
}
#endif

GtkWidget* PrefsDlg::create_notebook ()
{
	notebook = gtk_notebook_new();
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
#ifdef CONFIG_GPE
	gtk_notebook_set_scrollable(nb, true);
#else
	gtk_notebook_set_show_tabs(nb,false);
	gtk_notebook_set_show_border(nb,false);
	setup_logo_page ();
#endif	
	setup_dictionary_scan_page ();
	setup_dictionary_font_page ();
	setup_dictionary_cache_page ();
	setup_dictionary_export_page ();
	setup_dictionary_sound_page ();
	setup_mainwin_input_page ();
	setup_mainwin_options_page ();
	setup_mainwin_searchwebsite_page();
	setup_NotificationAreaIcon_options_page();
	setup_floatwin_options_page ();
#ifdef CONFIG_GPE
	gtk_notebook_set_current_page (nb, 0);
#else
	setup_floatwin_size_page ();
	gtk_notebook_set_current_page (nb, LOGO);	
#endif	
	return notebook;
}


PrefsDlg::PrefsDlg(GtkWindow *parent, GdkPixbuf *logo, const std::list<std::string>& key_combs_) :
	key_combs(key_combs_)
{
  parent_window=parent;
#ifndef CONFIG_GPE
  stardict_logo=logo;
#endif

  window = NULL;  
}

bool PrefsDlg::ShowModal()
{		
  window = gtk_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(window), parent_window);

  gtk_dialog_add_button(GTK_DIALOG(window),
			GTK_STOCK_HELP,
			GTK_RESPONSE_HELP);
	
  gtk_dialog_add_button(GTK_DIALOG(window),
			GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response(GTK_DIALOG(window), 
				  GTK_RESPONSE_CLOSE);
  g_signal_connect(G_OBJECT(window), "response",
		   G_CALLBACK(response_handler), this);
#ifndef CONFIG_GPE
  GtkWidget *hbox;
  hbox = gtk_hbox_new (FALSE, 18);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  GtkWidget *r;
  r = gtk_vbox_new (FALSE, 6);
	
  GtkWidget *label;
  label = gtk_label_new_with_mnemonic (_("Cat_egories:"));
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
  create_categories_tree();

  
  gtk_box_pack_start(GTK_BOX(r), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(r), categories_window, TRUE, TRUE, 0);
#endif
 
  GtkWidget *l = create_notebook ();

#ifdef CONFIG_GPE
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), l, true, true, 0);
#else
	gtk_box_pack_start (GTK_BOX (hbox), r, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox, true, true, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), categories_tree);
#endif
  
  gtk_widget_show_all (GTK_DIALOG (window)->vbox);
  gtk_window_set_title (GTK_WINDOW (window), _("Preferences"));

#ifndef CONFIG_GPE
  resize_categories_tree();
#endif
  
	gint result;
	while ((result = gtk_dialog_run(GTK_DIALOG(window)))==GTK_RESPONSE_HELP)
		;
	if (result != GTK_RESPONSE_NONE) {
		const gchar *ch;
		ch = gtk_entry_get_text(eExportFile);
		if (ch[0])
			conf->set_string_at("dictionary/export_file", ch);
#if defined(CONFIG_GTK) || defined(CONFIG_GPE)
		ch = gtk_entry_get_text(ePlayCommand);
		if (ch[0])
			conf->set_string_at("dictionary/play_command", ch);
#endif
		GtkTextBuffer *text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tts_textview));
		GtkTextIter start_iter;
		GtkTextIter end_iter;
		gtk_text_buffer_get_start_iter(text_view_buffer, &start_iter);
		gtk_text_buffer_get_end_iter(text_view_buffer, &end_iter);
		gchar *text = gtk_text_buffer_get_text(text_view_buffer, &start_iter, &end_iter, FALSE);
		conf->set_string_at("dictionary/tts_path", text);
		gpAppFrame->oReadWord.loadpath(text);
		g_free(text);
		gtk_widget_destroy(GTK_WIDGET(window));
		window = NULL;
		return false;
	} else {
		return true;
	}
}

void PrefsDlg::Close()
{
	if (window) {
		gtk_widget_destroy (window);
		window = NULL;
	}
}

#ifndef CONFIG_GPE
void PrefsDlg::resize_categories_tree(void)
{
  //this is hack for prevet horizontaly scrolling
  //if you know how it make better, just write
  GtkRequisition rtv, rsw;
  gtk_widget_size_request(categories_tree, &rtv);
  gtk_widget_size_request(GTK_SCROLLED_WINDOW(categories_window)->vscrollbar, &rsw);
  gtk_widget_set_size_request(categories_window, rtv.width+rsw.width+25, -1);
}
#endif
