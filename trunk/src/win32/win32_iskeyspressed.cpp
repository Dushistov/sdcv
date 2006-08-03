#include "win32_iskeyspressed.h"
#include <windows.h>

std::list<std::string> win32_hotkeys::posb_combs;

win32_hotkeys::win32_hotkeys()
{
  if (posb_combs.empty()) {
    posb_combs.push_back("Shift");
    posb_combs.push_back("Alt");
    posb_combs.push_back("Ctrl");
    posb_combs.push_back("Ctrl+Alt");
  }
}

const std::list<std::string>& win32_hotkeys::possible_combs()
{
  return posb_combs;
}

void win32_hotkeys::set_comb(const std::string& comb)
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
      comb_mask|=MOD_CONTROL;
    else if (cur=="Shift")
      comb_mask|=MOD_SHIFT;
    else if (cur=="Alt")
      comb_mask|=MOD_ALT;
  } while (pos!=std::string::npos);
}

bool win32_hotkeys::is_pressed()
{
	if (comb_mask & MOD_CONTROL) {
		if (!(GetAsyncKeyState(VK_CONTROL)&0x8000))
			return false;
	}
	if (comb_mask & MOD_SHIFT) {
		if (!(GetAsyncKeyState(VK_SHIFT)&0x8000))
			return false;
	}
	if (comb_mask & MOD_ALT) {
		if (!(GetAsyncKeyState(VK_MENU)&0x8000))
			return false;
	}
	return true;
}
