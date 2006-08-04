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

#include <cstring>
#include <string>
#include <glib/gi18n.h>

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#endif

#include "stardict.h"
#include "conf.h"
#include "utils.h"
#include "iskeyspressed.hpp"

#include "floatwin.h"

FloatWin::FloatWin()
{
	timeout = 0;
	now_window_width = 0;
	now_window_height = 0;
	button_box_once_shown = false;
	ismoving = false;
	menu = NULL;
}

void FloatWin::End()
{
	if (timeout)
		g_source_remove(timeout);
	if (menu)
		gtk_widget_destroy(menu);
	if (FloatWindow)
		gtk_widget_destroy(FloatWindow);
}

void FloatWin::Create()
{	
	FloatWindow = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_events(FloatWindow, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK);
	g_signal_connect (G_OBJECT (FloatWindow), "button_press_event", G_CALLBACK (vButtonPressCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "button_release_event", G_CALLBACK (vButtonReleaseCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "motion_notify_event", G_CALLBACK (vMotionNotifyCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "enter_notify_event", G_CALLBACK (vEnterNotifyCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "leave_notify_event", G_CALLBACK (vLeaveNotifyCallback), this);

	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(FloatWindow));
	gint screen_width = gdk_screen_get_width(screen);
	gint screen_height = gdk_screen_get_height(screen);
	int max_window_width=
		conf->get_int_at("floating_window/max_window_width");
	int max_window_height=
		conf->get_int_at("floating_window/max_window_height");
	if (max_window_width < MIN_MAX_FLOATWIN_WIDTH || 
			max_window_width > screen_width)
		conf->set_int_at("floating_window/max_window_width",
									DEFAULT_MAX_FLOATWIN_WIDTH);
	if (max_window_height < MIN_MAX_FLOATWIN_HEIGHT ||
			max_window_height > screen_height)
		conf->set_int_at("floating_window/max_window_height", DEFAULT_MAX_FLOATWIN_HEIGHT);
	
	int lock_x=
		conf->get_int_at("floating_window/lock_x");
	int lock_y=
		conf->get_int_at("floating_window/lock_y");
	
	max_window_width=
		conf->get_int_at("floating_window/max_window_width");
	max_window_height=
		conf->get_int_at("floating_window/max_window_height");
	if (lock_x<0)
		lock_x=0;
	else if (lock_x > (screen_width - max_window_width))
		lock_x = screen_width - max_window_width;
	if (lock_y<0)
		lock_y=0;
	else if (lock_y > (screen_height - max_window_height))
		lock_y = screen_height - max_window_height;
	gtk_window_move(GTK_WINDOW(FloatWindow),lock_x,lock_y);
	
	GtkWidget *frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_OUT);
	gtk_container_add(GTK_CONTAINER(FloatWindow),frame);
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), FLOATWIN_BORDER_WIDTH);
	gtk_container_add(GTK_CONTAINER(frame),vbox);
	
	button_hbox = gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(vbox),button_hbox,false,false,0);
	
	GtkWidget *button;
	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_FIND,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_query_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Query in the main window"),NULL);

	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_SAVE,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", 
			 G_CALLBACK(on_save_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Save to file"),NULL);

	PronounceWordButton= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(PronounceWordButton),gtk_image_new_from_stock(GTK_STOCK_EXECUTE,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (PronounceWordButton), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(PronounceWordButton),"clicked", G_CALLBACK(on_play_click), this);
	g_signal_connect(G_OBJECT(PronounceWordButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),PronounceWordButton,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,PronounceWordButton,_("Pronounce the word"),NULL);
	
	gtk_widget_set_sensitive(PronounceWordButton, false);

	StopButton= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(StopButton),gtk_image_new_from_stock(GTK_STOCK_STOP,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (StopButton), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(StopButton),"clicked", G_CALLBACK(on_stop_click), this);
	g_signal_connect(G_OBJECT(StopButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),StopButton,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips, StopButton, _("Stop selection-scanning"),NULL);

	gtk_widget_set_sensitive(gpAppFrame->oFloatWin.StopButton, 
													 conf->get_bool_at("dictionary/scan_selection"));

#ifndef CONFIG_GPE
	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_HELP,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_help_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Help"),NULL);

	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_QUIT,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_quit_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Quit"),NULL);
#endif

	button = gtk_button_new();

	if (conf->get_bool_at("floating_window/lock"))
		lock_image= gtk_image_new_from_stock(GTK_STOCK_GOTO_LAST,GTK_ICON_SIZE_MENU);
	else
		lock_image= gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(button),lock_image);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);	
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(vLockCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(button_hbox),button,false,false,0);
	gtk_tooltips_set_tip(gpAppFrame->tooltips,button,_("Lock floating window"),NULL);
	view.reset(new ArticleView(GTK_BOX(vbox), true));
		
	gtk_widget_show_all(frame);
	gtk_widget_hide(button_hbox); //show all will show hbox's children,now hide hbox only.	
}

void FloatWin::ShowText(gchar ***Word, gchar ****WordData, const gchar * sOriginWord)
{
	QueryingWord = sOriginWord;
	found_result = FLOAT_WIN_FOUND;
	view->BeginUpdate();
	view->Clear();
	std::string mark = "<b><span size=\"x-large\">";
	gchar *m_str = g_markup_escape_text(sOriginWord, -1);
	mark += m_str;
	g_free(m_str);
	mark += "</span></b>";
	view->AppendPangoText(mark.c_str());
	int j,k;
	for (std::size_t i=0; i<gpAppFrame->oLibs.ndicts(); i++) {
		if (Word[i]) {
			view->AppendNewline();
			view->AppendHeader(gpAppFrame->oLibs.dict_name(i), i);
			j=0;
			do {
				if (j==0) {
					if (strcmp(Word[i][j], sOriginWord))
						view->AppendWord(Word[i][j]);
				} else {
					view->AppendNewline();
					view->AppendWord(Word[i][j]);
				}
				view->AppendData(WordData[i][j][0], Word[i][j]);
				k=1;
				while (WordData[i][j][k]) {
					view->AppendNewline();
					view->AppendDataSeparate();
					view->AppendData(WordData[i][j][k], Word[i][j]);
					k++;
				}
				j++;
			} while (Word[i][j]);
		}
	}
	view->EndUpdate();
	gboolean pronounced = false;
	gboolean canRead = gpAppFrame->oReadWord.canRead(sOriginWord);
	if (canRead) {
		if (PronounceWord == sOriginWord)
			pronounced = true;
		else
			PronounceWord = sOriginWord;
	} else {
		for (std::size_t i=0;i< gpAppFrame->oLibs.ndicts(); i++) {
			if (Word[i] && strcmp(Word[i][0], sOriginWord)) {
				if (gpAppFrame->oReadWord.canRead(Word[i][0])) {
					canRead = TRUE;
					if (PronounceWord == Word[i][0])
						pronounced = true;
					else
						PronounceWord = Word[i][0];
				}
				break;
			}
		}
	}
	gtk_widget_set_sensitive(PronounceWordButton, canRead);

	Popup(true);
	if (canRead && (!pronounced) && conf->get_bool_at("floating_window/pronounce_when_popup"))
		gpAppFrame->oReadWord.read(PronounceWord.c_str());
}

void FloatWin::ShowText(gchar ****ppppWord, gchar *****pppppWordData, const gchar ** ppOriginWord, gint count, const gchar *sOriginWord)
{
  QueryingWord = sOriginWord;
  found_result = FLOAT_WIN_FUZZY_FOUND;

  view->BeginUpdate();
  view->Clear();
  view->GotoBegin();

  std::string mark;
  gchar *m_str;
  mark = _("Fuzzy query");
  mark += " <b><span size=\"x-large\">";
  m_str = g_markup_escape_text(sOriginWord,-1);
  mark += m_str;
  g_free(m_str);
  mark += "</span></b> ";
  mark += _("has succeeded.\n");
  mark += _("Found ");
  if (count ==1)
    mark+= _("1 word:\n");
  else {
    m_str=g_strdup_printf("%d",count);
    mark += m_str;
    g_free(m_str);
    mark+= _(" words:\n");
  }
  
  int j;
  for (j=0; j<count-1; j++) {
    mark += "<span size=\"x-large\">";
    m_str = g_markup_escape_text(ppOriginWord[j], -1);
    mark += m_str;
    g_free(m_str);
    mark += "</span> ";
  }
  mark += "<span size=\"x-large\">";
  m_str = g_markup_escape_text(ppOriginWord[count-1],-1);
  mark += m_str;
  g_free(m_str);
  mark += "</span>";
  view->AppendPangoText(mark.c_str());

  int m,n;
  for (j=0; j<count; j++) {
    if (!ppppWord[j])
	continue;
    mark = "\n\n<b><span size=\"x-large\">";
    m_str = g_markup_escape_text(ppOriginWord[j],-1);
    mark += m_str;
    g_free(m_str);
    mark += "</span></b>";
    view->AppendPangoText(mark.c_str());
	for (std::size_t i=0; i<gpAppFrame->oLibs.ndicts(); i++) {
	if (ppppWord[j][i]) {
		view->AppendNewline();
		view->AppendHeader(gpAppFrame->oLibs.dict_name(i), i);
		m=0;
		do {
			if (m==0) {
				if (strcmp(ppppWord[j][i][m], ppOriginWord[j]))
					view->AppendWord(ppppWord[j][i][m]);
			} else {
				view->AppendNewline();
				view->AppendWord(ppppWord[j][i][m]);
			}
			view->AppendData(pppppWordData[j][i][m][0], ppppWord[j][i][m]);
			n=1;
			while (pppppWordData[j][i][m][n]) {
				view->AppendNewline();
				view->AppendDataSeparate();
				view->AppendData(pppppWordData[j][i][m][n], ppppWord[j][i][m]);
				n++;
			}
			m++;
		} while (ppppWord[j][i][m]);
	}
    }
  }	
  view->EndUpdate();

  gboolean canRead = gpAppFrame->oReadWord.canRead(sOriginWord);
  if (canRead)
    PronounceWord = sOriginWord;
  gtk_widget_set_sensitive(PronounceWordButton, canRead);

 
  Popup(false);
  /*bool pronounce_when_popup=
		conf->get_bool_at("floating_window/pronounce_when_popup");

  if (canRead && pronounce_when_popup)
    gpAppFrame->oReadWord.read(PronounceWord.c_str());*/
}

void FloatWin::ShowNotFound(const char* sWord,const char* sReason, gboolean fuzzy)
{
	QueryingWord = sWord;
	if (fuzzy)
		found_result = FLOAT_WIN_FUZZY_NOT_FOUND;
	else
		found_result = FLOAT_WIN_NOT_FOUND;

	if (!conf->get_bool_at("floating_window/show_if_not_found"))
		return;

	gchar *text;
	gchar *m_word,*m_reason;
	m_word = g_markup_escape_text(sWord,-1);
	m_reason = g_markup_escape_text(sReason,-1);
	text = g_strdup_printf("<b><big>%s</big></b>\n<span foreground=\"blue\">%s</span>",m_word,m_reason);
	view->SetPangoText(text);
	
	gboolean pronounced = false;
	gboolean canRead = gpAppFrame->oReadWord.canRead(sWord);
	if (canRead) {
		if (PronounceWord == sWord)
			pronounced = true;
		else
			PronounceWord = sWord;
	}
	gtk_widget_set_sensitive(PronounceWordButton, canRead);

	if (fuzzy)
		Popup(false);
	else
		Popup(true);
	g_free(text);
	g_free(m_word);
	g_free(m_reason);

	if (canRead && (!pronounced) && conf->get_bool_at("floating_window/pronounce_when_popup"))
		gpAppFrame->oReadWord.read(PronounceWord.c_str());
}

void FloatWin::Popup(gboolean updatePosition)
{
  ismoving = true;

  GtkRequisition requisition;
  gtk_widget_size_request(view->Widget(), &requisition);
	int max_window_width=conf->get_int_at("floating_window/max_window_width");
  if (requisition.width > max_window_width) {
		// it is not really max window width setting.
    gtk_widget_set_size_request(view->Widget(), max_window_width, -1);
    gtk_label_set_line_wrap(GTK_LABEL(view->Widget()), true);
    gtk_widget_size_request(view->Widget(), &requisition); //update requisition
  }
  gint window_width,window_height;
  window_width = 2*(FLOATWIN_BORDER_WIDTH+2) + requisition.width; // 2 is the frame 's width.or get it by gtk function? i am lazy,hoho
	int max_window_height=
		conf->get_int_at("floating_window/max_window_height");
  if (requisition.height > max_window_height) {
    static gint vscrollbar_width = 0;
    if (!vscrollbar_width) {
      if (view->VScrollBar()) {
				GtkRequisition vscrollbar_requisition;
				gtk_widget_size_request(view->VScrollBar(), &vscrollbar_requisition);
				vscrollbar_width = vscrollbar_requisition.width;
				vscrollbar_width += view->ScrollSpace();
      }
    }
    view->SetSize(requisition.width + vscrollbar_width, max_window_height);
    window_height = 2*(FLOATWIN_BORDER_WIDTH+2) + max_window_height;
    window_width += vscrollbar_width;
  } else {
    view->SetSize(requisition.width, requisition.height);
    window_height = 2*(FLOATWIN_BORDER_WIDTH+2) + requisition.height;
  }
  
  gboolean button_hbox_visible = GTK_WIDGET_VISIBLE(button_hbox);
  if (button_hbox_visible) {
    window_height += (button_hbox->allocation).height;
    if (window_width < (button_hbox->allocation).width + 2*(FLOATWIN_BORDER_WIDTH+2))
      window_width = (button_hbox->allocation).width + 2*(FLOATWIN_BORDER_WIDTH+2);
  }	


	if (conf->get_bool_at("floating_window/lock")) {
    gtk_window_resize(GTK_WINDOW(FloatWindow),window_width,window_height);
    now_window_width = window_width;
    now_window_height = window_height;
    
    // need to make window 's resize relate to other corner?
    Show();
  } else {
    gint iCurrentX,iCurrentY;
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(FloatWindow));
    if ((!GTK_WIDGET_VISIBLE(FloatWindow)) || updatePosition) {
      button_box_once_shown = false;
      GdkDisplay *display = gdk_screen_get_display(screen);
      
      gdk_display_get_pointer(display, NULL, &iCurrentX, &iCurrentY, NULL);
      
      bool pressed = gpAppFrame->unlock_keys->is_pressed();

      if (pressed) {
				popup_pointer_x = iCurrentX;
				popup_pointer_y = iCurrentY;
      }	else {
				// popup by middle click on the notification area icon, 
				//so never hiden the floating window even mouse didn't moved as in FloatWin::vTimeOutCallback().
				popup_pointer_x = -1;
				popup_pointer_y = -1;
      }
      
      iCurrentX += FLOATWIN_OFFSET_X;
      iCurrentY += FLOATWIN_OFFSET_Y;
    } else {
      gtk_window_get_position(GTK_WINDOW(FloatWindow),&iCurrentX,&iCurrentY);
    }
    gint screen_width = gdk_screen_get_width(screen);
    gint screen_height = gdk_screen_get_height(screen);		
    if (iCurrentX + window_width > screen_width)
      iCurrentX = screen_width - window_width;
    if (iCurrentY + window_height > screen_height)
      iCurrentY = screen_height - window_height;		
    // don't use gdk_window_resize,it make the window can't be smaller latter!
    /*if (FloatWindow->window) {
      gdk_window_move_resize(FloatWindow->window, iCurrentX, iCurrentY, window_width, window_height);			
      }
      else {
      gtk_window_move(GTK_WINDOW(FloatWindow),iCurrentX,iCurrentY);			
      gtk_window_resize(GTK_WINDOW(FloatWindow),window_width,window_height);
      }*/
    
    //note: must do resize before move should be better,as the vTimeOutCallback() may hide it.
    gtk_window_resize(GTK_WINDOW(FloatWindow),window_width,window_height);
    gtk_window_move(GTK_WINDOW(FloatWindow),iCurrentX,iCurrentY);
    //gtk_decorated_window_move_resize_window(GTK_WINDOW(FloatWindow), iCurrentX, iCurrentY, window_width, window_height);
    now_window_width = window_width;
    now_window_height = window_height;
    
    Show();
  }	
  ismoving = false;	
}

void FloatWin::Show()
{
	gtk_widget_show(FloatWindow);
	if (!timeout)
		timeout = g_timeout_add(FLOAT_TIMEOUT,vTimeOutCallback,this);
}

void FloatWin::Hide()
{
	if (timeout) {
		g_source_remove(timeout);
		timeout = 0;
	}
	button_box_once_shown = false;
	PronounceWord.clear();
	gtk_widget_hide(FloatWindow);
}

gint FloatWin::vTimeOutCallback(gpointer data)
{
  FloatWin *oFloatWin = static_cast<FloatWin *>(data);
	bool lock=
		conf->get_bool_at("floating_window/lock");
  if(!lock && !oFloatWin->ismoving && 
     GTK_WIDGET_VISIBLE(oFloatWin->FloatWindow)) {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(oFloatWin->FloatWindow));
    GdkDisplay *display = gdk_screen_get_display(screen);

    gint iCurrentX,iCurrentY;
    gdk_display_get_pointer(display, NULL, &iCurrentX, &iCurrentY, NULL);
		bool only_scan_while_modifier_key=
			conf->get_bool_at("dictionary/only_scan_while_modifier_key");
		bool hide_floatwin_when_modifier_key_released=
			conf->get_bool_at("dictionary/hide_floatwin_when_modifier_key_released");
    if (only_scan_while_modifier_key && 
				hide_floatwin_when_modifier_key_released) {
      if (iCurrentX == oFloatWin->popup_pointer_x && iCurrentY==oFloatWin->popup_pointer_y) {
				bool released = !gpAppFrame->unlock_keys->is_pressed();

				if (released) {
					oFloatWin->Hide();
					gpAppFrame->oSelection.LastClipWord.clear(); //so press modifier key again will pop up the floatwin.
					return true;
				}
      }
    }
		
    int distance;
    gint window_x,window_y,window_width,window_height;
    gtk_window_get_position(GTK_WINDOW(oFloatWin->FloatWindow),&window_x,&window_y);
    //notice: gtk_window_get_size() is not really uptodate,don't use it! see "gtk reference".
    //gtk_window_get_size(GTK_WINDOW(oFloatWin->FloatWindow),&window_width,&window_height);
    window_width = oFloatWin->now_window_width;
    window_height = oFloatWin->now_window_height;
    
    if (iCurrentX < window_x) {
	distance = (iCurrentX-window_x)*(iCurrentX-window_x);
	if (iCurrentY < window_y)
	  distance += (iCurrentY-window_y)*(iCurrentY-window_y);
	else if (iCurrentY > window_y+window_height)
	  distance += (iCurrentY-window_y-window_height)*(iCurrentY-window_y-window_height);
    } else if (iCurrentX > window_x+window_width) {
      distance = (iCurrentX-window_x-window_width)*(iCurrentX-window_x-window_width);
      if (iCurrentY < window_y)
	distance += (iCurrentY-window_y)*(iCurrentY-window_y);
      else if ( iCurrentY > window_y+window_height )
	distance += (iCurrentY-window_y-window_height)*(iCurrentY-window_y-window_height);
    } else {
      if (iCurrentY < window_y)
	distance = (window_y - iCurrentY)*(window_y - iCurrentY);
      else if (iCurrentY > window_y+window_height)
	distance = (iCurrentY-window_y-window_height)*(iCurrentY-window_y-window_height);
      else
	distance = 0; //in the floating window.
    }
    if (distance > DISAPPEAR_DISTANCE){
      oFloatWin->Hide();
    }
  } // to be hidden
  
  
  return true;
}

gboolean FloatWin::vEnterNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin)
{	
	/*g_print("enter ");
	switch (event->detail)
	{
		case GDK_NOTIFY_ANCESTOR:
			g_print("GDK_NOTIFY_ANCESTOR\n");
			break;
		case GDK_NOTIFY_VIRTUAL:
			g_print("GDK_NOTIFY_VIRTUAL\n");
			break;
		case GDK_NOTIFY_NONLINEAR:
			g_print("GDK_NOTIFY_NONLINEAR\n");
			break;
		case GDK_NOTIFY_NONLINEAR_VIRTUAL:
			g_print("GDK_NOTIFY_NONLINEAR_VIRTUAL\n");
			break;		
		case GDK_NOTIFY_UNKNOWN:
			g_print("GDK_NOTIFY_UNKNOWN\n");
			break;
		case GDK_NOTIFY_INFERIOR:
			g_print("GDK_NOTIFY_INFERIOR\n");
			break;
	}*/

#ifdef _WIN32
	if ((event->detail==GDK_NOTIFY_ANCESTOR) || (event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL)) {
#else
	if ((event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL)) {
#endif
		if (!GTK_WIDGET_VISIBLE(oFloatWin->button_hbox)) {
			gtk_widget_show(oFloatWin->button_hbox);
		
			if (!oFloatWin->button_box_once_shown) {
				oFloatWin->button_box_once_shown = true;

				gint iCurrentX,iCurrentY;
				gtk_window_get_position(GTK_WINDOW(oFloatWin->FloatWindow),&iCurrentX,&iCurrentY);
				GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(oFloatWin->FloatWindow));
				gint screen_width = gdk_screen_get_width(screen);
				gint screen_height = gdk_screen_get_height(screen);					
				GtkRequisition requisition;
				gtk_widget_size_request(oFloatWin->button_hbox,&requisition);
				oFloatWin->now_window_height += requisition.height;
				requisition.width += 2*(FLOATWIN_BORDER_WIDTH+2);
				if (requisition.width > oFloatWin->now_window_width)
					oFloatWin->now_window_width = requisition.width;
				gboolean changed=false;		
				if (iCurrentX < 0) {
					iCurrentX = 0;
					changed = true;
				}
				else {
					if (iCurrentX + oFloatWin->now_window_width > screen_width) {
						iCurrentX = screen_width - oFloatWin->now_window_width;
						changed = true;
					}
				}
				if (iCurrentY < 0) {
					iCurrentY = 0;
					changed = true;
				}
				else {
					if (iCurrentY + oFloatWin->now_window_height > screen_height) {
						iCurrentY = screen_height - oFloatWin->now_window_height;	
						changed = true;			
					}			
				}
				if (changed)
					gtk_window_move(GTK_WINDOW(oFloatWin->FloatWindow),iCurrentX,iCurrentY);				
			}
		}
	}
	return true;
}

gboolean FloatWin::vLeaveNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin)
{
	/*g_print("leave ");
	switch (event->detail)
	{
		case GDK_NOTIFY_ANCESTOR:
			g_print("GDK_NOTIFY_ANCESTOR\n");
			break;
		case GDK_NOTIFY_VIRTUAL:
			g_print("GDK_NOTIFY_VIRTUAL\n");
			break;
		case GDK_NOTIFY_NONLINEAR:
			g_print("GDK_NOTIFY_NONLINEAR\n");
			break;
		case GDK_NOTIFY_NONLINEAR_VIRTUAL:
			g_print("GDK_NOTIFY_NONLINEAR_VIRTUAL\n");
			break;		
		case GDK_NOTIFY_UNKNOWN:
			g_print("GDK_NOTIFY_UNKNOWN\n");
			break;
		case GDK_NOTIFY_INFERIOR:
			g_print("GDK_NOTIFY_INFERIOR\n");
			break;
	}*/
#ifdef _WIN32
	if (((event->detail==GDK_NOTIFY_ANCESTOR) || (event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL))&&(!oFloatWin->ismoving)) {
#else
	if (((event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL))&&(!oFloatWin->ismoving)) {
#endif
		gtk_widget_hide(oFloatWin->button_hbox);
	}
	return true;
}

void FloatWin::on_menu_copy_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, oFloatWin->view->GetText().c_str(), -1);
}

void FloatWin::on_menu_save_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	oFloatWin->on_save_click(widget, oFloatWin);
}

void FloatWin::on_menu_query_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	oFloatWin->on_query_click(widget, oFloatWin);
}

void FloatWin::on_menu_play_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	oFloatWin->on_play_click(widget, oFloatWin);
}

void FloatWin::on_menu_fuzzyquery_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	gpAppFrame->LookupWithFuzzyToFloatWin(oFloatWin->QueryingWord.c_str());
}

gboolean FloatWin::vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin)
{
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == 1) {
			gtk_window_get_position(GTK_WINDOW(widget),&(oFloatWin->press_window_x),&(oFloatWin->press_window_y));
			oFloatWin->press_x_root = (gint)(event->x_root);
			oFloatWin->press_y_root = (gint)(event->y_root);
			oFloatWin->ismoving = true;
		}
		else if (event->button == 3) {			
			if (oFloatWin->menu)
				gtk_widget_destroy(oFloatWin->menu);
			
			oFloatWin->menu = gtk_menu_new();

			GtkWidget *menuitem;
			menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Copy"));
			GtkWidget *image;
			image = gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
			g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_copy_activate), oFloatWin);
			gtk_menu_shell_append(GTK_MENU_SHELL(oFloatWin->menu), menuitem);
			menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Save"));
			image = gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
			 g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_save_activate), oFloatWin);
			gtk_menu_shell_append(GTK_MENU_SHELL(oFloatWin->menu), menuitem);
			menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Query"));
			image = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
			g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_query_activate), oFloatWin);
			gtk_menu_shell_append(GTK_MENU_SHELL(oFloatWin->menu), menuitem);

			if (GTK_WIDGET_SENSITIVE(oFloatWin->PronounceWordButton)) {
				menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Play"));
				image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
				g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_play_activate), oFloatWin);
				gtk_menu_shell_append(GTK_MENU_SHELL(oFloatWin->menu), menuitem);
			}

			if ((oFloatWin->found_result != FLOAT_WIN_FUZZY_FOUND) && (oFloatWin->found_result != FLOAT_WIN_FUZZY_NOT_FOUND)) {
				menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Fuzzy Query"));
				image = gtk_image_new_from_stock(GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_MENU);
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
				g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_fuzzyquery_activate), oFloatWin);
				gtk_menu_shell_append(GTK_MENU_SHELL(oFloatWin->menu), menuitem);
			}			
			
			gtk_widget_show_all(oFloatWin->menu);
			gtk_menu_popup(GTK_MENU(oFloatWin->menu), NULL, NULL, NULL, NULL, event->button, event->time);
			play_sound_on_event("menushow");			
		}			
	}	else if (event->type == GDK_2BUTTON_PRESS) {
		if (oFloatWin->found_result == FLOAT_WIN_NOT_FOUND)
			gpAppFrame->LookupWithFuzzyToFloatWin(oFloatWin->QueryingWord.c_str());
		else
			oFloatWin->Hide();		
	}

	return true;
}

gboolean FloatWin::vButtonReleaseCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin)
{
	if (event->button == 1) {
		oFloatWin->ismoving = false;
	}
	else if (event->button==3) {
			//gpAppFrame->oAppCore->Popup();
	}

	return true;
}

gboolean FloatWin::vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , FloatWin *oFloatWin)
{
	if (event->window == oFloatWin->FloatWindow->window || (event->state & GDK_BUTTON1_MASK))
	{
		gint x,y;
		x = oFloatWin->press_window_x + (gint)(event->x_root) - oFloatWin->press_x_root;
		y = oFloatWin->press_window_y + (gint)(event->y_root) - oFloatWin->press_y_root;
		if (x<0)
			x = 0;
		if (y<0)
			y = 0;
		gtk_window_move(GTK_WINDOW(oFloatWin->FloatWindow), x, y);
	}		

	return true;
}

void FloatWin::on_query_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");

	if (!conf->get_bool_at("floating_window/lock"))
		oFloatWin->Hide();
	gpAppFrame->Query(oFloatWin->QueryingWord.c_str());	

	if (!GTK_WIDGET_VISIBLE(gpAppFrame->window))
		gpAppFrame->tray_icon_->maximize_from_tray();		

	gtk_window_present(GTK_WINDOW(gpAppFrame->window));
}

void FloatWin::on_save_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	if (conf->get_bool_at("dictionary/only_export_word")) {
		FILE *fp = fopen(conf->get_string_at("dictionary/export_file").c_str(), "a+");
		if(fp) {
			fputs(oFloatWin->QueryingWord.c_str(),fp);
			fputs("\n",fp);
			fclose(fp);
		}
	} else {
		FILE *fp = fopen(conf->get_string_at("dictionary/export_file").c_str(), "a+");
		if(fp) {
			fputs(oFloatWin->view->GetText().c_str(),fp);
			fputs("\n\n",fp);
			fclose(fp);
		}
	}
	play_sound_on_event("buttonactive");
}

void FloatWin::on_play_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	gpAppFrame->oReadWord.read(oFloatWin->PronounceWord.c_str());
}

void FloatWin::on_stop_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");
	conf->set_bool_at("dictionary/scan_selection", false);
}

#ifndef CONFIG_GPE
void FloatWin::on_help_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");

	if (!conf->get_bool_at("floating_window/lock"))
		oFloatWin->Hide();
	show_help("stardict-scan-selection");
}

void FloatWin::on_quit_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");
	gpAppFrame->Quit();
}
#endif

void FloatWin::vLockCallback(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");
	conf->set_bool_at("floating_window/lock", 
								 !conf->get_bool_at("floating_window/lock"));
}
