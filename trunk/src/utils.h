#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <string>
#include <vector>
#include <gdk-pixbuf/gdk-pixbuf.h>

template <typename T, typename UnrefType, void (*unref_func)(UnrefType *)>
class ResourceWrapper {
public:
	ResourceWrapper(T *p = NULL) : p_(p) {}
	~ResourceWrapper() { free_resource(); }
	T *get() { return p_; }
	void reset(T *newp) {
		if (p_ != newp) {
			free_resource();
			p_ = newp;
		}
	}
private:
	T *p_;

	void free_resource() { 
		if (p_) 
			unref_func(p_); 
		//p_ = NULL;// - not need
	}
};

extern void play_wav_file(const std::string& filename);
extern void show_help(const gchar *section);
extern void show_url(const gchar *url);
extern void ProcessGtkEvent();
extern void play_sound_on_event(const gchar *eventname);

//sinse glib 2.6 we have g_get_user_config_dir
//but because of compability with other 2.x...
extern std::string get_user_config_dir();
extern std::string combnum2str(gint comb_code);
extern void split(const std::string& str, char sep, std::vector<std::string>& res);
extern GdkPixbuf *load_image_from_file(const std::string& filename);

#endif/*UTILS_H*/
