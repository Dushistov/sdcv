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

#include <cstdlib>
#include <cstring>

#include "conf.h"
#include "utils.h"

#include "skin.h"

void AppSkin::load()
{		
	watch_cursor.reset(gdk_cursor_new(GDK_WATCH));
  std::string pixmaps_dir(gStarDictDataDir+ G_DIR_SEPARATOR_S "pixmaps" G_DIR_SEPARATOR_S);
  std::string filename;
#ifdef _WIN32			
  filename=pixmaps_dir+"stardict.png";
  icon.reset(load_image_from_file(filename));
#else
  icon.reset(load_image_from_file(GNOME_ICONDIR"/stardict.png"));
#ifdef CONFIG_GPE
  filename=pixmaps_dir+"docklet_gpe_normal.png";
#else
  filename=pixmaps_dir+"docklet_normal.png";
#endif
  docklet_normal_icon.reset(load_image_from_file(filename));
#ifdef CONFIG_GPE
  filename=pixmaps_dir+"docklet_gpe_scan.png";
#else
  filename=pixmaps_dir+"docklet_scan.png";
#endif
  docklet_scan_icon.reset(load_image_from_file(filename));
#ifdef CONFIG_GPE
  filename=pixmaps_dir+"docklet_gpe_stop.png";
#else
  filename=pixmaps_dir+"docklet_stop.png";
#endif
  docklet_stop_icon.reset(load_image_from_file(filename));
#endif
  filename=pixmaps_dir+"index_wazard.png";
  index_wazard.reset(load_image_from_file(filename));
  filename=pixmaps_dir+"index_appendix.png";
  index_appendix.reset(load_image_from_file(filename));
  filename=pixmaps_dir+"index_dictlist.png";
  index_dictlist.reset(load_image_from_file(filename));
}
