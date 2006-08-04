#ifndef __SD_CONF_H__
#define __SD_CONF_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>


#include "config_file.hpp"

const int MIN_MAX_FLOATWIN_WIDTH=80; // if user 's MAX_FLOATWIN_WIDTH setting is less then this,it will be set to DEFAULT_MAX_FLOATWIN_WIDTH.
const int MIN_MAX_FLOATWIN_HEIGHT=60;
#ifdef CONFIG_GPE
const int DEFAULT_MAX_FLOATWIN_WIDTH=160;
const int DEFAULT_MAX_FLOATWIN_HEIGHT=120;
#else
const int DEFAULT_MAX_FLOATWIN_WIDTH=320; // This is the lable's width in fact.
const int DEFAULT_MAX_FLOATWIN_HEIGHT=240;
#endif


/*
 * AppConf class encapsulate
 * all preference of stardict.
*/

template <class T>
struct Type2Type {
	typedef T OriginType;
};

class AppConf {
public:
	AppConf();
	~AppConf();
	void load();

	typedef std::map<std::string, baseconfval *> cache_t;

	const bool& get_bool(const std::string& name) const {
		return get(name, Type2Type<bool>());
	}

	const bool& get_bool_at(const std::string& name) const {
		return get_at(name, Type2Type<bool>());
	}

	const int& get_int(const std::string& name) const {
		return get(name, Type2Type<int>());
	}

	const int& get_int_at(const std::string& name) const {
		return get_at(name, Type2Type<int>());
	}

	const std::string& get_string(const std::string& name) const {
		return get(name, Type2Type<std::string>());
	}

	const std::string& get_string_at(const std::string& name) const {
		return get_at(name, Type2Type<std::string>());
	}

	const std::list<std::string>& get_strlist(const std::string& name) const {
		return get(name, Type2Type< std::list<std::string> >());
	}

	const std::list<std::string>& get_strlist_at(const std::string& name) const {
		return get_at(name, Type2Type< std::list<std::string> >());
	}

	void set_bool(const std::string& name, const bool& v) {
		set_value(name, v);
	}

	void set_bool_at(const std::string& name, const bool& v) {
		set_value_at(name, v);
	}

	void set_int(const std::string& name, const int& v) {
		set_value(name, v);
	}

	void set_int_at(const std::string& name, const int& v) {
		set_value_at(name, v);
	}

	void set_string(const std::string& name, const std::string& v) {
		set_value(name, v);
	}

	void set_string_at(const std::string& name, const std::string& v) {
		set_value_at(name, v);
	}

	void set_strlist(const std::string& name, const std::list<std::string>& v) {
		set_value(name, v);
	}

	void set_strlist_at(const std::string& name, const std::list<std::string>& v) {
		set_value_at(name, v);
	}


	void notify_add(const std::string& name, void (*on_change)(const baseconfval*, void *), void *arg);
private:
	std::auto_ptr<config_file> cf_;
	cache_t cache_;

	template <typename T>
	void set_value(const std::string& name, const T& val) {
		cache_t::iterator p = cache_.find(name);
		if (p == cache_.end())
			return;
		if (static_cast<confval<T> *>(p->second)->val == val)
			return;

		static_cast<confval<T> *>(p->second)->val = val;
		save_value(name, p->second);
	}

	template <typename T>
	void set_value_at(const std::string& name, const T& val) {
		set_value("/apps/stardict/preferences/" + name, val);
	}

	static std::string get_default_history_filename();
	static std::string get_default_export_filename();
	static std::list<std::string> get_default_search_website_list();
#ifdef _WIN32
	static bool get_win32_use_custom_font();
	static std::string get_win32_custom_font();
#endif
	void save_value(const std::string& name, const baseconfval* cv);

	template <typename T>
	void add_entry_at(const std::string& name, const T& def_val) {
		add_entry("/apps/stardict/preferences/" + name, def_val);		
	}

	template <typename T>
	void add_entry(const std::string& name, const T& def_val) {
		confval<T> *v = new confval<T>;
		if (typeid(T) == typeid(bool))
			v->type=baseconfval::BOOL_TYPE;
		else if (typeid(T) == typeid(int))
			v->type=baseconfval::INT_TYPE;
		else if (typeid(T) == typeid(std::string))
			v->type=baseconfval::STRING_TYPE;
		else if (typeid(T) == typeid(std::list<std::string>))
			v->type=baseconfval::STRLIST_TYPE;
		v->val = def_val;
		
		cache_[name] = v;
	}

	template <typename T>
	const T& get(const std::string& name, Type2Type<T>) const {
		static T empty;
		cache_t::const_iterator p = cache_.find(name);
		if (p != cache_.end())
			return static_cast<confval<T> *>(p->second)->val;
		return empty;
	}

	template <typename T>
	const T& get_at(const std::string& name, Type2Type<T> t2t) const {
		return get("/apps/stardict/preferences/" + name, t2t);
	}
};

extern std::auto_ptr<AppConf> conf;//global exemplar of AppConf class
extern std::string gStarDictDataDir;

#endif
