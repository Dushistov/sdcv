#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <string>
#include <vector>
#include <gdk-pixbuf/gdk-pixbuf.h>

extern void play_wav_file(const std::string& filename);
extern void show_help(const gchar *section);
extern void show_url(const gchar *url);
extern void ProcessGtkEvent();
extern void play_sound_on_event(const gchar *eventname);

//sinse glib 2.6 we have g_get_user_config_dir
//but because of compability with other 2.x...
extern std::string get_user_config_dir();
extern std::string combnum2str(gint comb_code);
extern std::vector<std::string> split(const std::string& str, char sep);
extern GdkPixbuf *load_image_from_file(const std::string& filename);

#endif/*UTILS_H*/
