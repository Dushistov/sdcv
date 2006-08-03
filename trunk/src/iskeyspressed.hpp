#ifndef _ISKEYSPRESSED_HPP_
#define _ISKEYSPRESSED_HPP_

#include <string>
#include <list>

class hotkeys {
public:
  virtual ~hotkeys() {}
  virtual const std::list<std::string>& possible_combs() = 0;
  virtual void set_comb(const std::string& comb) = 0;
  virtual bool is_pressed() = 0;
};

#endif//!_ISKEYSPRESSED_HPP_
