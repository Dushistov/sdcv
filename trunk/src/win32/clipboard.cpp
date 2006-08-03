#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "../stardict.h"
#include <glib/gi18n.h>

#include "clipboard.h"


// Notice: once you changed this file, try to change src/selection.cpp too.

Clipboard::Clipboard()
{
	IsBusy = 0;
	timeout = 0;
}

void Clipboard::Init()
{
	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
}

void Clipboard::End()
{
	stop();
}

void Clipboard::start()
{
	if (!timeout)
		timeout = g_timeout_add(CLIPBOARD_INTERVAL,TimeOutCallback,this);
}

void Clipboard::stop()
{
        if (timeout) {
                g_source_remove(timeout);
                timeout = 0;
        }
	LastClipWord.clear();
}

gint Clipboard::TimeOutCallback(gpointer data)
{
	Clipboard *oClipboard = (Clipboard *)data;
	if (oClipboard->IsBusy ) {
		oClipboard->IsBusy++;
		if (oClipboard->IsBusy*CLIPBOARD_INTERVAL > 8000 ) {
			oClipboard->IsBusy = 0;
		}
	} else {
		oClipboard->IsBusy = 1;
		gtk_clipboard_request_text(oClipboard->clipboard, ClipboardReceivedCallback, oClipboard);
	}
	return true;
}

void Clipboard::ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
	Clipboard *oClipboard = (Clipboard *)data;
	oClipboard->IsBusy = 0;
	if (text) {
		gchar *a = g_strdup(text);
		oClipboard->ClipboardReceived(a);
		g_free(a);
	}
	else {
		oClipboard->IsBusy = 0;
		oClipboard->LastClipWord.clear();
	}
}

void Clipboard::ClipboardReceived(gchar* sToken)
{
    if(sToken[0] == '\0') {
        LastClipWord.clear();
        return;
    }

	gint len = 0;
	gchar *a = sToken;
	while ((*a) && len < 256) {
		a = g_utf8_next_char (a);
		++len;
	}
	*a = '\0';
	
    while (g_ascii_isspace(*sToken))
		sToken++;
	if(sToken[0] == '\0') {
		LastClipWord.clear();
        return;
    }

	a=strchr(sToken,'\n');
	if (a)
		*a='\0';
	
    if (g_str_has_prefix(sToken,"http://") || g_str_has_prefix(sToken,"ftp://")) {
        LastClipWord.clear();
        return;
    }
    
    if (LastClipWord != sToken)     // not equal
    {
		LastClipWord = sToken;
        if(bIsPureEnglish(sToken))
        {
			//if ( g_str_has_suffix(sToken,".pdf") || g_str_has_suffix(sToken,".txt") || g_str_has_suffix(sToken,".cpp") )
				//return;
			if ( gpAppFrame->SimpleLookupToFloat(sToken,false) )  //found
				return;
			a = GetPureEnglishAlpha(sToken);
			if (*a) {
				if (LastClipWord == a) {
					gpAppFrame->ShowNotFoundToFloatWin(a, _("<Not Found!>"), false);
        			gpAppFrame->oTopWin.InsertHisList(a); //really need?
				}
				else
					gpAppFrame->SimpleLookupToFloat(a,true);
			}
			// else the string is too strange, don't show any thing.
        }
		else
		{
			gpAppFrame->SimpleLookupToFloat(sToken,true);
		}
    }
}
