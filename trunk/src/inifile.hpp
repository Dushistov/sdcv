#ifndef _INIFILE_HPP_
#define _INIFILE_HPP_

#include <map>
#include <string>

#include "config_file.hpp"

struct config_line {
	std::string key;
	std::string value;
	config_line() {}
	config_line(const	std::string& k, const std::string& v) :
		key(k), value(v) {}
};

struct config_section {
	std::string name;
	std::list<config_line> lines;
	config_section() {}
	explicit config_section(const std::string& n) : name(n) {}
	config_line *create_string(const std::string& key, const std::string& value);
	config_line *find_string(const std::string& key);
};

class inifile : public config_file {
public:
	explicit inifile(const std::string& path);
	bool read_bool(const gchar *sect, const gchar *key, bool& val);
	bool read_int(const gchar *sect, const gchar *key, int& val);
	bool read_string(const gchar * sect, const gchar *key, std::string& val);
	bool read_strlist(const gchar * sect, const gchar *key, std::list<std::string>& slist);

	void write_bool(const gchar *sect, const gchar *key, bool val);
	void write_int(const gchar *sect, const gchar *key, int val);
	void write_string(const gchar *sect, const gchar *key, const std::string& val);
	void write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist);

	void notify_add(const gchar *, const gchar *, 
			const sigc::slot<void, const baseconfval*>&);
private:
	std::string cfgfilename;
	std::list<config_section> sections;
	typedef std::map<std::string, sigc::signal<void, const baseconfval *> >
	ChangeEventsMap;
	ChangeEventsMap change_events_map_;

	config_section *create_section(const std::string& name);
	config_section *find_section(const std::string& name);
	bool open_inifile(const std::string& filename);
	bool saveas(const std::string& filename);
	inline void save(){ saveas(cfgfilename); }

	template <typename T>
	void expose_event(const gchar *sect, const gchar *key, const T& val) {
		std::string name(std::string(sect)+"/"+key);
		ChangeEventsMap::iterator p = change_events_map_.find(name);
		if (p == change_events_map_.end())
			return;
		confval<T> cv;
		cv.val = val;
		p->second.emit(&cv);
	}

	void save_string(const gchar *sect, const gchar *key, const std::string& val);
};

#endif//!_INIFILE_HPP_
