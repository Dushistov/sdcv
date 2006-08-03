#ifndef _CLASS_FACTORY_HPP_
#define _CLASS_FACTORY_HPP_

#include <string>

class stardict_class_factory {
public:
	static void *create_class_by_name(const std::string& name, void *param=NULL);
};

#endif//!_CLASS_FACTORY_HPP_
