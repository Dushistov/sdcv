#ifndef __SD_CLIPBOARD_H__
#define __SD_CLIPBOARD_H__

#include <gtk/gtk.h>
#include <string>

const int CLIPBOARD_INTERVAL=300; 		    // check clipboard interval.

class Clipboard
{
private:
    gint IsBusy;
	gint timeout;
	GtkClipboard* clipboard;

	static gint TimeOutCallback(gpointer data);
	static void ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data);
	void ClipboardReceived(gchar* sValue);
public:
	std::string LastClipWord;

	Clipboard();
	void Init();
	void End();
	void start();
	void stop();
	
};

#endif
