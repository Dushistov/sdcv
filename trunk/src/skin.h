#ifndef __SD_SKIN_H__
#define __SD_SKIN_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

template <typename T, typename unref_res_t, void (*unref_res)(unref_res_t *)>
struct resource_wrapper {
	resource_wrapper() : p(NULL) {}
	~resource_wrapper() { free_resource(); }
	T *get() { return p; }
	void reset(T *newp) {
		if (p==newp)		 
			return;
		free_resource();
		p=newp;
	}
private:
	T *p;

	void free_resource() { if (p) unref_res(p); }
};


typedef resource_wrapper<GdkCursor, GdkCursor, gdk_cursor_unref> Skin_cursor;
typedef resource_wrapper<GdkPixbuf, void, g_object_unref> Skin_pixbuf_1;

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
