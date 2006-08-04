#ifndef __SD_SKIN_H__
#define __SD_SKIN_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "utils.h"


typedef ResourceWrapper<GdkCursor, GdkCursor, gdk_cursor_unref> Skin_cursor;
typedef ResourceWrapper<GdkPixbuf, void, g_object_unref> Skin_pixbuf_1;

struct AppSkin {
	int width,height;
	Skin_cursor normal_cursor;
	Skin_cursor watch_cursor;
	Skin_pixbuf_1 icon;
#ifndef _WIN32
	Skin_pixbuf_1 docklet_normal_icon;
	Skin_pixbuf_1 docklet_scan_icon;
	Skin_pixbuf_1 docklet_stop_icon;
#endif
	Skin_pixbuf_1 index_wazard;
	Skin_pixbuf_1 index_appendix;
	Skin_pixbuf_1 index_dictlist;

	void load();
};

#endif
