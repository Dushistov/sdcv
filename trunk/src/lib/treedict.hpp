#ifndef _TREEDICT_HPP_
#define _TREEDICT_HPP_

#include <gtk/gtk.h>

#include "common.hpp"
#include "data.hpp"

class TreeDict : public DictBase {
public:
	TreeDict();
	bool load(const std::string& ifofilename);
	static GtkTreeStore *get_model() { return model; }
private:
	static GtkTreeStore *model;

	bool load_ifofile(const std::string& ifofilename, gulong *tdxfilesize);
	void load_model(gchar **buffer, GtkTreeIter *parent, guint32 count);
};

class TreeDicts {
public:
	TreeDicts();
	~TreeDicts();
	void load_dict(const std::string& url);
	GtkTreeStore* Load(const strlist_t& tree_dicts_dirs,
			   const strlist_t& order_list,
			   const strlist_t& disable_list);
	gchar * poGetWordData(guint32 offset, guint32 size, int iTreeDict);
private:
	std::vector<TreeDict *> oTreeDict;
};

#endif//!_TREEDICT_HPP_
