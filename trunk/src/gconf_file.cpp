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

#include <glib/gi18n.h>

#include "gconf_file.hpp"

gconf_file::gconf_file(const std::string& path)
{
  cfgname=path;
  if ((gconf_client = gconf_client_get_default())==NULL)
    g_warning("Cannot connect to gconf.\n");
  else
    gconf_client_add_dir(gconf_client, cfgname.c_str(), GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

gconf_file::~gconf_file()
{
	for (std::vector<guint>::iterator it=notification_ids.begin();
			 it!=notification_ids.end(); ++it)
		gconf_client_notify_remove(gconf_client, *it);

	if (!gconf_client)
		return;
  gconf_client_remove_dir(gconf_client, cfgname.c_str(), NULL);
  g_object_unref(gconf_client);
}

bool gconf_file::read_bool(const gchar *sect, const gchar *key, bool& val)
{
  if (!gconf_client)
    return false;
  
	std::string real_key(std::string(sect)+"/"+key);

	GConfValue *gval=gconf_client_get(gconf_client, real_key.c_str(), NULL);
	if (!gval)
		return false;
	val=gconf_value_get_bool(gval);
	gconf_value_free(gval);

	return true;
}

bool gconf_file::read_int(const gchar *sect, const gchar *key, int& val)
{
	if (!gconf_client)
    return false;
	std::string real_key(std::string(sect)+"/"+key);
	GConfValue *gval=gconf_client_get(gconf_client, real_key.c_str(), NULL);
	if (!gval)
		return false;
	val=gconf_value_get_int(gval);
	gconf_value_free(gval);

	return true;
}

bool gconf_file::read_string(const gchar * sect, const gchar *key, std::string& val)
{
	if (!gconf_client)
    return false;

	std::string real_key(std::string(sect)+"/"+key);
  gchar *gconf_val = gconf_client_get_string(gconf_client, real_key.c_str(), NULL);
	if (gconf_val!=NULL) 
		val=gconf_val;

	g_free(gconf_val); 

	return true;
}

bool gconf_file::read_strlist(const gchar * sect, const gchar * key, std::list<std::string>& slist)
{
	if (!gconf_client)
    return false;  

	std::string real_key(std::string(sect)+"/"+key);
	GSList  *gslist = gconf_client_get_list(gconf_client, real_key.c_str(), GCONF_VALUE_STRING, NULL);
	if (!gslist)
		return false;

	slist.clear();
	GSList *p = gslist; 
	while (p) {
		slist.push_back(static_cast<char *>(p->data));
		g_free(p->data);		
		p=g_slist_next(p);
	}
	g_slist_free(gslist);

	return true;
}

void gconf_file::write_bool(const gchar *sect, const gchar *key, bool val)
{
if (!gconf_client)
    return;
  gchar *real_key=g_strdup_printf("%s/%s", sect, key);
  gconf_client_set_bool(gconf_client, real_key, val, NULL);
  g_free(real_key);
}

void gconf_file::write_int(const gchar *sect, const gchar *key, int val)
{
	if (!gconf_client)
		return;
  gchar *real_key=g_strdup_printf("%s/%s", sect, key);
  gconf_client_set_int(gconf_client, real_key, val, NULL);
  g_free(real_key);
}

void gconf_file::write_string(const gchar *sect, const gchar *key, const std::string& val)
{
	if(!gconf_client)
    return;
  gchar *real_key=g_strdup_printf("%s/%s", sect, key);
  gconf_client_set_string(gconf_client, real_key, val.c_str(), NULL);
  g_free(real_key);
}

void gconf_file::write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist)
{
	if (!gconf_client)
		return;

 GSList *gslist = NULL;
 for (std::list<std::string>::const_iterator p=slist.begin(); p!=slist.end(); ++p)
	 gslist = g_slist_append(gslist, const_cast<char *>(p->c_str()));
 
 gchar *real_key=g_strdup_printf("%s/%s", sect, key);
 gconf_client_set_list(gconf_client, real_key, GCONF_VALUE_STRING, gslist, NULL);
 g_free(real_key);
 g_slist_free(gslist);
}

static void gconf_client_notify_func(GConfClient *client, guint cnxn_id,
																		 GConfEntry *entry, gpointer user_data)
{
	change_handler<const baseconfval *> *ch=static_cast<change_handler<const baseconfval *> *>(user_data);
	std::auto_ptr<baseconfval> cv;
	switch (entry->value->type) {
	case GCONF_VALUE_BOOL:
		cv.reset(new confval<bool>);
		static_cast<confval<bool> *>(cv.get())->val=gconf_value_get_bool(entry->value);
		break;
	case GCONF_VALUE_INT:
		cv.reset(new confval<int>);
		static_cast<confval<int> *>(cv.get())->val=gconf_value_get_int(entry->value);
		break;
	case GCONF_VALUE_STRING: {
		cv.reset(new confval<std::string>);
		const gchar *gconf_val=gconf_value_get_string(entry->value);
		if (gconf_val)
			static_cast<confval<std::string> *>(cv.get())->val=gconf_val;		
	}
	case GCONF_VALUE_LIST: {
		cv.reset(new confval<std::list<std::string> >);
		GSList *gslist=gconf_value_get_list(entry->value);
		
		GSList *p = gslist; 
		while (p) {
			static_cast< confval<std::list<std::string> > *>(cv.get())->val.push_back(static_cast<char *>(p->data));
			p=g_slist_next(p);
		}
		
	}
		break;
	default:
		return;
	}
	(*ch)(cv.get());
}

static void gfree_func(gpointer data)
{
	change_handler<baseconfval> *bcv=static_cast<change_handler<baseconfval> *>(data);
	delete bcv;
}

void gconf_file::notify_add(const gchar *sect, const gchar *key, 
														void (*on_change)(const baseconfval*, void *), void *arg)
{
	std::string name(std::string(sect)+"/"+key);
	change_handler<const baseconfval *> *ch=new change_handler<const baseconfval*>;
	ch->on_change=on_change;
	ch->arg=arg;
	guint id=gconf_client_notify_add(gconf_client, name.c_str(), 
																	 gconf_client_notify_func, ch, gfree_func, NULL);
	notification_ids.push_back(id);
}
