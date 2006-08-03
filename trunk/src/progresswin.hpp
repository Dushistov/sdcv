#ifndef _PROGRESSWIN_HPP_
#define _PROGRESSWIN_HPP_

#include <string>

class progress_win {
public:
	progress_win();
	~progress_win() { gtk_widget_destroy(win); }
	void display_action(const std::string& actname);
private:
	GtkLabel *text;
	GtkProgressBar *progress;
	GtkWidget *win;
};

#endif//!_PROGRESSWIN_HPP_
