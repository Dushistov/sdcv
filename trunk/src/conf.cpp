/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
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

#include <glib/gi18n.h>
#include "class_factory.hpp"
#include "utils.h"

#include "conf.h"

#ifdef CONFIG_GPE
const int DEFAULT_WINDOW_WIDTH=238;
const int DEFAULT_WINDOW_HEIGHT=279;
const int DEFAULT_HPANED_POS=79;
#else
const int DEFAULT_WINDOW_WIDTH=463;
const int DEFAULT_WINDOW_HEIGHT=321;
const int DEFAULT_HPANED_POS=127;
#endif

std::auto_ptr<AppConf> conf;
std::string gStarDictDataDir;

//---------------------------------------------------------------------------------
AppConf::AppConf() : 
	cf_(static_cast<config_file *>(stardict_class_factory::create_class_by_name("config_file")))
{
	add_entry_at("main_window/maximized", false);
#ifdef _WIN32
	add_entry_at("dictionary/use_custom_font", get_win32_use_custom_font());
#else
	add_entry_at("dictionary/use_custom_font", false);
#endif
	add_entry_at("main_window/hide_on_startup", false);
	add_entry_at("main_window/search_while_typing", true);
	add_entry_at("main_window/showfirst_when_notfound", true);
	add_entry_at("dictionary/enable_sound_event", true);
	add_entry_at("main_window/hide_list", false);
	add_entry_at("dictionary/scan_selection", true);
#ifdef _WIN32
	add_entry_at("dictionary/scan_clipboard", false);
	add_entry_at("dictionary/use_scan_hotkey", true);
	add_entry_at("dictionary/use_mainwindow_hotkey", true);
#endif
	add_entry_at("notification_area_icon/query_in_floatwin", true);
	add_entry_at("dictionary/only_scan_while_modifier_key", false);
	add_entry_at("dictionary/hide_floatwin_when_modifier_key_released", true);
	add_entry_at("floating_window/pronounce_when_popup", false);
	add_entry_at("floating_window/lock", false);
	add_entry_at("floating_window/show_if_not_found", true);

	add_entry_at("floating_window/lock_x", 0);
	add_entry_at("floating_window/lock_y", 0);
	add_entry_at("dictionary/scan_modifier_key", 0);
	add_entry_at("main_window/hpaned_pos", DEFAULT_HPANED_POS);
	add_entry_at("main_window/window_width", DEFAULT_WINDOW_WIDTH);
	add_entry_at("main_window/window_height", DEFAULT_WINDOW_HEIGHT);
	add_entry_at("floating_window/max_window_width", DEFAULT_MAX_FLOATWIN_WIDTH);
	add_entry_at("floating_window/max_window_height", DEFAULT_MAX_FLOATWIN_HEIGHT);

#ifdef _WIN32
	add_entry_at("dictionary/custom_font", get_win32_custom_font());
#else
	add_entry_at("dictionary/custom_font", std::string());
#endif

	add_entry_at("dictionary/create_cache_file", true);
	add_entry_at("dictionary/enable_collation", false);
	add_entry_at("dictionary/collate_function", 0);

#if defined(CONFIG_GTK) || defined (CONFIG_GPE)
	add_entry_at("dictionary/play_command", std::string("play"));
#endif
#ifdef _WIN32
	add_entry_at("dictionary/tts_path", std::string("C:\\Program Files\\WyabdcRealPeopleTTS\nC:\\Program Files\\OtdRealPeopleTTS\nWyabdcRealPeopleTTS\nOtdRealPeopleTTS"));
#else
	add_entry_at("dictionary/tts_path", std::string("/usr/share/WyabdcRealPeopleTTS\n/usr/share/OtdRealPeopleTTS"));
#endif
	add_entry_at("dictionary/history", get_default_history_filename());
	add_entry_at("dictionary/only_export_word", true);
	add_entry_at("dictionary/export_file", get_default_export_filename());
	
	add_entry_at("main_window/search_website_list", std::list<std::string>());

	add_entry("/apps/stardict/manage_dictionaries/treedict_order_list",
		  std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/treedict_disable_list",
		  std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/dict_order_list",
		  std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/dict_disable_list",
		  std::list<std::string>());

	std::list<std::string> dirs;
	dirs.push_back(gStarDictDataDir+ G_DIR_SEPARATOR_S "dic");
#ifndef _WIN32
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/dic");
#endif
	add_entry("/apps/stardict/manage_dictionaries/dict_dirs_list",
		  dirs);

	dirs.clear();
	dirs.push_back(gStarDictDataDir+ G_DIR_SEPARATOR_S "treedict");
#ifndef _WIN32
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/treedict");
#endif
	add_entry("/apps/stardict/manage_dictionaries/treedict_dirs_list",
		  dirs);
	
	load();
}
//---------------------------------------------------------------------------------
AppConf::~AppConf()
{
	for (cache_t::iterator it = cache_.begin(); it != cache_.end(); ++it)
		delete it->second;
}
//---------------------------------------------------------------------------------
static std::pair<std::string, std::string> split(const std::string& s) 
{
	std::string::size_type pos=s.rfind("/");
	std::pair<std::string, std::string> res;
	if (pos!=std::string::npos)
		res.second = s.substr(pos+1);
	else
		pos=s.length();

	res.first=s.substr(0, pos);
	
	return res;
}
//---------------------------------------------------------------------------------
void AppConf::save_value(const std::string& name, const baseconfval* cv) 
{
	std::pair<std::string, std::string> split_name = split(name);
	switch (cv->type) {
	case baseconfval::BOOL_TYPE:
		cf_->write_bool(split_name.first.c_str(), split_name.second.c_str(), 
				static_cast<const confval<bool> *>(cv)->val);
		break;
	case baseconfval::INT_TYPE:
		cf_->write_int(split_name.first.c_str(), split_name.second.c_str(), 
			       static_cast<const confval<int> *>(cv)->val);
		break;
	case baseconfval::STRING_TYPE:
		cf_->write_string(split_name.first.c_str(), split_name.second.c_str(), 
				  static_cast<const confval<std::string> *>(cv)->val);
		break;
	case baseconfval::STRLIST_TYPE:
		cf_->write_strlist(split_name.first.c_str(), split_name.second.c_str(), 
				   static_cast<const confval<std::list<std::string> > *>(cv)->val);
		break;
	default:
		/*nothing*/;
	}
}
//---------------------------------------------------------------------------------
void AppConf::notify_add(const std::string& name,
			 const sigc::slot<void, const baseconfval*>& slot)
{
	std::pair<std::string, std::string> split_name = split(name);
	cf_->notify_add(split_name.first.c_str(), split_name.second.c_str(), slot);
}
//---------------------------------------------------------------------------------
//load preference
void AppConf::load()
{	
	for (cache_t::iterator p = cache_.begin(); p != cache_.end(); ++p) {
		std::pair<std::string, std::string> name = split(p->first);
		switch (p->second->type) {
		case baseconfval::BOOL_TYPE:
			cf_->read_bool(name.first.c_str(), name.second.c_str(),
				      static_cast<confval<bool> *>(p->second)->val);
			break;
		case baseconfval::INT_TYPE:
			cf_->read_int(name.first.c_str(), name.second.c_str(),
				     static_cast<confval<int> *>(p->second)->val);
			break;
		case baseconfval::STRING_TYPE:
			cf_->read_string(name.first.c_str(), name.second.c_str(),
					static_cast<confval<std::string> *>(p->second)->val);
			break;
		case baseconfval::STRLIST_TYPE:
			cf_->read_strlist(name.first.c_str(), name.second.c_str(),
					 static_cast<confval<std::list<std::string> > *>(p->second)->val);
			break;
		default:
			/*nothing*/;
		}
	}
	const std::list<std::string> &list =
		get_strlist("/apps/stardict/preferences/main_window/search_website_list");
	if (list.empty()) {
		cache_t::iterator p =
			cache_.find("/apps/stardict/preferences/main_window/search_website_list");
		static_cast<confval< std::list<std::string> > *>(p->second)->val =
			get_default_search_website_list();
	}
}
//---------------------------------------------------------------------------------
std::string AppConf::get_default_history_filename()
{
	std::string histname;
#ifdef _WIN32
	histname = get_user_config_dir() + G_DIR_SEPARATOR_S "history.txt";
#else
	histname = get_user_config_dir() + G_DIR_SEPARATOR_S "history";
#endif

  return histname;

}

std::string AppConf::get_default_export_filename()
{
	std::string exportname;
#ifdef _WIN32
	exportname = gStarDictDataDir + G_DIR_SEPARATOR_S "dic.txt";
#else
	exportname = g_get_home_dir();
	exportname+= G_DIR_SEPARATOR_S "dic.txt";
#endif
	return exportname;
}

std::list<std::string> AppConf::get_default_search_website_list()
{
	gchar *default_website = _(
		"Dictionary.com	http://dictionary.reference.com	http://dictionary.reference.com/search?q=%s\n"
		"dict.leo.org	http://dict.leo.org	http://dict.leo.org/?search=%s&lang=en\n"
		"H2G2	http://www.h2g2.com	http://www.h2g2.com/Search?searchstring=%s&searchtype=ARTICLE&skip=0&show=20&showapproved=1&shownormal=1&showsubmitted=1\n"
		"WhatIs	http://whatis.techtarget.com	http://whatis.techtarget.com/wsearchResults/1,290214,sid9,00.html?query=%s\n"
		"Altavista	http://www.altavista.com	http://www.altavista.com/cgi-bin/query?q=%s\n"
		"WEB.DE	http://suche.web.de	http://suche.web.de/search/?su=%s\n"
		"WebCrawler	http://www.webcrawler.com	http://www.webcrawler.com/cgi-bin/WebQuery?searchText=%s\n"
		"Google	http://www.google.com	http://www.google.com/search?q=%s\n"
		"Yahoo	http://search.yahoo.com	http://search.yahoo.com/bin/search?p=%s\n"
		"CMU	http://www.speech.cs.cmu.edu	http://www.speech.cs.cmu.edu/cgi-bin/cmudict?in=%s\n"	
		);
	gchar *p = default_website;
	gchar *p1;
	std::list<std::string> list;
	while ((p1 = strchr(p, '\n'))!= NULL) {
		list.push_back(std::string(p, p1-p));
		p= p1+1;
	}
	return list;
}

#ifdef _WIN32
bool AppConf::get_win32_use_custom_font()
{
	gchar *ch = _("win32_use_custom_font=0");
	gchar *s = strstr(ch, "win32_use_custom_font=");
	if (s) {
		if (*(s+ sizeof("win32_use_custom_font=")-1)=='1')
			return true;
	}
	return false;
}

std::string AppConf::get_win32_custom_font()
{
	gchar *ch = _("win32_custom_font=");
	gchar *s = strstr(ch, "win32_custom_font=");
	if (s) {
		return (s+ sizeof("win32_custom_font=")-1);
	}
	return "";
}
#endif
//---------------------------------------------------------------------------------
