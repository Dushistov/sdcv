#ifndef _CONFIG_FILE_HPP_
#define _CONFIG_FILE_HPP_

#include <glib.h>
#include <list>
#include <string>

template<typename T>
struct change_handler {
	void (*on_change)(T, void *);
	void *arg;
	change_handler() : on_change(NULL), arg(NULL) {}
	void operator()(T val) { 
		if (on_change)
			on_change(val, arg);
	}
};

struct baseconfval {
	enum var_t {
		UNKNOWN_TYPE,	BOOL_TYPE, INT_TYPE,
		STRING_TYPE, STRLIST_TYPE
	} type;

	baseconfval() : type (UNKNOWN_TYPE) {}
	virtual ~baseconfval() {}
};

template<typename T> 
struct confval : public baseconfval {
	T val;
};

/*
 * This interface must be implemented if you want 
 * realize your own way of loading and saving stardict's preference
 */
class config_file {
public:
	virtual ~config_file() {}
	virtual bool read_bool(const gchar *sect, const gchar *key, bool& val) = 0;
  virtual bool read_int(const gchar *sect, const gchar *key, int& val) = 0;
  virtual bool read_string(const gchar *sect, const gchar *key, std::string& val) = 0;
  virtual bool read_strlist(const gchar * sect, const gchar *key, std::list<std::string>& slist) = 0;

  virtual void write_bool(const gchar *sect, const gchar *key, bool val) = 0;
  virtual void write_int(const gchar *sect, const gchar *key, int val) = 0;
  virtual void write_string(const gchar *sect, const gchar *key, const std::string& val) = 0;
  virtual void write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist) = 0;
	virtual void notify_add(const gchar *sect, const gchar *key, 
													void (*on_change)(const baseconfval*, void *), void *arg) = 0;
};

#endif//!_CONFIG_FILE_HPP_
