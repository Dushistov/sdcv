#ifndef _X11_ISKEYSPRESSED_HPP_
#define _X11_ISKEYSPRESSED_HPP_

#include <gdk/gdkx.h>
#include <X11/keysym.h>
#include <gtk/gtk.h>

#include "iskeyspressed.hpp"

typedef enum { SYM, CODE, BUTTON } KeyType_t;
typedef enum { PRESS, RELEASE } EventType_t;

struct Keys_t {

  KeyType_t type;

  EventType_t event_type;

  union {
    KeySym sym;
    KeyCode code;
    unsigned int button;
  } key;

  unsigned int modifier;

  Keys_t(KeyType_t t=SYM, EventType_t et=PRESS, KeySym s=0, unsigned int m=0) :
    type(t), event_type(et), modifier(m) 
  {
    key.sym=s;
  }
};

class x11_hotkeys : public hotkeys {
public:
  x11_hotkeys(GtkWindow *win_);
	~x11_hotkeys();
	const std::list<std::string>& possible_combs();
  void set_comb(const std::string& comb);
  bool is_pressed();
private:
  std::string comb;
  GtkWindow *win;
  bool pressed;
  Keys_t grab_key;
  std::list<std::string> possb_combs;
  std::auto_ptr<hotkeys> def_hot_keys;/*I don't want to duplicate work, 
																				so hold here pointer to default 
																				object of this class*/
  Display *display;
  static unsigned int numlock_mask, scrolllock_mask, capslock_mask;

  static GdkFilterReturn key_filter(GdkXEvent *gdk_xevent, 
																		GdkEvent *event, x11_hotkeys *th);
  void ungrabkeys(void);
  static void get_offending_modifiers(Display * dpy);
  static void my_grab_key(Display * dpy, KeyCode keycode, 
													unsigned int modifier, Window win);
  static void my_grab_button(Display * dpy, unsigned int button, 
														 unsigned int modifier, Window win);
  static bool grab_keys(Display *dpy, Keys_t keys[], int nb_keys);
};

#endif//!_X11_ISKEYSPRESSED_HPP_
