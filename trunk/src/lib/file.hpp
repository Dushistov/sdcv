#pragma once

#include <list>
#include <string>
#include <functional>

typedef std::list<std::string> List;

extern void for_each_file(const List& dirs_list, const std::string& suff,
                          const List& order_list, const List& disable_list, 
                          const std::function<void (const std::string&, bool)>& f);

