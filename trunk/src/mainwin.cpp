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

#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#ifdef _WIN32
#define VERSION "2.4.8"
#  include <gdk/gdkwin32.h>
#endif

#include "conf.h"
#include "stardict.h"
#include "utils.h"

#include "mainwin.h"


/**************************************************/
TopWin::TopWin()
{
	WordCombo = NULL; //need by save_yourself_cb().
	HisList = NULL;
	BackList = NULL;
	enable_change_cb = true;
	MainMenu = NULL;
	HistoryMenu = NULL;
	BackMenu = NULL;
}

TopWin::~TopWin()
{
	g_list_foreach (HisList, (GFunc)g_free, NULL);
	g_list_free(HisList);

	GSList *list = BackList;
	while (list) {
		g_free(((BackListData *)(list->data))->word);
		g_free(list->data);
		list = list->next;
	}
	g_slist_free(BackList);
}

void TopWin::Create(GtkWidget *vbox)
{
	GtkWidget *hbox = gtk_hbox_new(false,0);
	gtk_widget_show(hbox);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,3);
#endif

	GtkWidget *button;
#ifndef CONFIG_GPE	
	button = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_widget_show(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(ClearCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,3);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Clear the search box"),NULL);
#endif
	
	WordCombo = gtk_combo_new();
	LoadHistory();
	gtk_widget_set_size_request(WordCombo,60,-1);
	gtk_widget_show(WordCombo);
	gtk_combo_set_case_sensitive(GTK_COMBO(WordCombo),true);
	gtk_combo_disable_activate(GTK_COMBO(WordCombo));
	gtk_combo_set_use_arrows(GTK_COMBO(WordCombo), false);
	gtk_entry_set_max_length(GTK_ENTRY(GTK_COMBO(WordCombo)->entry), 255);
	g_signal_connect (G_OBJECT (GTK_COMBO(WordCombo)->entry), "changed",
			  G_CALLBACK (on_entry_changed), this);
	g_signal_connect (G_OBJECT (GTK_COMBO(WordCombo)->entry), "activate",
			  G_CALLBACK (on_entry_activate), this);
	gtk_box_pack_start(GTK_BOX(hbox),WordCombo,true,true,3);

#ifndef CONFIG_GPE
	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_FIND,GTK_ICON_SIZE_BUTTON));	
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(GoCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Fuzzy Query"),NULL);
#endif

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_UNDO,GTK_ICON_SIZE_BUTTON));	
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(BackCallback),this);
	g_signal_connect(G_OBJECT(button),"button_press_event", G_CALLBACK(on_back_button_press),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Go Back - Right button: history (Alt+Left)"),NULL);

	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_GO_BACK,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(PreviousCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Previous word (Alt+Up)"),NULL);

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(NextCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Next word (Alt+Down)"),NULL);

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_HOME,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(MenuCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);	
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Show the main menu (Alt+M)"),NULL);
}

void TopWin::Destroy(void)
{
	InsertHisList(GetText());
	SaveHistory();
	if (MainMenu)
		gtk_widget_destroy(MainMenu);
	if (HistoryMenu)
		gtk_widget_destroy(HistoryMenu);
	if (BackMenu)
		gtk_widget_destroy(BackMenu);
}

void TopWin::on_entry_changed(GtkEntry *entry, TopWin *oTopWin)
{
	if (!(oTopWin->enable_change_cb))
		return;
	if (!conf->get_bool("/apps/stardict/preferences/main_window/search_while_typing"))
		return;
	const gchar *sWord = gtk_entry_get_text(entry);
	if(sWord[0]!='\0') {
		gpAppFrame->TopWinWordChange(sWord);
	} else {
		gpAppFrame->oMidWin.oTextWin.queryWord.clear();
		gpAppFrame->oMidWin.oIndexWin.oResultWin.Clear();
		gpAppFrame->oMidWin.oTextWin.ShowTips();
	}
}

void TopWin::on_entry_activate(GtkEntry *entry, TopWin *oTopWin)
{
	gpAppFrame->TopWinEnterWord(gtk_entry_get_text(entry));
}

gboolean TopWin::on_back_button_press(GtkWidget * widget, GdkEventButton * event , TopWin *oTopWin)
{
	if (event->button == 3) {
		if (!(oTopWin->BackList))
			return true;
		gint index=0;
		GSList *list;
		list = oTopWin->BackList;
		if (!strcmp(((BackListData *)(list->data))->word, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(oTopWin->WordCombo)->entry)))) {
			list = g_slist_next(list);
			index++;
			if (!list)
				return true;
		}
		if (oTopWin->BackMenu)
			gtk_widget_destroy(oTopWin->BackMenu);
			
		oTopWin->BackMenu = gtk_menu_new();
		
		GtkWidget *menuitem;
		while (list) {			
			menuitem = gtk_image_menu_item_new_with_label(((BackListData *)(list->data))->word);
			g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_back_menu_item_activate), GINT_TO_POINTER(index));
			gtk_menu_shell_append(GTK_MENU_SHELL(oTopWin->BackMenu), menuitem);
			list = g_slist_next(list);
			index++;
		}
		gtk_widget_show_all(oTopWin->BackMenu);
		gtk_menu_popup(GTK_MENU(oTopWin->BackMenu), NULL, NULL, NULL, NULL, event->button, event->time);		
		return true;
	}
	return false;
}

void TopWin::on_back_menu_item_activate(GtkMenuItem *menuitem, gint index)
{
  for (int i=0; i<index; i++) {
    g_free(((BackListData *)((gpAppFrame->oTopWin.BackList)->data))->word);
    g_free((gpAppFrame->oTopWin.BackList)->data);
    gpAppFrame->oTopWin.BackList = g_slist_delete_link(gpAppFrame->oTopWin.BackList, gpAppFrame->oTopWin.BackList);	
  }
  gpAppFrame->oTopWin.InsertHisList(gpAppFrame->oTopWin.GetText());
  gpAppFrame->oTopWin.SetText(((BackListData *)((gpAppFrame->oTopWin.BackList)->data))->word);
  if (((BackListData *)(gpAppFrame->oTopWin.BackList->data))->adjustment_value != -1) {
    ProcessGtkEvent();
    gpAppFrame->oMidWin.oTextWin.view->ScrollTo(((BackListData *)(gpAppFrame->oTopWin.BackList->data))->adjustment_value);
  }
  g_free(((BackListData *)((gpAppFrame->oTopWin.BackList)->data))->word);
  g_free((gpAppFrame->oTopWin.BackList)->data);
  gpAppFrame->oTopWin.BackList = g_slist_delete_link(gpAppFrame->oTopWin.BackList, gpAppFrame->oTopWin.BackList);	
}

#ifndef CONFIG_GPE
void TopWin::ClearCallback(GtkWidget *widget, TopWin *oTopWin)
{
	play_sound_on_event("buttonactive");
	
	oTopWin->InsertHisList(oTopWin->GetText());
	oTopWin->InsertBackList();
	oTopWin->SetText("");
	oTopWin->GrabFocus();
}

void TopWin::GoCallback(GtkWidget *widget, TopWin *oTopWin)
{
	play_sound_on_event("buttonactive");
	
	const gchar *text = oTopWin->GetText();
	if (text[0]=='\0')
		return;
	std::string res;
	switch (analyse_query(text, res)) {
	case qtFUZZY:
		gpAppFrame->LookupWithFuzzyToMainWin(res.c_str());
		break;
	case qtREGEXP:
		gpAppFrame->LookupWithRuleToMainWin(res.c_str());
		break;
	case qtDATA:
                gpAppFrame->LookupDataToMainWin(res.c_str());
                break;
	default:
		gpAppFrame->LookupWithFuzzyToMainWin(res.c_str());
	}

	oTopWin->TextSelectAll();
	oTopWin->GrabFocus();
	oTopWin->InsertHisList(text);
	oTopWin->InsertBackList(text);
}
#endif

void TopWin::do_back()
{
  if (!BackList)
    return;
  GSList *list = BackList;
  if (!strcmp(((BackListData *)(list->data))->word, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry)))) {
    list = g_slist_next(list);
    if (!list)
      return;
  }
  InsertHisList(GetText());
  SetText(((BackListData *)(list->data))->word);
  if (GTK_WIDGET_HAS_FOCUS(GTK_WIDGET(GTK_COMBO(WordCombo)->entry)))
    gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry),0,-1);
  if (((BackListData *)(list->data))->adjustment_value != -1) {
    ProcessGtkEvent(); // so all the definition text have been inserted.
    gpAppFrame->oMidWin.oTextWin.view->ScrollTo(((BackListData *)(list->data))->adjustment_value);
  }
  g_free(((BackListData *)(list->data))->word);
  g_free(list->data);
  BackList = g_slist_delete_link(BackList,list);	
}

void TopWin::BackCallback(GtkWidget *widget, TopWin *oTopWin)
{
	play_sound_on_event("buttonactive");
	
	oTopWin->do_back();
}

void TopWin::do_previous()
{
	if (gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_EMPTY)
		return;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;

	gboolean selected = gtk_tree_selection_get_selected(selection,&model,&iter);
	if (gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_NORMAL_LIST) {
		if (!(selected && GTK_WIDGET_HAS_FOCUS(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview))) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return; //this should never happen.
		}
		GtkTreePath* path = gtk_tree_model_get_path(model,&iter);	
		if (gtk_tree_path_prev(path)) {
			gtk_tree_model_get_iter(model,&iter,path);
			gtk_tree_selection_select_iter(selection,&iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview),path,NULL,false,0,0);
		}
		else {
			// user have selected the first row.
			gchar *word;
			gtk_tree_model_get (model, &iter, 0, &word, -1);
			CurrentIndex *iPreIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * gpAppFrame->oLibs.ndicts());
			const gchar *preword = gpAppFrame->oLibs.poGetPreWord(word, iPreIndex);
			if (preword) {
				SetText(preword);
				if (GTK_WIDGET_HAS_FOCUS(GTK_WIDGET(GTK_COMBO(WordCombo)->entry)))
					gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry),0,-1);
			}
			g_free(iPreIndex);
			g_free(word);
		}		
		gtk_tree_path_free(path);
	}
	else if (gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_FUZZY_LIST ||
			gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_PATTERN_LIST||
			gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_DATA_LIST) {
		if (!selected) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return; //this should never happen.
		}
		GtkTreePath* path = gtk_tree_model_get_path(model,&iter);	
		if (gtk_tree_path_prev(path)) {
			gtk_tree_model_get_iter(model,&iter,path);
			gtk_tree_selection_select_iter(selection,&iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview),path,NULL,false,0,0);
		}
		else {
			// user have selected the first row,no action is need.
		}
		gtk_tree_path_free(path);
	}	
}

void TopWin::PreviousCallback(GtkWidget *widget, TopWin *oTopWin)
{	
	play_sound_on_event("buttonactive");

	oTopWin->do_previous();
}

void TopWin::do_next()
{
	if (gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_EMPTY)
		return;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;

	gboolean selected = gtk_tree_selection_get_selected(selection,&model,&iter); //make sure this will run,so model is set.
	if (gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_NORMAL_LIST) {
		gboolean real_selected = (selected && GTK_WIDGET_HAS_FOCUS(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview));
		if (!real_selected) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return; //this should never happen.
		}
		GtkTreeIter new_iter = iter; //if gtk_tree_model_iter_next fail,iter will be invalid,so save it.
		gchar *word;
		if (gtk_tree_model_iter_next(model,&iter)) {
			if (!real_selected) {
				gtk_tree_model_get (model, &iter, 0, &word, -1);
				SetText(word);
				if (GTK_WIDGET_HAS_FOCUS(GTK_WIDGET(GTK_COMBO(WordCombo)->entry)))
					gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry),0,-1);
				g_free(word);
			}
			else {
				gtk_tree_selection_select_iter(selection,&iter);
				GtkTreePath* path = gtk_tree_model_get_path(model,&iter);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview),path,NULL,false,0,0);
				gtk_tree_path_free(path);				
			}
		}
		else {
			// user have selected the last row.
			gtk_tree_model_get (model, &new_iter, 0, &word, -1);
			CurrentIndex *iNextIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * gpAppFrame->oLibs.ndicts());
			const gchar *nextword = gpAppFrame->oLibs.poGetNextWord(word, iNextIndex);
			if (nextword) {
				SetText(nextword);
				if (GTK_WIDGET_HAS_FOCUS(GTK_WIDGET(GTK_COMBO(WordCombo)->entry)))
					gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry),0,-1);
			}
			g_free(iNextIndex);
			g_free(word);
		}		
	}
	else if (gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_FUZZY_LIST ||
			gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_PATTERN_LIST ||
			gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type == LIST_WIN_DATA_LIST) {
		if (!selected) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return; //this should never happen.
		}
		if (gtk_tree_model_iter_next(model,&iter)) {
			gtk_tree_selection_select_iter(selection,&iter);
			GtkTreePath* path = gtk_tree_model_get_path(model,&iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gpAppFrame->oMidWin.oIndexWin.oListWin.treeview),path,NULL,false,0,0);
			gtk_tree_path_free(path);
		}
		else {
			// user have selected the last row,no action is need.
		}
	}
}

void TopWin::NextCallback(GtkWidget *widget, TopWin *oTopWin)
{
	play_sound_on_event("buttonactive");
	oTopWin->do_next();
}

void TopWin::on_main_menu_preferences_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{	
	gpAppFrame->PopupPrefsDlg();
}

void TopWin::on_main_menu_dictmanage_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	gpAppFrame->PopupDictManageDlg();
}

void TopWin::on_main_menu_newversion_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
  show_url("http://stardict.sourceforge.net");
}

void TopWin::on_main_menu_help_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
  show_help(NULL);
}

void TopWin::on_main_menu_about_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	const gchar *authors[] = {
    "Hu Zheng <huzheng_001@163.com>",
    "Evgeniy <dushistov@mail.ru>",
    "Opera Wang <wangvisual@sohu.com>",
    "Ma Su'an <msa@wri.com.cn>",
        NULL
    };
    gchar *documenters[] = {
		"Hu Zheng <huzheng_001@163.com>",
		"Will Robinson <wsr23@stanford.edu>",
		"Anthony Fok <foka@debian.org>",
	    NULL
    };
    gchar *translator_credits = _("translator_credits");

	gtk_show_about_dialog(GTK_WINDOW (gpAppFrame->window),
		"name", _("StarDict"),
		"version", VERSION,
		"website", "http://stardict.sourceforge.net",
		"comments", _("StarDict is an international dictionary for GNOME."),
		"copyright", "Copyright \xc2\xa9 1999 by Ma Su'an\n" "Copyright \xc2\xa9 2002 by Opera Wang\n" "Copyright \xc2\xa9 2003-2006 by Hu Zheng",
		"authors", (const char **)authors,
		"documenters", (const char **)documenters,
		"translator-credits", strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
		"logo", gpAppFrame->oAppSkin.icon.get(),
		NULL);
}

void TopWin::on_main_menu_quit_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	gpAppFrame->Quit();
}

void TopWin::ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
	if (text)
		gpAppFrame->Query(text);
}

void TopWin::do_menu()
{
	play_sound_on_event("menushow");
	
	if (!MainMenu) {
		MainMenu = gtk_menu_new();

		GtkWidget *menuitem;
		menuitem = gtk_image_menu_item_new_with_mnemonic(_("Pr_eferences"));
		GtkWidget *image;
		image = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_preferences_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("Manage _Dict"));
		image = gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_dictmanage_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);
		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_New Version"));
		image = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_newversion_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);
		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Help"));
		image = gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_help_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_About"));
		image = gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_BUTTON);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_about_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);
		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
		image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_quit_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		gtk_widget_show_all(MainMenu);

	}
	gtk_menu_popup(GTK_MENU(MainMenu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
}

void TopWin::MenuCallback(GtkWidget *widget, TopWin *oTopWin)
{
	oTopWin->do_menu();
}

void TopWin::SetText(const gchar *word, bool notify)
{
	if (strcmp(word, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry))) == 0)
		return;
	enable_change_cb = false;
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry),word); // this will emit "changed" signal twice, one for "", one for word. so disable it.
	enable_change_cb = true;

	if (!notify)
		return;
	if(word[0]!='\0') {
		gpAppFrame->TopWinWordChange(word);
	} else {
		gpAppFrame->oMidWin.oTextWin.queryWord.clear();
		gpAppFrame->oMidWin.oIndexWin.oResultWin.Clear();
		gpAppFrame->oMidWin.oTextWin.ShowTips();
	}
}

/*void TopWin::SetText_without_notify(const gchar *word)
{
	enable_change_cb = false;
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry),word);
	enable_change_cb = true;
}*/

const gchar* TopWin::GetText()
{
	return gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry));
}

void TopWin::GrabFocus()
{
	gtk_widget_grab_focus(GTK_COMBO(WordCombo)->entry);
}

void TopWin::TextSelectAll()
{
	gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry),0,-1);
}

gboolean TopWin::TextSelected()
{
    return (gtk_editable_get_selection_bounds(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry),NULL,NULL));
}

gint TopWin::HisCompareFunc(gconstpointer a,gconstpointer b)
{
	return strcmp((const gchar *)a, (const gchar *)b);
}

gint TopWin::BackListDataCompareFunc(gconstpointer a,gconstpointer b)
{
	return strcmp(((const BackListData *)a)->word, ((const BackListData *)b)->word);
}

void TopWin::InsertHisList(const gchar *word)
{
	if (!word || word[0]=='\0')
		return;

	gchar *str = g_strdup(word);
	GList *list = g_list_find_custom(HisList, str, HisCompareFunc);
	if (list) {
		g_free(list->data);
		HisList = g_list_delete_link(HisList,list);
		HisList = g_list_prepend(HisList,str);
	}	else
		HisList = g_list_prepend(HisList, str);
	
	if (g_list_length(HisList)> MAX_HISTORY_WORD_ITEM_NUM) {
		list = g_list_last(HisList);
		g_free(list->data);
		HisList = g_list_delete_link(HisList,list);
	}

	const gchar *text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry));
	// if the list's first item don't equal to entry's text,
	//gtk_combo_set_popdown_strings will change the entry's text to it.
	// so here make it always equal.
	if (strcmp(text, word)) {
		list = g_list_append(NULL, (gpointer)text);
		list = g_list_concat(list, HisList);
		gtk_combo_set_popdown_strings(GTK_COMBO(WordCombo), list);
		HisList = g_list_delete_link(list,list);
	}	else
		gtk_combo_set_popdown_strings(GTK_COMBO(WordCombo),HisList);
}

void TopWin::SaveHistory()
{
	FILE *f=g_fopen(conf->get_string("/apps/stardict/preferences/dictionary/history").c_str(), "w");
	if (!f)
		return;
	GList *hist_list = HisList;
	while (hist_list) {
		fprintf(f, "%s\n", (char *)hist_list->data);
		hist_list=g_list_next(hist_list);
	}
	fclose(f);
}

void TopWin::LoadHistory()
{
	const gchar *filename = conf->get_string("/apps/stardict/preferences/dictionary/history").c_str();
	struct stat stats;
	if (g_stat (filename, &stats) == -1)
        	return;
	FILE *historyfile;
	historyfile = g_fopen(filename,"r");
	if (!historyfile)
		return;
	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t readsize = fread (buffer, 1, stats.st_size, historyfile);
	fclose (historyfile);
	buffer[readsize] = '\0';
	gchar *p,*p1;
	p=buffer;
	HisList = g_list_append(HisList, (gpointer)"");
	while (*p) {
		p1=strchr(p, '\n');
		if (p1) {
			*p1='\0';
			HisList = g_list_append(HisList, g_strdup(p));
			p = p1+1;
		} else {
			break;
		}
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(WordCombo), HisList);
	HisList = g_list_delete_link(HisList, HisList);
	g_free(buffer);
}

void TopWin::InsertBackList(const gchar *word)
{
  BackListData *backlistdata;
  if (word) {
    if (word[0] == '\0')
      return;
    backlistdata = (BackListData *)g_malloc(sizeof(BackListData));
    backlistdata->word = g_strdup(word);
    backlistdata->adjustment_value = -1;
  } else {
    word = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry));
    if (word[0] == '\0')
      return;
    backlistdata = (BackListData *)g_malloc(sizeof(BackListData));
    backlistdata->word = g_strdup(word);
    backlistdata->adjustment_value = gpAppFrame->oMidWin.oTextWin.view->ScrollPos();
  }	

  GSList* list = 
    g_slist_find_custom(BackList, backlistdata, BackListDataCompareFunc);
  if (list) {
    g_free(((BackListData *)(list->data))->word);
    g_free(list->data);
    BackList = g_slist_delete_link(BackList,list);
    BackList = g_slist_prepend(BackList,backlistdata);
  } else {
    BackList = g_slist_prepend(BackList,backlistdata);
  }
  if (g_slist_length(BackList)> MAX_BACK_WORD_ITEM_NUM)	{
    list = g_slist_last(BackList);
    g_free(((BackListData *)(list->data))->word);
    g_free(list->data);
    BackList = g_slist_delete_link(BackList,list);
  }
}

/**************************************************/
ListWin::ListWin()
{
}

void ListWin::Create(GtkWidget *notebook)
{
	list_word_type = LIST_WIN_EMPTY;
	
	list_model = gtk_list_store_new (1,G_TYPE_STRING);
	tree_model = gtk_tree_store_new (1,G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(list_model));
	nowIsList = true;
	gtk_widget_show(treeview);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("word", renderer,
						     "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (on_selection_changed), this);
	
	g_signal_connect (G_OBJECT (treeview), "button_press_event", G_CALLBACK (on_button_press), this);

	GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		
	gtk_container_add(GTK_CONTAINER(scrolledwindow),treeview);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledwindow, NULL);
}

void ListWin::SetModel(bool isListModel)
{
	if (isListModel) {
		if (!nowIsList) {
			gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(list_model));
			nowIsList = true;
		}
	}
	else {
		if (nowIsList) {
			gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(tree_model));
			nowIsList = false;
		}
	}
}
 
void ListWin::Destroy()
{
	g_object_unref (list_model);
	g_object_unref (tree_model);
}

void ListWin::Clear()
{
	gtk_list_store_clear(list_model);
	gtk_tree_store_clear(tree_model);
}

/*
void ListWin::RemoveLast()
{
	//note,this function is not to remove the list row,but the LIST_WIN_ROW_NUM row.	
	gchar lastrow[5];
	sprintf(lastrow,"%d", LIST_WIN_ROW_NUM); // not LIST_WIN_ROW_NUM -1,as Prepend is done first.
	GtkTreePath *path = gtk_tree_path_new_from_string (lastrow);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL(list_model), &iter, path)) {
		gtk_list_store_remove(list_model, &iter);
	}
	gtk_tree_path_free(path);
}

void ListWin::UnSelectAll()
{
	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)));
}

void ListWin::Prepend(const gchar *word)
{
	GtkTreeIter iter;
	gtk_list_store_prepend (list_model, &iter);
	gtk_list_store_set (list_model, &iter, 0, word, -1);
}
*/

void ListWin::ReScroll()
{
	GtkTreePath *path = gtk_tree_path_new_from_string ("0");
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview),path,NULL,false,0,0);
	gtk_tree_path_free(path);
	/*if (GTK_WIDGET_REALIZED(treeview))
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(treeview),0,0);*/
}

void ListWin::InsertLast(const gchar *word)
{
	GtkTreeIter iter;
	gtk_list_store_append (list_model, &iter);
	gtk_list_store_set (list_model, &iter, 0, word, -1);
}

void ListWin::SetTreeModel(std::vector<gchar *> *reslist)
{
	GtkTreeIter parent;
	GtkTreeIter iter;
	for (int i=0; i<gpAppFrame->oLibs.ndicts(); i++) {
		if (!reslist[i].empty()) {
			gtk_tree_store_append (tree_model, &parent, NULL);
			gtk_tree_store_set (tree_model, &parent, 0, gpAppFrame->oLibs.dict_name(i).c_str(), -1);
			for (std::vector<gchar *>::iterator p=reslist[i].begin(); p!=reslist[i].end(); ++p) {
				gtk_tree_store_append(tree_model, &iter, &parent);
				gtk_tree_store_set (tree_model, &iter, 0, *p, -1);
				g_free(*p);
			}
		}
	}
	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));
}

gboolean ListWin::on_button_press(GtkWidget * widget, GdkEventButton * event, ListWin *oListWin)
{
	if (event->type==GDK_2BUTTON_PRESS) {
		GtkTreeModel *model;
		GtkTreeIter iter;
		
		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oListWin->treeview));
		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			if (!oListWin->nowIsList) {
				if (gtk_tree_model_iter_has_child(model, &iter)) {
                                	GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                                	if (gtk_tree_view_row_expanded(GTK_TREE_VIEW (oListWin->treeview), path))
                                        	gtk_tree_view_collapse_row(GTK_TREE_VIEW (oListWin->treeview), path);
                                	else
                                        	gtk_tree_view_expand_row(GTK_TREE_VIEW (oListWin->treeview), path, false);
                                	gtk_tree_path_free(path);
					return true;
                        	}
			}
			gchar *word;
			gtk_tree_model_get (model, &iter, 0, &word, -1);
			gpAppFrame->ListClick(word);
			g_free(word);
		}		
		return true;
	} else {
		return false;
	}
}

void ListWin::on_selection_changed(GtkTreeSelection *selection, ListWin *oListWin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		if (!oListWin->nowIsList) {
			gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
			if (!strchr(path_str, ':')) { // bookname entry.
				g_free(path_str);
				return;
			}
			g_free(path_str);
		}
		gchar *word;
		gtk_tree_model_get (model, &iter, 0, &word, -1);
		gpAppFrame->SimpleLookupToTextWin(word,NULL);
		g_free(word);
	}
}

/**************************************************/
bool TreeWin::Create(GtkWidget *notebook)
{
  GtkTreeStore *model = 
		gpAppFrame->oTreeDicts.Load(
																conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_dirs_list"),
																conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_order_list"),
																conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list")
																);
	if (!model)
		return false;
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	gtk_widget_show(treeview);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("word", renderer,
						     "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (on_selection_changed), this);
	
	g_signal_connect (G_OBJECT (treeview), "button_press_event", G_CALLBACK (on_button_press), this);
	GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		
	gtk_container_add(GTK_CONTAINER(scrolledwindow),treeview);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledwindow, NULL);
	return true;
}

gboolean TreeWin::on_button_press(GtkWidget * widget, GdkEventButton * event, TreeWin *oTreeWin)
{
	if (event->type==GDK_2BUTTON_PRESS)
	{
		GtkTreeModel *model;
		GtkTreeIter iter;
		
		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oTreeWin->treeview));
		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			if (gtk_tree_model_iter_has_child(model, &iter)) {
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				if (gtk_tree_view_row_expanded(GTK_TREE_VIEW (oTreeWin->treeview), path))
					gtk_tree_view_collapse_row(GTK_TREE_VIEW (oTreeWin->treeview), path);
				else
					gtk_tree_view_expand_row(GTK_TREE_VIEW (oTreeWin->treeview), path, false);
				gtk_tree_path_free(path);
			}
			else {
				guint32 offset, size;
				gtk_tree_model_get (model, &iter, 1, &offset, 2, &size, -1);
				if (size != 0) {
					gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
					gint iTreeDict;
					gchar *p = strchr(path_str, ':');
					if (p)
						*p = '\0';
					iTreeDict = atoi(path_str);
					gpAppFrame->ShowTreeDictDataToTextWin(offset, size, iTreeDict);
					g_free(path_str);
				}
			}
		}		
		return true;
	}
	else
	{
		return false;
	}
}

void TreeWin::on_selection_changed(GtkTreeSelection *selection, TreeWin *oTreeWin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		guint32 offset, size;
		gtk_tree_model_get (model, &iter, 1, &offset, 2, &size, -1);
		if (size != 0) {
			gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
			gint iTreeDict;
			gchar *p = strchr(path_str, ':');
			if (p)
				*p = '\0';
			iTreeDict = atoi(path_str);
			gpAppFrame->ShowTreeDictDataToTextWin(offset, size, iTreeDict);
			g_free(path_str);
		}
	}
}


/**************************************************/
void ResultWin::Create(GtkWidget *notebook)
{
	GtkListStore *model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	gtk_widget_show(treeview);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("word", renderer,
						     "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (on_selection_changed), this);
	
	GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		
	gtk_container_add(GTK_CONTAINER(scrolledwindow),treeview);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledwindow, NULL);
}

void ResultWin::InsertLast(const gchar *word, const gchar *mark)
{
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
	GtkTreeIter iter;
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, 0, word, 1, mark, -1);
}

void ResultWin::Clear()
{
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
	gtk_list_store_clear(model);
}

void ResultWin::on_selection_changed(GtkTreeSelection *selection, ResultWin *oResultWin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *markstr;
		gtk_tree_model_get(model, &iter, 1, &markstr, -1);
		GtkTextView *textview = GTK_TEXT_VIEW(gpAppFrame->oMidWin.oTextWin.view->Widget());
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
		GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, markstr);
		g_free(markstr);
		if (mark) {
			gtk_text_view_scroll_to_mark(textview, mark, 0.0, TRUE, 0.0, 0.0);
		}
	}
}

/**************************************************/
IndexWin::IndexWin()
{
}

void IndexWin::Create(GtkWidget *hpaned)
{
	vbox = gtk_vbox_new(FALSE, 3);

	if (!conf->get_bool("/apps/stardict/preferences/main_window/hide_list"))
		gtk_widget_show(vbox);
	
	gtk_paned_pack1(GTK_PANED(hpaned),vbox,true,true);
	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(vbox),notebook, true, true, 0);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), false);
	oListWin.Create(notebook);
	oResultWin.Create(notebook);

	GtkWidget *table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 3);
	gtk_box_pack_start(GTK_BOX(vbox),table, false, false, 0);
	
	GtkWidget *wazard_button = gtk_radio_button_new(NULL);
	GTK_WIDGET_UNSET_FLAGS (wazard_button, GTK_CAN_FOCUS);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(wazard_button), false);
	gtk_table_attach(GTK_TABLE(table), wazard_button, 0, 1, 0, 1, (GtkAttachOptions)(GTK_EXPAND|GTK_SHRINK|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_SHRINK|GTK_FILL), 0, 0);
#ifdef CONFIG_GPE
	GtkWidget *image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.index_wazard.get());
	gtk_container_add (GTK_CONTAINER (wazard_button), image);
#else
	GtkWidget *hbox1 = gtk_hbox_new(false, 2);
	gtk_container_add (GTK_CONTAINER (wazard_button), hbox1);
	GtkWidget *image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.index_wazard.get());
	gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
	GtkWidget *label = gtk_label_new_with_mnemonic(_("_List"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), wazard_button);
#endif
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wazard_button), true);
	g_signal_connect(G_OBJECT(wazard_button),"toggled", G_CALLBACK(on_wazard_button_toggled), this);

	GtkWidget *result_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(wazard_button));
	GTK_WIDGET_UNSET_FLAGS (result_button, GTK_CAN_FOCUS);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(result_button), false);
	gtk_table_attach(GTK_TABLE(table), result_button, 1, 2, 0, 1, (GtkAttachOptions)(GTK_EXPAND|GTK_SHRINK|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_SHRINK|GTK_FILL), 0, 0);
#ifdef CONFIG_GPE
	image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.index_dictlist.get());
	gtk_container_add (GTK_CONTAINER (result_button), image);
#else
	hbox1 = gtk_hbox_new(false, 2);
	gtk_container_add (GTK_CONTAINER (result_button), hbox1);
	image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.index_dictlist.get());
	gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_Result"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), result_button);
#endif
	g_signal_connect(G_OBJECT(result_button),"toggled", G_CALLBACK(on_result_button_toggled), this);

	if (oTreeWin.Create(notebook)) {
		GtkWidget *appendix_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(wazard_button));
		GTK_WIDGET_UNSET_FLAGS (appendix_button, GTK_CAN_FOCUS);
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(appendix_button), false);
		gtk_table_attach(GTK_TABLE(table), appendix_button, 0, 2, 1, 2, (GtkAttachOptions)(GTK_EXPAND|GTK_SHRINK|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_SHRINK|GTK_FILL), 0, 0);
#ifdef CONFIG_GPE
		image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.index_appendix.get());
		gtk_container_add (GTK_CONTAINER (appendix_button), image);
#else
		hbox1 = gtk_hbox_new(false, 2);
		gtk_container_add (GTK_CONTAINER (appendix_button), hbox1);
		image = gtk_image_new_from_pixbuf(gpAppFrame->oAppSkin.index_appendix.get());
		gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
		label = gtk_label_new_with_mnemonic(_("_Tree"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), appendix_button);	
#endif
		g_signal_connect(G_OBJECT(appendix_button),"toggled", G_CALLBACK(on_appendix_button_toggled), this);
	}
	gtk_widget_show_all(table);
}

void IndexWin::on_wazard_button_toggled(GtkToggleButton *button, IndexWin *oIndexWin)
{
	if (gtk_toggle_button_get_active(button))
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oIndexWin->notebook), 0);
}

void IndexWin::on_result_button_toggled(GtkToggleButton *button, IndexWin *oIndexWin)
{
	if (gtk_toggle_button_get_active(button))
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oIndexWin->notebook), 1);
}

void IndexWin::on_appendix_button_toggled(GtkToggleButton *button, IndexWin *oIndexWin)
{
	if (gtk_toggle_button_get_active(button))
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oIndexWin->notebook), 2);
}

/************************************************/

void ToolWin::Create(GtkWidget *vbox)
{
	GtkWidget *hbox;
	hbox = gtk_hbox_new(false,0);
	gtk_widget_show(hbox);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,3);
#endif
	
	GtkWidget *image;
	ShowListButton=gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GOTO_LAST,GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(ShowListButton),image);
	gtk_button_set_relief (GTK_BUTTON (ShowListButton), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (ShowListButton, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(ShowListButton),"clicked", G_CALLBACK(ShowListCallback),NULL);
	g_signal_connect(G_OBJECT(ShowListButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),ShowListButton,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),ShowListButton,false,false,5);
#endif
	gtk_tooltips_set_tip(gpAppFrame->tooltips,ShowListButton,_("Show the word list"),NULL);
	
	HideListButton=gtk_button_new();	
	image = gtk_image_new_from_stock(GTK_STOCK_GOTO_FIRST,GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(HideListButton),image);
	gtk_button_set_relief (GTK_BUTTON (HideListButton), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (HideListButton, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(HideListButton),"clicked", G_CALLBACK(HideListCallback),NULL);
	g_signal_connect(G_OBJECT(HideListButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),HideListButton,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),HideListButton,false,false,5);
#endif
	gtk_tooltips_set_tip(gpAppFrame->tooltips, HideListButton,_("Hide the word list"),NULL);


	if (conf->get_bool("/apps/stardict/preferences/main_window/hide_list"))
		gtk_widget_show(ShowListButton);
	else
		gtk_widget_show(HideListButton);
	
	
	GtkWidget *button;
#ifndef CONFIG_GPE
	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_COPY,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(CopyCallback),this);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);	
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Copy"),NULL);
#endif

	PronounceWordButton=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(PronounceWordButton),gtk_image_new_from_stock(GTK_STOCK_EXECUTE,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(PronounceWordButton);
	gtk_button_set_relief (GTK_BUTTON (PronounceWordButton), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (PronounceWordButton, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(PronounceWordButton),"clicked", G_CALLBACK(PlayCallback),this);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),PronounceWordButton,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),PronounceWordButton,false,false,5);	
#endif
	gtk_tooltips_set_tip(gpAppFrame->tooltips,PronounceWordButton,_("Pronounce the word"),NULL);	
	gtk_widget_set_sensitive(PronounceWordButton, false);

	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_SAVE,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(SaveCallback),this);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);
#endif
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Save to file(Alt+E)"),NULL);
	
#ifndef CONFIG_GPE
	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_PRINT,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(PrintCallback),this);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);	
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Print"),NULL);
	gtk_widget_set_sensitive(button, false);
#endif

	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_FIND,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(SearchCallback),this);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);	
#endif
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Search in the definition (Ctrl+F)"),NULL);
}

void ToolWin::ShowListCallback(GtkWidget *widget, gpointer data)
{
  conf->set_bool("/apps/stardict/preferences/main_window/hide_list", false);
}

void ToolWin::HideListCallback(GtkWidget *widget, gpointer data)
{
  conf->set_bool("/apps/stardict/preferences/main_window/hide_list", true);
}

#ifndef CONFIG_GPE
void ToolWin::CopyCallback(GtkWidget *widget, ToolWin *oToolWin)
{
  std::string text = gpAppFrame->oMidWin.oTextWin.view->GetText();

  GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text(clipboard, text.c_str(), -1);
}
#endif

void ToolWin::PlayCallback(GtkWidget *widget, ToolWin *oToolWin)
{
	gpAppFrame->oTopWin.InsertHisList(gpAppFrame->oTopWin.GetText());
	gpAppFrame->oTopWin.InsertBackList();
	gpAppFrame->oReadWord.read(gpAppFrame->oMidWin.oTextWin.pronounceWord.c_str());
}

void ToolWin::do_save()
{
	TextWin &oTextWin=gpAppFrame->oMidWin.oTextWin;

	if (conf->get_bool("/apps/stardict/preferences/dictionary/only_export_word")) {
		if (!oTextWin.queryWord.empty()) {
			FILE *fp = fopen(conf->get_string("/apps/stardict/preferences/dictionary/export_file").c_str(), "a+");
        	        if(fp) {
				fputs(oTextWin.queryWord.c_str(),fp);
				fputs("\n",fp);
				fclose(fp);
			}
		}
	} else {
		// check for selections in Text Area
		GtkTextIter start, end;
		gchar *str = NULL;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTextWin.view->Widget()));
		if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end))
			 str = gtk_text_buffer_get_text(buffer, &start, &end, false);
		FILE *fp = fopen(conf->get_string("/apps/stardict/preferences/dictionary/export_file").c_str(), "a+");
		if(fp) {
			if(str) {
				fputs(oTextWin.queryWord.c_str(),fp);
				fputs("\n",fp);
				fputs(str,fp);
				fputs("\n",fp);
			} else {
				fputs(oTextWin.view->GetText().c_str(),fp);
			}
			//separat line
			fputs("\n",fp);
			fclose(fp);
		}
		if (str)
			g_free(str);
	}
	play_sound_on_event("buttonactive");
}

void ToolWin::SaveCallback(GtkWidget *widget, ToolWin *oToolWin)
{
	oToolWin->do_save();
}

void ToolWin::PrintCallback(GtkWidget *widget, ToolWin *oToolWin)
{
}

void ToolWin::do_search()
{
	TextWin &oTextWin=gpAppFrame->oMidWin.oTextWin;
	oTextWin.ShowSearchPanel();
	oTextWin.search_from_beginning = TRUE;
		
	GtkTextIter start,end;
	GtkTextBuffer *buffer = 
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTextWin.view->Widget()));
	if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
		if (gtk_text_iter_get_offset(&end) - gtk_text_iter_get_offset(&start) < 80) {			
			gchar *str = gtk_text_buffer_get_text(buffer, &start, &end, false);
			oTextWin.find_text = str;
      g_free(str);
		}
	}

	if (!oTextWin.find_text.empty())
		gtk_entry_set_text(oTextWin.eSearch, oTextWin.find_text.c_str());
}

void ToolWin::SearchCallback(GtkWidget *widget, ToolWin *oToolWin)
{    
	oToolWin->do_search();
}


/**********************************************/

void TextWin::OnCloseSearchPanel(GtkWidget *widget, TextWin *oTextWin)
{
  oTextWin->HideSearchPanel();
}

gboolean TextWin::OnSearchKeyPress(GtkWidget *widget, GdkEventKey *event,
																	 TextWin *oTextWin)
{  
  if (event->type==GDK_KEY_PRESS && event->keyval==GDK_Return)
    gtk_button_clicked(oTextWin->btFind);
    
  return FALSE;
}

void TextWin::OnFindSearchPanel(GtkWidget *widget, TextWin *oTextWin)
{
  const gchar *text=gtk_entry_get_text(oTextWin->eSearch);
  if (text && *text)
    if (oTextWin->Find(text, oTextWin->search_from_beginning)) {
      oTextWin->search_from_beginning = FALSE;

      oTextWin->find_text = text;
    } else
      oTextWin->search_from_beginning = TRUE;
  
}

void TextWin::ShowSearchPanel()
{
  gtk_widget_show_all(hbSearchPanel);
  gtk_widget_grab_focus(GTK_WIDGET(eSearch));
}

void TextWin::Create(GtkWidget *vbox)
{
  view.reset(new ArticleView(GTK_BOX(vbox)));

  g_signal_connect(G_OBJECT(view->Widget()), "button_press_event", 
		   G_CALLBACK(on_button_press), this);
  g_signal_connect(G_OBJECT(view->Widget()), "selection_received", 
		   G_CALLBACK(SelectionCallback), this);  

	hbSearchPanel = gtk_hbox_new(FALSE, 0);
  btClose = GTK_BUTTON(gtk_button_new());
  GtkWidget *image =
    gtk_image_new_from_stock(GTK_STOCK_CANCEL,
           GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show(image);
  gtk_container_add(GTK_CONTAINER(btClose), image);
  gtk_button_set_relief(btClose, GTK_RELIEF_NONE);
  GTK_WIDGET_UNSET_FLAGS(btClose, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(btClose), "clicked",
									 G_CALLBACK(OnCloseSearchPanel), this);
	gtk_box_pack_start(GTK_BOX(hbSearchPanel), GTK_WIDGET(btClose), FALSE, FALSE, 3);
	eSearch = GTK_ENTRY(gtk_entry_new());
	gtk_widget_set_size_request(GTK_WIDGET(eSearch), 20, -1);
  g_signal_connect(G_OBJECT(eSearch), "key_press_event",
									 G_CALLBACK(OnSearchKeyPress), this);

  gtk_box_pack_start(GTK_BOX(hbSearchPanel), GTK_WIDGET(eSearch), TRUE, TRUE, 0);
	btFind = GTK_BUTTON(gtk_button_new_from_stock("gtk-find"));
  gtk_button_set_relief(btFind, GTK_RELIEF_NONE);
  GTK_WIDGET_UNSET_FLAGS(btFind, GTK_CAN_FOCUS);
  g_signal_connect(G_OBJECT(btFind), "clicked",
									 G_CALLBACK(OnFindSearchPanel), this);

  gtk_box_pack_start(GTK_BOX(hbSearchPanel), GTK_WIDGET(btFind), FALSE, FALSE, 5);

  //gtk_widget_show_all(hbSearchPanel);
  gtk_box_pack_start(GTK_BOX(vbox), hbSearchPanel, FALSE, FALSE, 0);

}

void TextWin::ShowInitFailed()
{
	Show(_("Warning! No dictionary is loaded.\nPlease go to StarDict's website and download some dictionaries:\nhttp://stardict.sourceforge.net"));
}

void TextWin::ShowTips()
{
  query_result = TEXT_WIN_TIPS;
  view->SetText(
		_(
		  "        Welcome to StarDict!\n\n"
		  "   Press Ctrl+Q to quit. Press Alt+Z to iconify the window or Alt+X to hide the window.\n"
		  "   Press Alt+C or ESC to clear the input entry's text.\n"
		  "   Press Space key to move focus to the input entry.\n"
		  "   If the query word was not found, you can press Tab key to select the first word in the word list.\n"
		  "   After selected some text, clicking the middle mouse button on the main window's Definition area or on the notification area icon will look up that word.\n"
		  "   StarDict can match strings against patterns containing '*' (wildcard) and '?' (joker).\n"
		  "   Input a word beginning with \'/\' to do a Fuzzy query.\n"
		  "   When the floating window reports that a word was not found, double clicking will perform a fuzzy query.\n"
		  )
		);
  view->ScrollTo(0);
}

void TextWin::ShowInfo()
{
  query_result = TEXT_WIN_INFO;
  view->SetText(
		_(
		  "       Welcome to StarDict\n"
		  "StarDict is a Cross-Platform and international dictionary written in Gtk2. "
		  "It has powerful features such as \"Glob-style pattern matching,\" \"Scan selected word,\" \"Fuzzy query,\" etc.\n\n"
		  "       Here is an introduction to using StarDict:\n\n"
		  "       1. Glob-style pattern matching\n"
		  "       You can input strings containing \'*\' (wildcard) and \'?\' (joker) as the pattern. "
		  "\'*\' matches an arbitrary, possibly empty, string, and \'?\' matches an arbitrary character. "
		  "After pressing Enter, the words that match this pattern will be shown in the list.\n"
		  "       2. Fuzzy query\n"
		  "       When you can't remember how to spell a word exactly, you can try StarDict's Fuzzy query. "
		  "It uses \"Levenshtein Edit Distance\" to compute the similarity between two words, and gives the match results which are most "
		  "similar to the word that you input. "
		  "To create a fuzzy query, just input the word with a beginning \"/\", and then press Enter.\n"
		  "       3. Full-text search\n"
		  "       Full-text search searchs a word in the data. To create a Full-text search, just input the word with a beginning \"|\", and then press Enter. For example, \"|ab cd\" searchs data which contain both \"ab\" and \"cd\". If the words contain Space character, you can use \"\\ \", such as \"|apple\\ pie\", other escaping characters are \"\\\\\" for \'\\\', \"\\t\" for Tab and \"\\n\" for new line.\n"
		  "       4. Special character search\n"
		  "       If your words contain special characters, you can use \'\\\' to escape it, for example, \"a\\*b\\?\" searchs \"a*b?\", \"\\/abc\" searchs \"/abc\".\n"
		  "       5. Scan selected word\n"
		  "       Turn on the check button at the bottom-left corner of the StarDict window to activate this feature. "
		  "When this feature is on, StarDict will automatically look up words, phrases, and Chinese characters in other applications. "
		  "Just highlight a word or phrase with your mouse, and a floating window will pop up showing the definition of the "
		  "selected word.\n"
		  "       6. Dictionary management\n"
		  "       Click the \"Manage dictionaries\" button at the bottom-right corner of the window to access the dictionary management "
		  "dialog. From here, you can disable some dictionaries that you don't need, and set the dictionaries\' querying order.\n"
		  "\n\n"
		  "       Statement: This program is distributed in the hope that it will be useful, "
		  "but WITHOUT ANY WARRANTY; without the warranty "
		  "that the word spelling, definition, and phonetic information are correct.\n"
		  )
		);

  view->ScrollTo(0);
}

void TextWin::Show(const gchar *str)
{
  view->SetText(str);
  view->ScrollTo(0);
}

void TextWin::Show(gchar ***Word, gchar ****WordData)
{
	view->BeginUpdate();
	view->Clear();
	view->GotoBegin();

	int j,k;
	for (int i=0; i<gpAppFrame->oLibs.ndicts(); i++) {
		if (Word[i]) {
			view->AppendHeader(gpAppFrame->oLibs.dict_name(i), i);
			j=0;
			do {
				view->AppendWord(Word[i][j]);
				view->AppendData(WordData[i][j][0], Word[i][j]);
				view->AppendNewline();
				k=1;
				while (WordData[i][j][k]) {
					view->AppendDataSeparate();
					view->AppendData(WordData[i][j][k], Word[i][j]);
					view->AppendNewline();
					k++;
				}
				j++;
			} while (Word[i][j]);
		}
	}
	view->EndUpdate();
}

void TextWin::ShowTreeDictData(gchar *data)
{
	view->BeginUpdate();
	view->Clear();
	view->GotoBegin();
	if (data) {
		view->AppendData(data, "");
		view->AppendNewline();
	}
	view->EndUpdate();
}

gboolean TextWin::Find (const gchar *text, gboolean start)
{    
  GtkTextBuffer *buffer = 
    gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->Widget()));

  GtkTextIter iter;
  if (start) 
    gtk_text_buffer_get_start_iter(buffer, &iter);
  else {
    GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, "last_search");
    	
    if (mark)
      gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    else
      gtk_text_buffer_get_start_iter(buffer, &iter);
  }
    
  GtkTextIter match_start, match_end;
  if (gtk_text_iter_forward_search(&iter, text,
				   (GtkTextSearchFlags) (GTK_TEXT_SEARCH_VISIBLE_ONLY | GTK_TEXT_SEARCH_TEXT_ONLY),
				   &match_start, &match_end,
				   NULL)) {
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view->Widget()), &match_start,
				 0.0, TRUE, 0.5, 0.5);
    gtk_text_buffer_place_cursor(buffer, &match_end);
    gtk_text_buffer_move_mark(buffer,
			      gtk_text_buffer_get_mark(buffer, "selection_bound"),
			      &match_start);
    gtk_text_buffer_create_mark(buffer, "last_search", &match_end, FALSE);
    
    return TRUE;
  } else {
    GtkWidget *message_dlg = 
      gtk_message_dialog_new(
			      GTK_WINDOW(gpAppFrame->window),
			      (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			      GTK_MESSAGE_INFO,
			      GTK_BUTTONS_OK,
			      _("The text \"%s\" was not found."), text);
    
    gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);
    
    gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);
    
    if (gtk_dialog_run(GTK_DIALOG(message_dlg))!=GTK_RESPONSE_NONE)
      gtk_widget_destroy(message_dlg);
    return FALSE;
  }
}

gboolean TextWin::on_button_press(GtkWidget * widget, GdkEventButton * event, TextWin *oTextWin)
{
	if (event->button==2)
	{
		gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, gpAppFrame->oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
		return true;
	}
	else
	{
		return false;
	}
}

void TextWin::SelectionCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, TextWin *oTextWin)
{
	gchar *result;
	result = (gchar *)gtk_selection_data_get_text (selection_data);
	if (!result) {
		/* If we asked for UTF8 and didn't get it, try compound_text;
		* if we asked for compound_text and didn't get it, try string;
		* If we asked for anything else and didn't get it, give up.
		*/
		if (selection_data->target == gpAppFrame->oSelection.UTF8_STRING_Atom) {
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, gpAppFrame->oSelection.COMPOUND_TEXT_Atom, GDK_CURRENT_TIME);
		}
		else if (selection_data->target == gpAppFrame->oSelection.COMPOUND_TEXT_Atom)
		{
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, GDK_TARGET_STRING, GDK_CURRENT_TIME);
		}
		return;
    }
	gpAppFrame->Query(result);
	g_free (result);
}

/*********************************************/
void MidWin::Create(GtkWidget *vbox)
{
	hpaned = gtk_hpaned_new();	
	gtk_widget_show(hpaned);
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE,0);

	oIndexWin.Create(hpaned);

	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox1);
	gtk_paned_pack2(GTK_PANED(hpaned), vbox1, TRUE, FALSE);	
	oToolWin.Create(vbox1);
	oTextWin.Create(vbox1);	

	int pos=conf->get_int("/apps/stardict/preferences/main_window/hpaned_pos");

	gtk_paned_set_position(GTK_PANED(hpaned), pos);
}


/*********************************************/
BottomWin::BottomWin()
{
	SearchWebsiteMenu = NULL;
}

void BottomWin::Create(GtkWidget *vbox)
{
	GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
#else
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
#endif
	
	ScanSelectionCheckButton = gtk_check_button_new_with_mnemonic(_("_Scan"));
	gtk_widget_show(ScanSelectionCheckButton);
	GTK_WIDGET_UNSET_FLAGS (ScanSelectionCheckButton, GTK_CAN_FOCUS);
  bool scan=conf->get_bool("/apps/stardict/preferences/dictionary/scan_selection");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ScanSelectionCheckButton), scan);
	g_signal_connect(G_OBJECT(ScanSelectionCheckButton), "toggled", 
									 G_CALLBACK(ScanCallback), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),ScanSelectionCheckButton,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,ScanSelectionCheckButton,_("Scan the selection"),NULL);

	GtkWidget *button = gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(AboutCallback), NULL);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,2);
#else
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,8);
#endif
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Show info"),NULL);

	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_QUIT,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(QuitCallback), NULL);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Quit"), NULL);


	// the next buttons will be pack from right to left.	
#ifndef CONFIG_GPE
	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(PreferenceCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Preferences"),NULL);

	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(DictManageCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Manage dictionaries"),NULL);

	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_INDEX,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(NewVersionCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Go to the StarDict website"),NULL);
#endif

	button=gtk_button_new();	
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_JUMP_TO,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(InternetSearchCallback),this);
	g_signal_connect(G_OBJECT(button),"button_press_event", G_CALLBACK(on_internetsearch_button_press),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Search an Internet dictionary - Right button: website list"),NULL);
}

void BottomWin::ScanCallback(GtkToggleButton *button, gpointer data)
{
	bool scan_selection=gtk_toggle_button_get_active(button);
  conf->set_bool("/apps/stardict/preferences/dictionary/scan_selection",
									scan_selection);
}

void BottomWin::AboutCallback(GtkButton *button, gpointer data)
{
	play_sound_on_event("buttonactive");	
	gpAppFrame->oMidWin.oTextWin.ShowInfo();
}

void BottomWin::QuitCallback(GtkButton *button, gpointer data)
{
	play_sound_on_event("buttonactive");
	gpAppFrame->Quit();	
}

gboolean BottomWin::on_internetsearch_button_press(GtkWidget * widget, GdkEventButton * event , BottomWin *oBottomWin)
{
	if (event->button == 3) {
		const std::list<std::string> &list=
			conf->get_strlist("/apps/stardict/preferences/main_window/search_website_list");
		if (list.empty())
			return true;
	
		if (oBottomWin->SearchWebsiteMenu)
			gtk_widget_destroy(oBottomWin->SearchWebsiteMenu);
			
		oBottomWin->SearchWebsiteMenu = gtk_menu_new();
		
		GtkWidget *menuitem;
		
		for (std::list<std::string>::const_iterator ci=list.begin(); ci!=list.end(); ++ci) {
			std::vector<std::string> web_list = split(*ci, '\t');
			if (web_list.size()==3 && web_list[2].find("%s")!=std::string::npos) {
				menuitem = gtk_image_menu_item_new_with_label(web_list[0].c_str());
					g_signal_connect(G_OBJECT(menuitem), "activate",
													 G_CALLBACK(on_internetsearch_menu_item_activate),
													 const_cast<char *>(ci->c_str()));
					gtk_menu_shell_append(GTK_MENU_SHELL(oBottomWin->SearchWebsiteMenu), menuitem);
			}
		}

		gtk_widget_show_all(oBottomWin->SearchWebsiteMenu);
		gtk_menu_popup(GTK_MENU(oBottomWin->SearchWebsiteMenu), NULL, NULL, NULL, NULL, event->button, event->time);		
		return true;
	}
	return false;
}

void BottomWin::on_internetsearch_menu_item_activate(GtkMenuItem *menuitem, const gchar *website)
{
	std::vector<std::string> weblist = split(website, '\t');

	if (weblist[2].find("%s")==std::string::npos)
		return;

	const gchar *text;
	text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry));
	if (text[0]) {		
	  gchar *url = g_strdup_printf(weblist[2].c_str(), text);
	  show_url(url);
		g_free(url);
	} else
	  show_url(weblist[1].c_str());	
}

void BottomWin::InternetSearchCallback(GtkButton *button, BottomWin *oBottomWin)
{	
	const std::list<std::string> &search_website_list=
		conf->get_strlist("/apps/stardict/preferences/main_window/search_website_list");
	if (search_website_list.empty())
		return;

	std::vector<std::string> weblist = 
		split(search_website_list.front(), '\t');

	if (weblist.size()!=3)
		return;

	if (weblist[2].find("%s")==std::string::npos)
		return;

	const gchar *text;
	text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gpAppFrame->oTopWin.WordCombo)->entry));
	if (text[0]) {		
	  gchar *url = g_strdup_printf(weblist[2].c_str(), text);
	  show_url(url);
		g_free(url);
	} else
	  show_url(weblist[1].c_str());
}

#ifndef CONFIG_GPE
void BottomWin::NewVersionCallback(GtkButton *button, BottomWin *oBottomWin)
{
  show_url("http://stardict.sourceforge.net");
}

void BottomWin::DictManageCallback(GtkButton *button, BottomWin *oBottomWin)
{
	gpAppFrame->PopupDictManageDlg();
}

void BottomWin::PreferenceCallback(GtkButton *button, BottomWin *oBottomWin)
{
	gpAppFrame->PopupPrefsDlg();
}
#endif
