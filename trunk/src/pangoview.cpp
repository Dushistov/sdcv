/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2005 Evgeniy <dushistov@mail.ru>
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

#include "gtktextviewpango.h"
#include "utils.h"

#include "pangoview.h"

void PangoView::Create(bool autoresize_)
{
	update=false;
  autoresize=autoresize_;
  if (!autoresize) {
    textview = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_widget_show(GTK_WIDGET(textview));
    gtk_text_view_set_editable(textview, FALSE);
    gtk_text_view_set_cursor_visible(textview, FALSE);
    gtk_text_view_set_wrap_mode(textview, GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(textview, 5);
    gtk_text_view_set_right_margin(textview, 5);  
       
    scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    gtk_widget_show(GTK_WIDGET(scrolled_window));
    gtk_scrolled_window_set_policy(scrolled_window,
				   //altought textview's set_wrap_mode will cause 
				   //this can be GTK_POLICY_NEVER,but...
				   //there are widgets that may make this broken.
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(textview));
    gtk_scrolled_window_set_shadow_type(scrolled_window, GTK_SHADOW_IN);
  } else {
    label = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
    
    scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    gtk_scrolled_window_set_shadow_type(scrolled_window, GTK_SHADOW_NONE);
    gtk_scrolled_window_set_policy(scrolled_window, GTK_POLICY_NEVER, 
				   GTK_POLICY_AUTOMATIC);
  
    GtkWidget *viewport = 
      gtk_viewport_new(gtk_scrolled_window_get_hadjustment(scrolled_window),
		       gtk_scrolled_window_get_vadjustment(scrolled_window));
    gtk_widget_add_events(viewport, GDK_BUTTON1_MOTION_MASK);
    gtk_widget_add_events(viewport, GDK_BUTTON_RELEASE_MASK);	
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), viewport);
    gtk_container_add(GTK_CONTAINER(viewport), GTK_WIDGET(label));
  }
}

PangoView::PangoView(GtkBox *owner, bool autoresize_)
{
  Create(autoresize_);
  gtk_box_pack_start(owner, GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);
}

PangoView::PangoView(GtkContainer *owner, bool autoresize_)
{
  Create(autoresize_);
  gtk_container_add(owner, GTK_WIDGET(scrolled_window));
}

void PangoView::SetText(const char *text)
{  
	if (update) {
		cache=text;
		return;
	}

	if (!autoresize) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);

		std::list<GtkTextMark *>::const_iterator it;
		for (it=marklist.begin(); it!=marklist.end(); ++it) {
			gtk_text_buffer_delete_mark(buffer, *it);
		}
		marklist.clear();

		gtk_text_buffer_set_text(buffer, text, -1);
  } else {
    ScrollTo(0);
    // this should speed up the next two line.
    gtk_label_set_markup(label, ""); 
    // so Popup()'s gtk_widget_size_request(label, &requisition); can
    gtk_widget_set_size_request(GTK_WIDGET(label), -1, -1);
    // get its original width.
    gtk_label_set_line_wrap(label, FALSE); 
    gchar *mstr=g_markup_escape_text(text, -1);
    gtk_label_set_text(label, mstr);
    g_free(mstr);
  }
}

void PangoView::AppendText(const char *str)
{
	if (update) {
		cache+=str;
		return;
	}

  if (!autoresize) {
    GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview);
    gtk_text_buffer_insert(buffer, &iter, str, strlen(str));
  } else
    SetText((std::string(gtk_label_get_text(label))+str).c_str()); 
}

void PangoView::SetPangoText(const char *str)
{  
	if (update) {
		cache=str;
		return;
	}

  if (!autoresize) {
    Clear();
    GotoBegin();
    AppendPangoText(str);
  } else {
    ScrollTo(0);
    // this should speed up the next two line.
    gtk_label_set_markup(label, ""); 
    // so Popup()'s gtk_widget_size_request(label,&requisition); can
    gtk_widget_set_size_request(GTK_WIDGET(label), -1, -1);
    // get its original width.
    gtk_label_set_line_wrap(label, FALSE); 
    gtk_label_set_markup(label, str);
  }
}

void PangoView::AppendPangoText(const char *str)
{
	if (update) {
		cache+=str;
		return;
	}

  if (!autoresize) {
    GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview);
    gtk_text_buffer_insert_markup(buffer, &iter, str);
  } else {
    SetPangoText((std::string(gtk_label_get_label(label))+str).c_str());
  }
}

void PangoView::AppendMark(const char *mark)
{
	if (autoresize)
		return;
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview);
	if (update) {
		if (!cache.empty()) {
			gtk_text_buffer_insert_markup(buffer, &iter, cache.c_str());
			cache.clear();
		}
	}
	marklist.push_back(gtk_text_buffer_create_mark(buffer, mark, &iter, TRUE));
}

void PangoView::ScrollTo(gdouble value)
{
  gtk_adjustment_set_value(
    gtk_scrolled_window_get_vadjustment(scrolled_window), value
  );
}

gdouble PangoView::ScrollPos(void)
{
  return  gtk_adjustment_get_value(
	    gtk_scrolled_window_get_vadjustment(scrolled_window)
	  );
}

void PangoView::BeginUpdate(void) 
{
	update=true;
	cache.clear();
  if (!autoresize)
    gtk_text_buffer_begin_user_action(gtk_text_view_get_buffer(textview));
}

void PangoView::EndUpdate(void)
{
	if (update) {
		update=false;
		if (autoresize)
			SetPangoText(cache.c_str());
		else
			AppendPangoText(cache.c_str());
		cache.clear();
	}

  if (!autoresize)
    gtk_text_buffer_end_user_action(gtk_text_view_get_buffer(textview));
}

void PangoView::Clear(void)
{
	if (!autoresize) {
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);

	std::list<GtkTextMark *>::const_iterator it;
	for (it=marklist.begin(); it!=marklist.end(); ++it) {
		gtk_text_buffer_delete_mark(buffer, *it);
	}
	marklist.clear();

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	gtk_text_buffer_delete(buffer, &start, &end);
	ScrollTo(0);
	} else 
		SetText("");
}

void PangoView::GotoBegin(void) 
{
  if (!autoresize)
    gtk_text_buffer_get_iter_at_offset(
      gtk_text_view_get_buffer(textview), &iter, 0
    ); 
}

std::string PangoView::GetText(void)
{
  std::string res;
  if (!autoresize) {
    GtkTextIter start, end;
    GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview);
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    res=text;
    g_free(text);
  } else {
    res=gtk_label_get_text(label);
  }

  return res;
}
