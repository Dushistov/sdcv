/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <algorithm>
#include <functional>
#include <cerrno>
#include <glib/gi18n.h>
#include <glib/gstdio.h>


#include "inifile.hpp"

const char STRING_SEP = 0xff;

//---------------------------------------------------------------------------------
//may be use split from utils module ???
void str2list(const gchar *str, std::list<std::string> &slist)
{
  gchar *p;
	slist.clear();
	//TODO: const_cast is peace of shit, need more good solution
  while ((p = strchr(const_cast<char *>(str), STRING_SEP))!=NULL) {
    slist.push_back(std::string(str, p - str));
    str = p+1;
	}
  if (str[0])
    slist.push_back(str);
}

//---------------------------------------------------------------------------------
config_line *config_section::create_string(const std::string& key, const std::string& value)
{
	lines.push_back(config_line(key, value));
	return &lines.back();
}
//---------------------------------------------------------------------------------
class with_such_key : public std::unary_function<const config_line&, bool> {
public:
	explicit with_such_key(const std::string& k) : key(k) {}
	bool operator()(const config_line& l) { return l.key==key; }
private:
	const std::string& key;
};

config_line *config_section::find_string(const std::string& key)
{
	std::list<config_line>::iterator it=
		std::find_if(lines.begin(), lines.end(), with_such_key(key));
	if (it==lines.end())
		return NULL;
	return &(*it);
}
//---------------------------------------------------------------------------------
config_section *inifile::create_section(const std::string& name)
{
	sections.push_back(config_section(name));
	return &sections.back();
}
//---------------------------------------------------------------------------------
class with_such_name : public std::unary_function<const config_section&, bool> {
public:
	explicit with_such_name(const std::string& n) : name(n) {}
	bool operator()(const config_section& s) { return s.name==name; }
private:
	const std::string& name;
};

config_section *inifile::find_section(const std::string& name)
{
	std::list<config_section>::iterator it=
		std::find_if(sections.begin(), sections.end(), with_such_name(name));
	if (it==sections.end()) 
		return NULL;
	return &(*it);
}
//---------------------------------------------------------------------------------
bool inifile::open_inifile(const std::string& filename)
{	  
  gchar *tmp;
  gint i;

  cfgfilename = filename;  

	gchar *buffer=NULL;
	if (!g_file_get_contents(filename.c_str(), &buffer, NULL, NULL))
		return false;

  gchar **lines = g_strsplit(buffer, "\n", 0);
  g_free(buffer);
  i = 0;
  while (lines[i]) {
    if (lines[i][0] == '[') {
      if ((tmp = strchr(lines[i], ']'))) {
				*tmp = '\0';
				create_section(&lines[i][1]);
      }
    } else if(lines[i][0] != '#' && !sections.empty()) {
      if ((tmp = strchr (lines[i], '='))) {
				*tmp = '\0';
				tmp++;
				sections.back().create_string(lines[i], tmp);
      }
    }
    i++;
  }
  g_strfreev(lines);

  return true;
}
//---------------------------------------------------------------------------------
bool inifile::saveas(const std::string& filename)
{ 
  FILE *file=g_fopen(filename.c_str(), "wb"); 
  if (!file) {
    g_warning(_("Can not open: %s - %s\n"), filename.c_str(), strerror(errno));
    return false;
  }

  
	for (std::list<config_section>::iterator i=sections.begin();
			 i!=sections.end(); ++i)
    if (!i->lines.empty()) {
      fprintf(file, "[%s]\n", i->name.c_str());
			for (std::list<config_line>::iterator j=i->lines.begin();
					 j!=i->lines.end(); ++j)
				fprintf(file, "%s=%s\n", j->key.c_str(), j->value.c_str());
      
      fprintf(file, "\n");
    }
	
  fclose(file);
  return true;
}
//---------------------------------------------------------------------------------
inifile::inifile(const std::string& path)
{	
	cfgfilename=path;
	open_inifile(cfgfilename);
}
//---------------------------------------------------------------------------------
bool inifile::read_bool(const gchar *sect, const gchar *key, bool& val)
{
	std::string str;

  if (!read_string(sect, key, str))
    return false;  

  if (str=="0")
    val = false;
	else
    val = true;

	return true;
}
//---------------------------------------------------------------------------------
bool inifile::read_int(const gchar *sect, const gchar *key, int& val)
{
	std::string str;

  if (!read_string(sect, key, str))
    return false;
  
  val = atoi(str.c_str());

	return true;
}
//---------------------------------------------------------------------------------
bool inifile::read_string(const gchar * sect, const gchar *key, std::string& val)
{
	config_section *section=find_section(sect);
	if (!section)
		return false;

  config_line *line=section->find_string(key);
	if (!line)
		return false;

  val = line->value;

	return true;
}
//---------------------------------------------------------------------------------
bool inifile::read_strlist(const gchar *sect, const gchar * key, std::list<std::string>& slist)
{
	std::string str;

  if (!read_string(sect, key, str))
    return false;
  

  str2list(str.c_str(), slist);

	return true;
}
//---------------------------------------------------------------------------------
void inifile::write_bool(const gchar *sect, const gchar *key, bool val)
{
	if (val)
		save_string(sect, key, "1");
	else
		save_string(sect, key, "0");
	expose_event(sect, key, val);
}
//---------------------------------------------------------------------------------
void inifile::write_int(const gchar *sect, const gchar *key, int val)
{
	gchar *strvalue = g_strdup_printf("%d", val);
  save_string(sect, key, strvalue);
  g_free(strvalue);
	expose_event(sect, key, val);
}
//---------------------------------------------------------------------------------
void inifile::save_string(const gchar *sect, const gchar *key, const std::string& val)
{
  config_section *section = find_section(sect);
  if (!section)
    section = create_section(sect);

  config_line *line=section->find_string(key);
  if (line) 
    line->value = val;
	else
    section->create_string(key, val);
  save();
}
//---------------------------------------------------------------------------------
void inifile::write_string(const gchar *sect, const gchar *key, const std::string& val)
{
	save_string(sect, key, val);
	expose_event(sect, key, val);
}
//---------------------------------------------------------------------------------
void inifile::write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist)
{
	std::string str;
	std::list<std::string>::const_iterator p=slist.begin();

	if (p!=slist.end()) {
    str+=*p;
		++p;
  }

  while (p!=slist.end()) {
    str += STRING_SEP;
    str += *p;
		++p;
  }
 
  save_string(sect, key, str);
	expose_event(sect, key, slist);
}
//---------------------------------------------------------------------------------
void inifile::notify_add(const gchar *sect, const gchar *key, 
			 const sigc::slot<void, const baseconfval*>& slot)
{
	std::string name = std::string(sect) + "/" + key;
	ChangeEventsMap::iterator it =
		change_events_map_.insert(
			std::make_pair(name,
				       sigc::signal<void, const baseconfval*>())).first;
	it->second.connect(slot);
}
