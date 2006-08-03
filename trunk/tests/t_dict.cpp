/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2005 Evgeniy <dushistov@mail.ru>
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
#include <ctime>
#include <iterator>
///#include <ios>
#include <iostream>
#include <cstdlib>
#include <sys/resource.h>
#include <vector>

#include "file.hpp"
#include "lib.h"

typedef std::vector<Dict *> dicts_list_t;

class dict_loader {
public:
	dict_loader(dicts_list_t& dl) : dicts_list(dl) {}
	void operator()(const std::string& url, bool) { 
		Dict *d=new Dict;
		if (d->load(url, false, false, UTF8_GENERAL_CI,
			    &Libs::default_show_progress))
			dicts_list.push_back(d);
		else
			delete d;
	}
private:
	dicts_list_t& dicts_list;
};

static inline glong random(glong from, glong to)
{
	return glong(double(rand())/RAND_MAX*(to-from+1)+from);
}

static bool test_dict_lookup_success(Dict *d)
{
	const char too_small[]={0x1, 0x1, 0x1, 0x0};
	const char too_big[]={0xCF, 0xCF, 0xCF, 0x0};
	glong i;
	if (d->Lookup(too_small, i)) {
		std::cerr<<"too_small test failed for: "<<d->dict_name()<<std::endl;
		return false;
	}
	if (d->Lookup(too_big, i)) {
		std::cerr<<"too_big test failed for: "<<d->dict_name()<<std::endl;
		return false;
	}
	std::string first(d->get_key(0)), last(d->get_key(d->narticles()-1));
	//if (!d->Lookup(first.c_str(), i) || i!=0) {
	if (!d->Lookup(first.c_str(), i)) {
		std::cerr<<"first word lookup failed: "<<d->dict_name()<<std::endl;
		return false;
	}
	//if (!d->Lookup(last.c_str(), i) || i!=glong(d->narticles()-1)) {
	if (!d->Lookup(last.c_str(), i)) {
		std::cerr<<"last word lookup failed: "<<d->dict_name()<<std::endl;
		return false;
	}
	srand(time(NULL));
	for (int j=0; j<10000; ++j) {
		glong idx=random(0, d->narticles()-1);
		std::string word(d->get_key(idx));
		//if (!d->Lookup(word.c_str(), i) || i!=idx) {
		if (!d->Lookup(word.c_str(), i)) {
			std::cerr<<"random word lookup failed: "<<d->dict_name()<<std::endl;
			return false;
		}
	}
	return true;
}

int main(int argc, char *argv[])
{
	dicts_list_t dicts;
	List dirs;
	gtk_init(&argc, &argv);
	clock_t t=clock();
#if !defined(_WIN32)
	dirs.push_back("/usr/share/stardict/dic");
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/dic");
#endif
	for_each_file(dirs, ".ifo", List(), List(), dict_loader(dicts));
	t=clock()-t;
	std::cout<<double(t)/CLOCKS_PER_SEC<<std::endl;
	t=clock();
	int ret=EXIT_SUCCESS;
	for (dicts_list_t::iterator it=dicts.begin(); it!=dicts.end(); ++it)
		if (!test_dict_lookup_success(*it)) {
			ret=EXIT_FAILURE;
			break;
		}
	t=clock()-t;
	std::cout<<double(t)/CLOCKS_PER_SEC<<std::endl;
	for (dicts_list_t::iterator it=dicts.begin(); it!=dicts.end(); ++it)
		delete *it;
	return ret;
}
