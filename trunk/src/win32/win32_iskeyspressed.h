#ifndef _WIN32_ISKEYSPRESSED_HPP_
#define _WIN32_ISKEYSPRESSED_HPP_

#include "../iskeyspressed.hpp"

class win32_hotkeys : public hotkeys {
public:
	win32_hotkeys();
	const std::list<std::string>& possible_combs();
	void set_comb(const std::string& comb);
	bool is_pressed();
private:
  int comb_mask;
  static std::list<std::string> posb_combs;
};

#endif//!_WIN32_ISKEYSPRESSED_HPP_
