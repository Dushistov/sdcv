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

/*
 * goal: react on key press even if there are window 
 * have no focus in XWindows. This code based on xbindkeys.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gtk_iskeyspressed.hpp"

#include "x11_iskeyspressed.hpp"



unsigned int x11_hotkeys::numlock_mask, x11_hotkeys::scrolllock_mask, 
  x11_hotkeys::capslock_mask;

void x11_hotkeys::get_offending_modifiers(Display * dpy)
{
  int i;
  XModifierKeymap *modmap;
  KeyCode nlock, slock;
  static int mask_table[8] = {
    ShiftMask, LockMask, ControlMask, Mod1Mask,
    Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
  };

  nlock = XKeysymToKeycode (dpy, XK_Num_Lock);
  slock = XKeysymToKeycode (dpy, XK_Scroll_Lock);

  /*
   * Find out the masks for the NumLock and ScrollLock modifiers,
   * so that we can bind the grabs for when they are enabled too.
   */
  modmap = XGetModifierMapping (dpy);

  if (modmap != NULL && modmap->max_keypermod > 0) {
    for (i = 0; i < 8 * modmap->max_keypermod; i++) {
      if (modmap->modifiermap[i] == nlock && nlock != 0)
				numlock_mask = mask_table[i / modmap->max_keypermod];
      else if (modmap->modifiermap[i] == slock && slock != 0)
				scrolllock_mask = mask_table[i / modmap->max_keypermod];
    }
  }

  capslock_mask = LockMask;

  if (modmap)
    XFreeModifiermap (modmap);
}

void x11_hotkeys::my_grab_key(Display * dpy, KeyCode keycode, 
			   unsigned int modifier, Window win)
{
  modifier &= ~(numlock_mask | capslock_mask | scrolllock_mask);


  XGrabKey(dpy, keycode, modifier, (win ? win : DefaultRootWindow(dpy)),
	   False, GrabModeAsync, GrabModeAsync);

  if (modifier == AnyModifier)
    return;

  if (numlock_mask)
    XGrabKey (dpy, keycode, modifier | numlock_mask,
	      (win ? win : DefaultRootWindow (dpy)),
	      False, GrabModeAsync, GrabModeAsync);

  if (capslock_mask)
    XGrabKey (dpy, keycode, modifier | capslock_mask,
	      (win ? win : DefaultRootWindow (dpy)),
	      False, GrabModeAsync, GrabModeAsync);

  if (scrolllock_mask)
    XGrabKey (dpy, keycode, modifier | scrolllock_mask,
	      (win ? win : DefaultRootWindow (dpy)),
	      False, GrabModeAsync, GrabModeAsync);

  if (numlock_mask && capslock_mask)
    XGrabKey (dpy, keycode, modifier | numlock_mask | capslock_mask,
	      (win ? win : DefaultRootWindow (dpy)),
	      False, GrabModeAsync, GrabModeAsync);

  if (numlock_mask && scrolllock_mask)
    XGrabKey (dpy, keycode, modifier | numlock_mask | scrolllock_mask,
	      (win ? win : DefaultRootWindow (dpy)),
	      False, GrabModeAsync, GrabModeAsync);

  if (capslock_mask && scrolllock_mask)
    XGrabKey (dpy, keycode, modifier | capslock_mask | scrolllock_mask,
	      (win ? win : DefaultRootWindow (dpy)),
	      False, GrabModeAsync, GrabModeAsync);

  if (numlock_mask && capslock_mask && scrolllock_mask)
    XGrabKey (dpy, keycode,
	      modifier | numlock_mask | capslock_mask | scrolllock_mask,
	      (win ? win : DefaultRootWindow (dpy)), False, GrabModeAsync,
	      GrabModeAsync);

}


void x11_hotkeys::my_grab_button(Display * dpy, unsigned int button, 
			      unsigned int modifier, Window win)
{
  modifier &= ~(numlock_mask | capslock_mask | scrolllock_mask);

  XGrabButton (dpy, button, modifier, (win ? win : DefaultRootWindow (dpy)),
	       False, ButtonPressMask | ButtonReleaseMask,
	       GrabModeAsync, GrabModeAsync, None, None);

  if (modifier == AnyModifier)
    return;

  if (numlock_mask)
    XGrabButton (dpy, button, modifier | numlock_mask,
		 (win ? win : DefaultRootWindow (dpy)),
		 False, ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None, None);


  if (capslock_mask)
    XGrabButton (dpy, button, modifier | capslock_mask,
		 (win ? win : DefaultRootWindow (dpy)),
		 False, ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None, None);

  if (scrolllock_mask)
    XGrabButton (dpy, button, modifier | scrolllock_mask,
		 (win ? win : DefaultRootWindow (dpy)),
		 False, ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None, None);

  if (numlock_mask && capslock_mask)
    XGrabButton (dpy, button, modifier | numlock_mask | capslock_mask,
		 (win ? win : DefaultRootWindow (dpy)),
		 False, ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None, None);

  if (numlock_mask && scrolllock_mask)
    XGrabButton (dpy, button, modifier | numlock_mask | scrolllock_mask,
		 (win ? win : DefaultRootWindow (dpy)),
		 False, ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None, None);

  if (capslock_mask && scrolllock_mask)
    XGrabButton (dpy, button, modifier | capslock_mask | scrolllock_mask,
		 (win ? win : DefaultRootWindow (dpy)),
		 False, ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None, None);

  if (numlock_mask && capslock_mask && scrolllock_mask)
    XGrabButton (dpy, button,
		 modifier | numlock_mask | capslock_mask | scrolllock_mask,
		 (win ? win : DefaultRootWindow (dpy)), False,
		 ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
		 GrabModeAsync, None, None);
}

bool x11_hotkeys::grab_keys(Display *dpy, Keys_t keys[], int nb_keys)
{
  int i;
  int min, max;
  int screen;
  

  XDisplayKeycodes(dpy, &min, &max);

#ifdef DEBUG  
  printf("\n");
  printf("min_keycode=%d     max_keycode=%d (ie: know keycodes)\n", 
	 min, max);
#endif  


  for (i = 0; i < nb_keys; i++) {
#ifdef DEBUG
    print_key(dpy, &keys[i]);
#endif
    if (keys[i].type == SYM) {
      for (screen = 0; screen < ScreenCount (dpy); screen++) {
				my_grab_key(dpy, XKeysymToKeycode(dpy, keys[i].key.sym),
										keys[i].modifier, RootWindow(dpy, screen));
      }
    } else if (keys[i].type == BUTTON) {
      for (screen = 0; screen < ScreenCount (dpy); screen++) {
	my_grab_button(dpy, keys[i].key.button, keys[i].modifier,
		       RootWindow (dpy, screen));
      }
    } else {
      if (keys[i].key.code >= min && keys[i].key.code <= max) {
	for (screen = 0; screen < ScreenCount (dpy); screen++) {
	  my_grab_key (dpy, keys[i].key.code, keys[i].modifier,
		       RootWindow (dpy, screen));
	}
      } else {
#ifdef DEBUG	
	print_key (dpy, &keys[i]);


	fprintf (stderr,
		 "  The keycode %d cannot be used, as it's not between the\n"
		 "  min(%d) and max(%d) keycode of your keyboard.\n"
		 "  Please increase the 'maximum' value in\n"
		 "    /usr/X11R6/lib/X11/xkb/keycodes/xfree86,\n"
		 "  then restart X.\n", keys[i].key.code, min, max);	
#endif	
	return false;
      }
    }
  }

  return true;
}


x11_hotkeys::x11_hotkeys(GtkWindow *win_) : 
  win(win_),
  pressed(false),
  def_hot_keys(new gtk_hotkeys(win_))
{
  possb_combs=def_hot_keys->possible_combs();
  possb_combs.push_back("Ctrl+e");
  possb_combs.push_back("F1");
  possb_combs.push_back("F2");
  possb_combs.push_back("F3");
  possb_combs.push_back("F4");
  display=gdk_x11_display_get_xdisplay(gdk_screen_get_display(gtk_window_get_screen(win)));
  get_offending_modifiers(display);
  gdk_window_add_filter(NULL, GdkFilterFunc(key_filter), this);
}

x11_hotkeys::~x11_hotkeys()
{
  ungrabkeys();
}

const std::list<std::string>& x11_hotkeys::possible_combs()
{
  return possb_combs;
}

void x11_hotkeys::set_comb(const std::string& val)
{
  if (comb==val)
    return;

  unsigned int modifier=0;
  KeySym key=0;
  std::string::size_type pos=std::string::npos;

  do {
    std::string::size_type prev_pos=pos;
    pos=val.find('+', pos+1);
    std::string cur;
    if (pos!=std::string::npos)
      cur.assign(val, prev_pos+1, pos-prev_pos-1);
    else
      cur.assign(val, prev_pos+1, val.length()-prev_pos-1);
    if (cur=="Ctrl") {
      modifier|=ControlMask;
    } else if (cur=="Alt") {
      modifier|=Mod1Mask;
    } else if (cur=="Shift") {
      modifier|=ShiftMask;
    } else if (cur=="Win") {
      modifier|=Mod4Mask;
    } else {
      key=XStringToKeysym(cur.c_str());
	}
  } while (pos!=std::string::npos);

  comb=val;
  pressed=false;
  grab_key.key.sym=0;
  if (key!=0) {
    ungrabkeys();
    grab_key.type=SYM;
    grab_key.event_type=PRESS;
    grab_key.key.sym=key;
    grab_key.modifier=modifier;
    grab_keys(display, &grab_key, 1);
  } else
    def_hot_keys->set_comb(val);
}

bool x11_hotkeys::is_pressed()
{
  if (grab_key.key.sym!=0)
    return pressed;
  
  return def_hot_keys->is_pressed();
}

void x11_hotkeys::ungrabkeys()
{ 
  if (grab_key.key.sym==0)
    return;
  Display *d=display;

  for (int screen = 0; screen<ScreenCount(d); screen++) {
    XUngrabKey(d, AnyKey, AnyModifier, RootWindow (d, screen));
    XUngrabButton(d, AnyButton, AnyModifier, RootWindow (d, screen));
  }  
}

GdkFilterReturn x11_hotkeys::key_filter(GdkXEvent *gdk_xevent, 
				     GdkEvent *event,
				     x11_hotkeys *th) 
{
  int type;
  XKeyEvent *xevent;
  
  xevent = (XKeyEvent *)gdk_xevent;
  type = xevent->type;
  KeySym keysym = XKeycodeToKeysym(GDK_DISPLAY(), ((XKeyEvent *)xevent)->keycode, 0);
  unsigned int state=xevent->state;
  state &= ~(numlock_mask | capslock_mask | scrolllock_mask);
  if (type == KeyPress) {    
    if (keysym==th->grab_key.key.sym && state==th->grab_key.modifier) {
      th->pressed=true;    
    }
  } else if (type==KeyRelease) {
    if (keysym==th->grab_key.key.sym) {
      th->pressed=false;
    }
  }

  return GDK_FILTER_CONTINUE;
}
