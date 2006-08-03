#ifndef __SD_FLOATWIN_H__
#define __SD_FLOATWIN_H__

#include <gtk/gtk.h>
#include <string>
#include "articleview.h"

const int FLOAT_TIMEOUT=300;		        // interval used by floating window.
const int DISAPPEAR_DISTANCE=400; // it is the square of the distance.
const int FLOATWIN_BORDER_WIDTH=4;
const int FLOATWIN_OFFSET_X=8;
const int FLOATWIN_OFFSET_Y=2; //when the window is too right,the x will change,so make offset_y not equal to 0 will make button box not show all the same.

enum FloatWinQueryResult
{
	FLOAT_WIN_FOUND,
	FLOAT_WIN_FUZZY_FOUND,
	FLOAT_WIN_NOT_FOUND,
	FLOAT_WIN_FUZZY_NOT_FOUND
};

class FloatWin {
private:
  gint press_x_root,press_y_root,press_window_x,press_window_y;
  gint popup_pointer_x, popup_pointer_y;
  gint timeout;
  gint now_window_width,now_window_height;
  gboolean button_box_once_shown;
  gboolean ismoving;
  
  static gint vTimeOutCallback(gpointer data);	
  static void on_query_click(GtkWidget *widget, FloatWin *oFloatWin);	
  static void on_save_click(GtkWidget *widget, FloatWin *oFloatWin);
  static void on_play_click(GtkWidget *widget, FloatWin *oFloatWin);
  static void on_stop_click(GtkWidget *widget, FloatWin *oFloatWin);
  static void on_help_click(GtkWidget *widget, FloatWin *oFloatWin);
  static void on_quit_click(GtkWidget *widget, FloatWin *oFloatWin);
  static void vLockCallback(GtkWidget *widget, FloatWin *oFloatWin);
  static gboolean vEnterNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin);
  static gboolean vLeaveNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin);
  static gboolean vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin);
  static gboolean vButtonReleaseCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin);
  static gboolean vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , FloatWin *oFloatWin);
  static void on_menu_copy_activate(GtkWidget * widget, FloatWin *oFloatWin);
  static void on_menu_save_activate(GtkWidget * widget, FloatWin *oFloatWin);
  static void on_menu_query_activate(GtkWidget * widget, FloatWin *oFloatWin);
  static void on_menu_play_activate(GtkWidget * widget, FloatWin *oFloatWin);
  static void on_menu_fuzzyquery_activate(GtkWidget * widget, FloatWin *oFloatWin);
  
  void Popup(gboolean changeposition);
public:
  std::string QueryingWord;
  std::string PronounceWord;
  FloatWinQueryResult found_result;
  GtkWidget *FloatWindow, *lock_image, *button_hbox;
  GtkWidget *PronounceWordButton, *StopButton;
  GtkWidget *menu;
  std::auto_ptr<ArticleView> view;
  
  FloatWin();
  void Create();
  void End();
  void ShowText(gchar ***pppWord, gchar **** ppppWordData, const gchar * sOriginWord);
  void ShowText(gchar ****ppppWord, gchar ***** pppppWordData, const gchar ** ppOriginWord, gint count,const gchar * sOriginWord);
  void ShowNotFound(const char* sWord,const char* sReason, gboolean fuzzy);
  void Show();
  void Hide();
};

#endif
