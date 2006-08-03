#ifndef PANGOVIEW_H
#define PAGNOVIEW_H

#include <list>
#include <string>
#include <gtk/gtk.h>

class PangoView {
public:
  PangoView(GtkContainer *owner, bool autoresize_=false);
  PangoView(GtkBox *owner, bool autoresize_=false);
  void AppendText(const char *str);
  void SetText(const char *str);
  void AppendPangoText(const char *str);
  void SetPangoText(const char *str); 
  void AppendMark(const char *mark);

  GtkWidget *Widget(void) {
    if (autoresize)
      return GTK_WIDGET(label);
    else
      return GTK_WIDGET(textview);
  }
  void ScrollTo(gdouble value);
  gdouble ScrollPos();
  void BeginUpdate();
  void EndUpdate();
  void Clear();
  void GotoBegin();
  std::string GetText();
  GtkWidget *VScrollBar() { return scrolled_window->vscrollbar; }
  void SetSize(gint w, gint h)
  {
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), w, h);
  }
  gint ScrollSpace()
  {
    gint scrollbar_spacing;
    gtk_widget_style_get(GTK_WIDGET(scrolled_window), 
			 "scrollbar_spacing", &scrollbar_spacing, NULL);
    return scrollbar_spacing;
  }
private:
	bool update;
	std::string cache;
  bool autoresize;
  GtkScrolledWindow *scrolled_window;
  GtkTextIter iter;
  std::list<GtkTextMark *> marklist;
  union {
    GtkTextView *textview;
    GtkLabel *label;
  };

  void Create(bool autoresize_);
};

#endif//pangoview.h
