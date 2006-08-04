#ifndef _GCONF_FILE_HPP_
#define _GCONF_FILE_HPP_

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <string>
#include <vector>

#include "config_file.hpp"

class gconf_file : public config_file {
public:
	explicit gconf_file(const std::string& path);
	~gconf_file();
	bool read_bool(const gchar *sect, const gchar *key, bool& val);
	bool read_int(const gchar *sect, const gchar *key, int& val);
	bool read_string(const gchar * sect, const gchar *key, std::string& val);
	bool read_strlist(const gchar * sect, const gchar * key, std::list<std::string>& slist);

	void write_bool(const gchar *sect, const gchar *key, bool val);
	void write_int(const gchar *sect, const gchar *key, int val);
	void write_string(const gchar *sect, const gchar *key, const std::string& val);
	void write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist);
	void notify_add(const gchar *, const gchar *, 
			const sigc::slot<void, const baseconfval*>&);
private:
	std::string cfgname;
	GConfClient *gconf_client;
	std::vector<guint> notification_ids;
};

#endif//!_GCONF_FILE_HPP_
