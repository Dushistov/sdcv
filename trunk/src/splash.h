#ifndef __SD_SPLASH_H__
#define __SD_SPLASH_H__

#include <string>

#include <gtk/gtk.h>

class splash_screen {
public:
	void display_action(const std::string& actname);
	void show();
private:
	GtkLabel *text;
	GtkProgressBar *progress;
};

extern splash_screen stardict_splash;

#endif
