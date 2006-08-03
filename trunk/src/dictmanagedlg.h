#ifndef __SD_DICT_MANAGE_DLG_H__
#define __SD_DICT_MANAGE_DLG_H__

#include <gtk/gtk.h>

class DictManageDlg {
private:	
	GtkWidget *wazard_button;
	GtkWidget *notebook;
	GtkWidget *dict_treeview;
	GtkTreeModel *dict_tree_model;
	GtkWidget *treedict_treeview;
	GtkTreeModel *treedict_tree_model;
	GtkWindow *parent_win;
	GdkPixbuf *dicts_icon, *tree_dicts_icon;
	GtkWidget *window;

	static GtkTreeModel* create_dict_tree_model(bool istreedict);
	GtkWidget *create_dict_tree(gboolean istreedict);
		
	GtkWidget *create_buttons();

	void write_order_list(bool istreedict);

	static void on_wazard_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_appendix_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);

	static gboolean on_treeview_button_press(GtkWidget * widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);
	static void response_handler (GtkDialog *dialog, gint res_id, DictManageDlg *oDictManageDlg);
	static void on_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg);
	static void on_treedict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg);

	static void drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg);
	static void drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, DictManageDlg *oDictManageDlg);
public:
	DictManageDlg(GtkWindow *parent_win, GdkPixbuf *dicts_icon, GdkPixbuf *tree_dicts_icon);
	bool Show();
	void Close();
};

#endif
