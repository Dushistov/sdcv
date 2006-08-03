#ifndef __SD_SELECTION_H__
#define __SD_SELECTION_H__

#include <gtk/gtk.h>
#include <string>

const int SELECTION_INTERVAL=300; 		    // check selection interval.

class Selection {
private:
    gint IsBusy;
	gint timeout;

	void create_selection_widget();

	static gint TimeOutCallback(gpointer data);
	static void SelectionReceivedCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, Selection *oSelection);
	void SelectionReceived(gchar* sValue);
	gboolean Enable();
public:
	GdkAtom UTF8_STRING_Atom, COMPOUND_TEXT_Atom; //need to set it to static? this make oTextWin can't use it conveniently.
	std::string LastClipWord;
	GtkWidget* selection_widget;

	Selection();
	void Init();
	void End();
	void start();
	void stop();
};

#endif
