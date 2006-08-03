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

#ifdef CONFIG_GNOME
#  include "gconf_file.hpp"
#else
#  include "inifile.hpp"
#endif
#ifdef _WIN32
#  include "win32/win32_iskeyspressed.h"
#else
#  include "x11_iskeyspressed.hpp"
#endif
#include "utils.h"

#include "class_factory.hpp"

void *stardict_class_factory::create_class_by_name(const std::string& name, void *param)
{
	if (name=="config_file") {
#ifdef CONFIG_GNOME
		return new gconf_file("/apps/stardict");
#else
		return new inifile(get_user_config_dir()+ G_DIR_SEPARATOR_S "stardict.cfg");
#endif
	} else if (name=="hotkeys") {
#ifdef _WIN32
		return new win32_hotkeys();
#else
		return new x11_hotkeys(GTK_WINDOW(param));
#endif
	}
	return NULL;
}

