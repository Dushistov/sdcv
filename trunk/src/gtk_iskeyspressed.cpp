/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 *
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

#include "gtk_iskeyspressed.hpp"

//gtk_iskeyspressed don't work fine in Win32, when in the windows application window, modifier key information can't be get.
//Do we need to develop a win32_iskeyspressed module?

std::list<std::string> gtk_hotkeys::posb_combs;

gtk_hotkeys::gtk_hotkeys(GtkWindow *win_) : win(win_) 
{
  if (posb_combs.empty()) {
#ifndef _WIN32
    posb_combs.push_back("Win");
#endif
    posb_combs.push_back("Shift");
    posb_combs.push_back("Alt");
    posb_combs.push_back("Ctrl");
    posb_combs.push_back("Ctrl+Alt");
  }
}

const std::list<std::string>& gtk_hotkeys::possible_combs()
{
  return posb_combs;
}

void gtk_hotkeys::set_comb(const std::string& comb)
{
  comb_mask=0;
  std::string::size_type pos=std::string::npos;
  do {
    std::string::size_type prev_pos=pos;
    pos=comb.find('+', pos+1);
    std::string cur;
    if (pos!=std::string::npos)
      cur.assign(comb, prev_pos+1, pos-prev_pos-1);
    else
      cur.assign(comb, prev_pos+1, comb.length()-prev_pos-1);

    if (cur=="Ctrl")
      comb_mask|=GDK_CONTROL_MASK;
    else if (cur=="Shift")
      comb_mask|=GDK_SHIFT_MASK;
    else if (cur=="Alt")
      comb_mask|=GDK_MOD1_MASK;
    else if (cur=="Win") 
      comb_mask|=GDK_MOD4_MASK;
  } while (pos!=std::string::npos);
}

bool gtk_hotkeys::is_pressed(void)
{
  GdkScreen *screen = gtk_window_get_screen(win);
  GdkDisplay *display = gdk_screen_get_display(screen);
  GdkModifierType mask;
  gdk_display_get_pointer(display, NULL, NULL, NULL, &mask);
  bool pressed=false;
  /*
    typedef enum
    {
    GDK_SHIFT_MASK    = 1 << 0,
    GDK_LOCK_MASK     = 1 << 1,
    GDK_CONTROL_MASK  = 1 << 2,
    GDK_MOD1_MASK     = 1 << 3,
    GDK_MOD2_MASK     = 1 << 4,
    GDK_MOD3_MASK     = 1 << 5,
    GDK_MOD4_MASK     = 1 << 6,
    GDK_MOD5_MASK     = 1 << 7,
    GDK_BUTTON1_MASK  = 1 << 8,
    GDK_BUTTON2_MASK  = 1 << 9,
    GDK_BUTTON3_MASK  = 1 << 10,
    GDK_BUTTON4_MASK  = 1 << 11,
    GDK_BUTTON5_MASK  = 1 << 12,
  */

  mask = GdkModifierType(guint(mask) & (1<<13)-1);

  mask = 
    GdkModifierType(guint(mask) & ~(GDK_LOCK_MASK | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
				    GDK_BUTTON3_MASK | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK |
				    GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD5_MASK));

  if (mask == comb_mask)
    pressed=true;

  return pressed;
}


