#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <sys/stat.h>
#include <glib/gi18n.h>
#include <algorithm>

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#endif

#include "conf.h"
#include "lib/common.hpp"
#include "utils.h"
#include "lib/file.hpp"

#include "dictmanagedlg.h"

DictManageDlg::DictManageDlg(GtkWindow *pw, GdkPixbuf *di,  GdkPixbuf *tdi) :
	parent_win(pw), dicts_icon(di), tree_dicts_icon(tdi), window(NULL)
{
}

void DictManageDlg::response_handler (GtkDialog *dialog, gint res_id, DictManageDlg *oDictManageDlg)
{
	if (res_id==GTK_RESPONSE_HELP)
    show_help("stardict-dictmanage");
}

void DictManageDlg::on_wazard_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button))
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 0);
}

void DictManageDlg::on_appendix_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button))
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 1);
}

class GetInfo {
public:
	GetInfo(GtkListStore *model_, bool istreedict_):
		model(model_), istreedict(istreedict_) {}
	void operator()(const std::string& url, bool disable) {
		DictInfo dictinfo;
		if (dictinfo.load_from_ifo_file(url.c_str(), istreedict)) {
			GtkTreeIter iter;
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter, 
												 0, !disable, 
												 1, dictinfo.bookname.c_str(), 
												 2, dictinfo.wordcount, 
												 3, dictinfo.author.c_str(), 
												 4, dictinfo.email.c_str(), 
												 5, dictinfo.website.c_str(),
												 6, dictinfo.description.c_str(), 
												 7, dictinfo.date.c_str(), 
												 8, url.c_str(), 
												 -1);
		}
	}
private:
	GtkListStore *model;
	bool istreedict;
};

static GtkTreeModel* __create_dict_tree_model(const std::list<std::string> &dirs,
																							const std::list<std::string>& order_list,
																							const std::list<std::string>& disable_list, bool istreedict)
{
	GtkListStore *model = 
		gtk_list_store_new(9, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_LONG, 
					 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	for_each_file(dirs, ".ifo", order_list, disable_list, 
								GetInfo(model, istreedict));
	
	return GTK_TREE_MODEL(model);
}

GtkTreeModel* DictManageDlg::create_dict_tree_model(bool istreedict)
{
	if (istreedict) {
		return __create_dict_tree_model(
																		conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_dirs_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_order_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list"),
																		istreedict
																		);
	} else {
		return __create_dict_tree_model(
																		conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/dict_disable_list"),
																		istreedict
																		);
	}		
	
}

void DictManageDlg::on_dict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg)
{
	GtkTreeModel *model = oDictManageDlg->dict_tree_model;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean enable;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &enable, -1);

	enable = !enable;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, enable, -1);

	gtk_tree_path_free (path);

	gboolean have_iter;
	gchar *filename;
	std::list<std::string> disable_list;
	
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &enable, -1);
		if (!enable) {
			gtk_tree_model_get (model, &iter, 8, &filename, -1);
			disable_list.push_back(filename);
			g_free(filename);
		}
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	conf->set_strlist("/apps/stardict/manage_dictionaries/dict_disable_list", disable_list);
}

void DictManageDlg::on_treedict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg)
{
	GtkTreeModel *model = oDictManageDlg->treedict_tree_model;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean enable;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &enable, -1);

	enable = !enable;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, enable, -1);

	gtk_tree_path_free (path);

	gboolean have_iter;
	gchar *filename;
	std::list<std::string> disable_list;
	
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &enable, -1);
		if (!enable) {
			gtk_tree_model_get (model, &iter, 8, &filename, -1);
			disable_list.push_back(filename);
			g_free(filename);
		}
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list", disable_list);
}

void DictManageDlg::drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, DictManageDlg *oDictManageDlg)
{
	if (data->target == gdk_atom_intern("STARDICT_DICTMANAGE", FALSE)) {
		GtkTreeRowReference *ref;
		GtkTreePath *source_row;

		ref = (GtkTreeRowReference *)g_object_get_data(G_OBJECT(ctx), "gtk-tree-view-source-row");
		source_row = gtk_tree_row_reference_get_path(ref);

		if (source_row == NULL)
			return;

		GtkTreeIter iter;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)))
			gtk_tree_model_get_iter(oDictManageDlg->dict_tree_model, &iter, source_row);
		else
			gtk_tree_model_get_iter(oDictManageDlg->treedict_tree_model, &iter, source_row);

		gtk_selection_data_set(data, gdk_atom_intern("STARDICT_DICTMANAGE", FALSE), 8, (const guchar *)&iter, sizeof(iter));

		gtk_tree_path_free(source_row);
	}
}

void DictManageDlg::drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg)
{
	if (sd->target == gdk_atom_intern("STARDICT_DICTMANAGE", FALSE) && sd->data) {
		GtkTreePath *path = NULL;
		GtkTreeViewDropPosition position;

		GtkTreeIter drag_iter;
		memcpy(&drag_iter, sd->data, sizeof(drag_iter));

		if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &position)) {

			GtkTreeIter iter;
			
			GtkTreeModel *model;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)))
				model = oDictManageDlg->dict_tree_model;
			else
				model = oDictManageDlg->treedict_tree_model;
			
			gtk_tree_model_get_iter(model, &iter, path);

			switch (position) {
				case GTK_TREE_VIEW_DROP_AFTER:
				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					gtk_list_store_move_after(GTK_LIST_STORE(model), &drag_iter, &iter);
					break;

				case GTK_TREE_VIEW_DROP_BEFORE:
				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					gtk_list_store_move_before(GTK_LIST_STORE(model), &drag_iter, &iter);
					//Bug: when move the entry to before the first item, it is moved to the end! But I think this is gtk's bug.
					break;
				default:
					return;
			}
			oDictManageDlg->write_order_list(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)));
		}
	}
}

gboolean DictManageDlg::on_treeview_button_press(GtkWidget * widget, GdkEventButton * event, DictManageDlg *oDictManageDlg)
{
	if (event->type==GDK_2BUTTON_PRESS) {
		GtkTreeModel *model;
		GtkTreeIter iter;

		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			gchar *dictname, *author, *email, *website, *description, *date, *filename;
			glong wordcount;
			gtk_tree_model_get (model, &iter, 1, &dictname, 2, &wordcount, 3, &author, 4, &email, 5, &website, 6, &description, 7, &date, 8, &filename, -1);
			GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(oDictManageDlg->window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				"<b>%s:</b> %s\n<b>%s:</b> %ld\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s",
				_("Dictionary Name"), dictname,
				_("Word count"), wordcount,
				_("Author"), author,
				_("Email"), email,
				_("Website"), website,
				_("Description"), description,
				_("Date"), date,
				_("File name"), filename);
			g_free(dictname);
			g_free(author);
			g_free(email);
			g_free(website);
			g_free(description);
			g_free(date);
			g_free(filename);
			if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_NONE)
				gtk_widget_destroy (dialog);
		}
		return true;
	} else {
		return false;
	}
}

GtkWidget *DictManageDlg::create_dict_tree(gboolean istreedict)
{	
	GtkWidget *sw;	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
      	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	
	gtk_widget_set_size_request (sw, 350, 230);

	GtkTreeModel *now_tree_model = create_dict_tree_model (istreedict);
	if (istreedict)
		treedict_tree_model = now_tree_model;
	else
		dict_tree_model = now_tree_model;

	GtkWidget *now_treeview = gtk_tree_view_new_with_model (now_tree_model);
	g_signal_connect(G_OBJECT(now_treeview), "button_press_event",
                   G_CALLBACK(on_treeview_button_press), this);
	if (istreedict)
		treedict_treeview = now_treeview;
	else
		dict_treeview = now_treeview;
	g_object_unref (G_OBJECT (now_tree_model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (now_treeview), TRUE);
	
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	renderer = gtk_cell_renderer_toggle_new ();
	if (istreedict)
		g_signal_connect (renderer, "toggled", G_CALLBACK (on_treedict_enable_toggled), this);
	else
		g_signal_connect (renderer, "toggled", G_CALLBACK (on_dict_enable_toggled), this);
	column = gtk_tree_view_column_new_with_attributes (_("Enable"), renderer, "active", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	
	renderer = gtk_cell_renderer_text_new ();
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  	
	column = gtk_tree_view_column_new_with_attributes (_("Dictionary Name"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Word count"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	/*renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Author"), renderer, "text", 3, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Email"), renderer, "text", 4, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Website"), renderer, "text", 5, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Description"), renderer, "text", 6, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Date"), renderer, "text", 7, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("File name"), renderer, "text", 8, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);*/

	GtkTargetEntry gte[] = {{"STARDICT_DICTMANAGE", GTK_TARGET_SAME_APP, 0}};
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(now_treeview), GDK_BUTTON1_MASK, gte, 1, GDK_ACTION_COPY);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(now_treeview), gte, 1, (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE));

	g_signal_connect(G_OBJECT(now_treeview), "drag-data-received", G_CALLBACK(drag_data_received_cb), this);
	g_signal_connect(G_OBJECT(now_treeview), "drag-data-get", G_CALLBACK(drag_data_get_cb), this);

	gtk_container_add (GTK_CONTAINER (sw), now_treeview);
	return sw;
}

void DictManageDlg::write_order_list(bool istreedict)
{
	GtkTreeIter iter;
	gboolean have_iter;
	gchar *filename;
	std::list<std::string> order_list;
	GtkTreeModel *now_tree_model;
	
	if (istreedict)
		now_tree_model = treedict_tree_model;
	else
		now_tree_model = dict_tree_model;
	
	have_iter = gtk_tree_model_get_iter_first(now_tree_model, &iter);
	while (have_iter) {
		gtk_tree_model_get (now_tree_model, &iter, 8, &filename, -1);
		order_list.push_back(filename);
		g_free(filename);
		have_iter = gtk_tree_model_iter_next(now_tree_model, &iter);
	}
	
	if (istreedict)
	  conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_order_list", order_list);
	else
	  conf->set_strlist("/apps/stardict/manage_dictionaries/dict_order_list", order_list);
}

void DictManageDlg::on_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button));	
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_treeview;
	
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* first_path = gtk_tree_path_new_first();
		GtkTreePath* now_path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_compare(first_path, now_path)!=0) {
			gtk_list_store_move_after(GTK_LIST_STORE(model), &iter, NULL);
			
			gtk_tree_selection_select_path(selection, first_path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), first_path, NULL, false, 0, 0);
			oDictManageDlg->write_order_list(istreedict);
		}
		gtk_tree_path_free(first_path);
		gtk_tree_path_free(now_path);
	}
}

void DictManageDlg::on_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button));	
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_treeview;

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreeIter tmp,last;
		tmp = last = iter;
		while (gtk_tree_model_iter_next(model, &tmp))
			last = tmp;
		GtkTreePath* now_path = gtk_tree_model_get_path(model, &iter);
		GtkTreePath* last_path = gtk_tree_model_get_path(model, &last);
		if (gtk_tree_path_compare(last_path, now_path)!=0) {
			gtk_list_store_move_after(GTK_LIST_STORE(model), &iter, &last);			
			
			gtk_tree_selection_select_path(selection, last_path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), last_path, NULL, false, 0, 0);
			oDictManageDlg->write_order_list(istreedict);
		}
		gtk_tree_path_free(last_path);
		gtk_tree_path_free(now_path);
	}	
}

void DictManageDlg::on_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button));	
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_treeview;

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			GtkTreeIter prev;
			gtk_tree_model_get_iter(model, &prev, path);
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &prev);	
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), path, NULL, false, 0, 0);
			oDictManageDlg->write_order_list(istreedict);
		}		
		gtk_tree_path_free(path);		
	}	
}

void DictManageDlg::on_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button));	
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_treeview;

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_next(path);
		GtkTreeIter next;
		if (gtk_tree_model_get_iter(model, &next, path)) {
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &next);	
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), path, NULL, false, 0, 0);
			oDictManageDlg->write_order_list(istreedict);
		}		
		gtk_tree_path_free(path);		
	}	
}

GtkWidget *DictManageDlg::create_buttons()
{
	GtkWidget *vbox;
#ifdef CONFIG_GPE
	vbox = gtk_vbox_new(false,2);
#else
	vbox = gtk_vbox_new(false,6);
#endif
	GtkWidget *button;
	button = gtk_button_new_from_stock(GTK_STOCK_GOTO_TOP);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_top_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_up_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_down_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GOTO_BOTTOM);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_bottom_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	return vbox;
}

bool DictManageDlg::Show()
{
	if (!window) {	
		window = gtk_dialog_new();
		gtk_window_set_transient_for(GTK_WINDOW(window), parent_win);

		gtk_dialog_add_button(GTK_DIALOG(window),
													GTK_STOCK_HELP,
													GTK_RESPONSE_HELP);
	
		gtk_dialog_add_button (GTK_DIALOG (window),
													 GTK_STOCK_CLOSE,
													 GTK_RESPONSE_CLOSE);
		gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_CLOSE);
		g_signal_connect(G_OBJECT(window), "response",
										 G_CALLBACK(response_handler), this);
		
		GtkWidget *vbox;
#ifdef CONFIG_GPE
		vbox = gtk_vbox_new (FALSE, 2);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
#else
		vbox = gtk_vbox_new (FALSE, 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
#endif
		
		GtkWidget *hbox = gtk_hbox_new(false, 3);
		gtk_box_pack_start(GTK_BOX(vbox),hbox, false, false, 0);
		
		wazard_button = gtk_radio_button_new(NULL);
		GTK_WIDGET_UNSET_FLAGS (wazard_button, GTK_CAN_FOCUS);
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(wazard_button), false);
		gtk_box_pack_start (GTK_BOX (hbox), wazard_button, false, false, 0);	
		GtkWidget *hbox1 = gtk_hbox_new(false, 2);
		gtk_container_add (GTK_CONTAINER (wazard_button), hbox1);
		GtkWidget *image = gtk_image_new_from_pixbuf(dicts_icon);
		gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
		GtkWidget *label = gtk_label_new_with_mnemonic(_("D_ictionaries"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), wazard_button);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wazard_button), true);
		g_signal_connect(G_OBJECT(wazard_button),"toggled", G_CALLBACK(on_wazard_button_toggled), this);
		
		GtkWidget *appendix_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(wazard_button));
		GTK_WIDGET_UNSET_FLAGS (appendix_button, GTK_CAN_FOCUS);
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(appendix_button), false);
		gtk_box_pack_start (GTK_BOX (hbox), appendix_button, false, false, 0);	
		hbox1 = gtk_hbox_new(false, 2);
		gtk_container_add (GTK_CONTAINER (appendix_button), hbox1);
		image = gtk_image_new_from_pixbuf(tree_dicts_icon);
		gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
		label = gtk_label_new_with_mnemonic(_("T_ree dictionaries"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), appendix_button);		
		g_signal_connect(G_OBJECT(appendix_button),"toggled", G_CALLBACK(on_appendix_button_toggled), this);
		
#ifdef CONFIG_GPE
		hbox = gtk_hbox_new (FALSE, 2);
#else
		hbox = gtk_hbox_new (FALSE, 18);
#endif
		gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 0);
		
		notebook = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(hbox),notebook, true, true, 0);
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), false);
		
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(false), NULL);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(true), NULL);
		

		GtkWidget *buttons = create_buttons ();
		gtk_box_pack_start (GTK_BOX (hbox), buttons, false, false, 0);		
		
		label = gtk_label_new_with_mnemonic (_("These settings will take effect the next time you run StarDict."));
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
		g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
		gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox,
												true, true, 0);
		
		
		gtk_widget_show_all(GTK_DIALOG (window)->vbox);	
	
		gtk_window_set_title(GTK_WINDOW (window), _("Manage Dictionaries"));	
	}
	gint result;
	while ((result = gtk_dialog_run(GTK_DIALOG(window)))==GTK_RESPONSE_HELP)
		;
	if (result == GTK_RESPONSE_NONE) {
		// Caused by gtk_widget_destroy(), quiting.
		return true;
	} else {
		gtk_widget_hide(GTK_WIDGET(window));
		return false;
	}
}

void DictManageDlg::Close()
{
	if (window) {
		gtk_widget_destroy (window);
		window = NULL;
	}
}
