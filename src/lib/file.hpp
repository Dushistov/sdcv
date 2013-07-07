#pragma once

#include <list>
#include <string>
#include <functional>

extern void for_each_file(const std::list<std::string>& dirs_list, const std::string& suff,
                          const std::list<std::string>& order_list, const std::list<std::string>& disable_list, 
                          const std::function<void (const std::string&, bool)>& f);

