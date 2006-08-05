#ifndef __SD_SKIN_H__
#define __SD_SKIN_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <map>
#include <string>

#include "utils.h"


typedef ResourceWrapper<GdkCursor, GdkCursor, gdk_cursor_unref> Skin_cursor;
typedef ResourceWrapper<GdkPixbuf, void, g_object_unref> Skin_pixbuf_1;

class AppSkin {
public:
	int width,height;
	Skin_cursor normal_cursor;
	Skin_cursor watch_cursor;
	Skin_pixbuf_1 icon;

	Skin_pixbuf_1 index_wazard;
	Skin_pixbuf_1 index_appendix;
	Skin_pixbuf_1 index_dictlist;

	~AppSkin();
	GdkPixbuf *get_image(const std::string& name) const;
	void load();
private:
	typedef std::map<std::string, GdkPixbuf *> ImageMap;
	ImageMap image_map_;
};

#endif
