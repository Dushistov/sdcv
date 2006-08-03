#ifndef ARTICLEVIEW_H
#define ARTICLEVIEW_H

#include <string>
#include <gtk/gtk.h>

#include "pangoview.h" 

//class which show dictionary's aritcles
class ArticleView : public PangoView {
public:
  ArticleView(GtkContainer *owner, bool floatw=false)
    : PangoView(owner, floatw), for_float_win(floatw) {}
  ArticleView(GtkBox *owner, bool floatw=false)
    : PangoView(owner, floatw), for_float_win(floatw) {}

	void AppendHeader(const std::string& dict_name, int i);
	void AppendWord(const gchar *word);
	void AppendData(gchar *data, const gchar *oword);
	void AppendNewline();
	void AppendDataSeparate();

private:
  bool for_float_win;
};


#endif//ARTICLEVIEW_H
