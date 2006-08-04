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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <iostream>
#include <gtk/gtk.h>

#include "config_file.hpp"
#include "inifile.hpp"
#ifdef CONFIG_GNOME
# include "gconf_file.hpp"
#endif

using std::string;
using std::ostream_iterator;

typedef std::list<string> strlist;

std::ostream& operator<<(std::ostream& os, const strlist &sl)
{
	std::copy(sl.begin(), sl.end(), ostream_iterator<string>(os, "\n"));
	return os;
}

struct tmp_file {
	string fname;
	tmp_file(const string& fn) : fname(fn) {}
	~tmp_file() {
		if (remove(fname.c_str())==-1)
			std::cerr<<"can not remove: "<<fname<<". "<<strerror(errno)<<std::endl;
	}
};

class EventHandler : public sigc::trackable {
public:
	EventHandler() : get_event_(false) {}
	void on_event(const baseconfval*) { get_event_ = true; }
	bool is_recieve_event() const { return get_event_; }
private:
	bool get_event_;
};

#define TEST(type, def_val, some_val) do { \
		type type##_val = def_val; \
		cf->read_##type("/stardict-test", #type, type##_val); \
		if (type##_val != def_val) { \
			std::cerr<<"type: "<<#type<<", default value was changed"<<std::endl;	\
			std::cerr<<"default: "<<def_val<<std::endl<<"have: "<<type##_val<<std::endl; \
			return false; \
		}  \
		EventHandler ehandler;		\
		if (check_events) \
			cf->notify_add("/stardict-test", #type,		\
				       sigc::mem_fun(ehandler, &EventHandler::on_event)); \
		\
		cf->write_##type("/stardict-test", #type, def_val); \
		cf->write_##type("/stardict-test", #type, some_val); \
		if (check_events && !ehandler.is_recieve_event()) {		      \
			std::cerr << "type: " << #type << "handler was not exposed" << std::endl; \
			return false; \
		} \
		cf->read_##type("/stardict-test", #type, type##_val); \
		if (type##_val != some_val) { \
			std::cerr<<"type: "<<#type<<", default value was NOT changed"<<std::endl;  \
			std::cerr<<"set to: "<<some_val<<std::endl<<"get: "<<type##_val<<std::endl; \
			return false; \
		} \
	} while (0)

static bool is_test_passed(config_file *cf, bool check_events = true)
{
	TEST(bool, true, false);
	TEST(int, 17, 18);
	TEST(string, "aaa", "bbb");
	strlist def_val, some_val;
	def_val.push_back("a");
	def_val.push_back("bbb");
	def_val.push_back("cdad");

	some_val.push_back("a");
	some_val.push_back("bbb");
	some_val.push_back("cdad");
	some_val.push_back("fdfad");

	TEST(strlist, def_val, some_val);

	return true;
}

#ifdef CONFIG_GNOME
bool test_gconf()
{
	string tmp1=string(g_get_home_dir())+G_DIR_SEPARATOR_S+".gconf/stardict-test";
	string tmp2=string(g_get_home_dir())+G_DIR_SEPARATOR_S+".gconf/stardict-test/%gconf.xml";
	remove(tmp1.c_str());
	remove(tmp2.c_str());
	std::auto_ptr<config_file> cf;
	cf.reset(new gconf_file("/apps/stardict"));
	tmp_file tf=tmp1;
	tmp_file tf1=tmp2;
	bool res = is_test_passed(cf.get(), false);
	cf.reset(0);
	system("gconftool-2 --shutdown");
	return res;
}
#endif

bool test_inifile()
{
	std::auto_ptr<config_file> cf;
	string fname("/tmp/stardict.cfg");
	tmp_file tf(fname);
	cf.reset(new inifile(fname));
	return is_test_passed(cf.get());
}

int main(int argc, char *argv[])
{
	if (!test_inifile()) {
		std::cerr<<"ini file test failed"<<std::endl;
		return EXIT_FAILURE;
	}

#ifdef CONFIG_GNOME
	gtk_init(&argc, &argv);
	if (!test_gconf()) {
		std::cerr<<"Gconf test failed"<<std::endl;
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}
