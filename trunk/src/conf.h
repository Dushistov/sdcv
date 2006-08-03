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

class AppConf {
public:
	AppConf();
	~AppConf();
  void Load();
	
	typedef std::map<std::string, baseconfval *> cache_t;

	const bool& get_bool(const std::string& name) const {
		static bool empty;
		cache_t::const_iterator p=cache.find(name);
		if (p!=cache.end())
			return static_cast<confval<bool> *>(p->second)->val;
		return empty;
	}

	const int& get_int(const std::string& name) const {
		static int empty;
		cache_t::const_iterator p=cache.find(name);
		if (p!=cache.end())
			return static_cast<confval<int> *>(p->second)->val;
		return empty;
	}

	const std::string& get_string(const std::string& name) const {
		static std::string empty;
		cache_t::const_iterator p=cache.find(name);
		if (p!=cache.end())
			return static_cast<confval<std::string> *>(p->second)->val;
		return empty;
	}

	const std::list<std::string>& get_strlist(const std::string& name) const {
		static std::list<std::string> empty;
		cache_t::const_iterator p=cache.find(name);
		if (p!=cache.end())
			return static_cast<confval<std::list<std::string> > *>(p->second)->val;
		return empty;
	}

	void set_bool(const std::string& name, const bool& v) {
		set_value(name, v);
	}

	void set_int(const std::string& name, const int& v) {
		set_value(name, v);
	}

	void set_string(const std::string& name, const std::string& v) {
		set_value(name, v);
	}

	void set_strlist(const std::string& name, const std::list<std::string>& v) {
		set_value(name, v);
	}
	
	template <typename T>
	void add_entry(const std::string& name, const T& def_val)
	{
		confval<T> *v=new confval<T>;
		if (typeid(T)==typeid(bool))
			v->type=baseconfval::BOOL_TYPE;
		else if (typeid(T)==typeid(int))
			v->type=baseconfval::INT_TYPE;
		else if (typeid(T)==typeid(std::string))
			v->type=baseconfval::STRING_TYPE;
		else if (typeid(T)==typeid(std::list<std::string>))
			v->type=baseconfval::STRLIST_TYPE;
		v->val=def_val;
		cache[name]=v;
	}
	void notify_add(const std::string& name, void (*on_change)(const baseconfval*, void *), void *arg);
private:
	std::auto_ptr<config_file> cf;
	cache_t cache;	

	template <typename T>
	void set_value(const std::string& name, const T& val) {
		cache_t::iterator p=cache.find(name);
		if (p==cache.end())
			return;
		if (static_cast<confval<T> *>(p->second)->val==val)
			return;

		static_cast<confval<T> *>(p->second)->val=val;
		save_value(name, p->second);
	}    	
	static std::string get_default_history_filename();
	static std::string get_default_export_filename();
	static std::list<std::string> get_default_search_website_list();
#ifdef _WIN32
	static bool get_win32_use_custom_font();
	static std::string get_win32_custom_font();
#endif
	void save_value(const std::string& name, const baseconfval* cv);
};

extern std::auto_ptr<AppConf> conf;//global exemplar of AppConf class
extern std::string gStarDictDataDir;

#endif
