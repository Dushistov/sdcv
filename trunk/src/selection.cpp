#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <glib/gi18n.h>

#include "stardict.h"
#include "conf.h"
#include "iskeyspressed.hpp"

#include "selection.h"


// Notice: once you changed this file, try to change src/win32/clipboard.cpp too.


// Is there any way to know that selection is changed? so we don't need to get selection text every interval miliseconds
// and compare the text to know whether selection changed.
// i tried the "selection_notify_event",but it means that some one is requesting the selection.
// if you use the "selection-clear-event",so when selection is changed,you will get this event,
// but it need you do a gtk_selection_owner_set,this make the old selection be cleared :(

// ok,there is no way the know the selection is changed...is there any way to only get the selection's text with a limit length,but
// not the whole text? stardict only need the first 256 chars....it seems no way too :(

/*
GdkAtom Selection::TARGETS_Atom;
GdkAtom Selection::UTF8_STRING_Atom;
*/

Selection::Selection()
{
	UTF8_STRING_Atom = COMPOUND_TEXT_Atom = GDK_NONE;
	IsBusy = 0;
	selection_widget = NULL;
	timeout = 0;
}

/********************************************************************/
gboolean Selection::Enable()
{
  return !gpAppFrame->oTopWin.TextSelected();
}

void Selection::create_selection_widget()
{
	if (selection_widget)
		gtk_widget_destroy(selection_widget);
	selection_widget = gtk_invisible_new ();
	g_signal_connect (G_OBJECT (selection_widget), "selection_received", G_CALLBACK (SelectionReceivedCallback), this);
}


void Selection::Init()
{
	UTF8_STRING_Atom = gdk_atom_intern("UTF8_STRING",false);
	COMPOUND_TEXT_Atom = gdk_atom_intern("COMPOUND_TEXT",false);

	create_selection_widget();
}

void Selection::End()
{
	stop();
	if (selection_widget)
		gtk_widget_destroy(selection_widget);
}

void Selection::start()
{
	if (!timeout)
		timeout = g_timeout_add(SELECTION_INTERVAL,TimeOutCallback,this);
}

void Selection::stop()
{
	if (timeout) {
		g_source_remove(timeout);
		timeout = 0;
	}
	LastClipWord.clear();
}

gint Selection::TimeOutCallback(gpointer data)
{
  Selection *oSelection = (Selection *)data;
  if (oSelection->Enable()) {

		if (conf->get_bool("/apps/stardict/preferences/dictionary/only_scan_while_modifier_key")) {
      bool do_scan = gpAppFrame->unlock_keys->is_pressed();
      if (!do_scan)
				return true;
    }

    if (oSelection->IsBusy) {
      oSelection->IsBusy++;
      if (oSelection->IsBusy*SELECTION_INTERVAL > 8000 ) {
				//"selection_received" signal is not received for 8 seconds.
				//there encounter some error,and i find that this often take a long time (serveral minutes) to get the signal at last,
				//during this perioed,if you call gtk_selection_convert(),the "selection_received" signal will not be received also,
				//and at last these signals are received at almost the same time...BAD.
				
				//so here create a new selection_widget, then call gtk_selection_convert()...this should can throw that error selection.
				//!!!:
				//But this seems(i am not sure) will make the widgets in StarDict become unselectable! see BUGS.
				g_warning("Error, selection data didn't received, retring!\n");
				oSelection->create_selection_widget();
				oSelection->IsBusy = 0;
      }
    } else {
      oSelection->IsBusy = 1;
      gtk_selection_convert (oSelection->selection_widget, GDK_SELECTION_PRIMARY, oSelection->UTF8_STRING_Atom, GDK_CURRENT_TIME);
    }
  }

  return true;
}

void Selection::SelectionReceivedCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, Selection *oSelection)
{	
	gchar *result;
	result = (gchar *)gtk_selection_data_get_text (selection_data);
	if (!result) {
		/* If we asked for UTF8 and didn't get it, try compound_text;
		* if we asked for compound_text and didn't get it, try string;
		* If we asked for anything else and didn't get it, give up.
		*/
		if (selection_data->target == oSelection->UTF8_STRING_Atom) {
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, oSelection->COMPOUND_TEXT_Atom, GDK_CURRENT_TIME);
		}
		else if (selection_data->target == oSelection->COMPOUND_TEXT_Atom)
		{
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, GDK_TARGET_STRING, GDK_CURRENT_TIME);
		}
		else {
			oSelection->IsBusy = 0;
			oSelection->LastClipWord.clear();
		}
		return;
    }
	oSelection->IsBusy = 0;

	oSelection->SelectionReceived(result);	
	g_free (result);
}

/********************************************************************/
void Selection::SelectionReceived(gchar* sToken)
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
