#ifndef _GTK_ISKEYSPRESSED_HPP_
#define _GTK_ISKEYSPRESSED_HPP_

#include <gtk/gtk.h>

#include "iskeyspressed.hpp"

class gtk_hotkeys : public hotkeys {
public:
  gtk_hotkeys(GtkWindow *win_);
	const std::list<std::string>& possible_combs();
  void set_comb(const std::string& comb);
  bool is_pressed();
private:
  GtkWindow *win;
  gint comb_mask;
  static std::list<std::string> posb_combs;
};

#endif//!_GTK_ISKEYSPRESSED_HPP_
