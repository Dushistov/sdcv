#ifndef _STDDICT_HPP_
#define _STDDICT_HPP_

#include <glib.h>
#include <string>
#include <vector>
#include <list>

#include "data.hpp"
#include "collation.h"

const int MAX_FUZZY_DISTANCE= 3; // at most MAX_FUZZY_DISTANCE-1 differences allowed when find similar words
const int MAX_MATCH_ITEM_PER_LIB=100;

extern gint stardict_casecmp(const gchar *s1, const gchar *s2, bool isClt, CollateFunctions func);
extern bool bIsPureEnglish(const gchar *str);

class show_progress_t {
public:
	virtual ~show_progress_t() {}
	virtual void notify_about_start(const std::string& title) {}
	virtual void notify_about_work() {}
};

class cache_file {
public:
	guint32 *wordoffset;

	cache_file(bool _isoftfile);
	~cache_file();
	bool load_cache(const std::string& url, const std::string& saveurl, CollateFunctions cltfunc, glong filedatasize);
	bool save_cache(const std::string& url, CollateFunctions cltfunc, gulong npages);
private:
	bool isoftfile;
	MapFile *mf;
	bool get_cache_filename(const std::string& url, std::string &cachefilename, bool create);
	MapFile* get_cache_loadfile(const gchar *filename, const std::string &url, const std::string &saveurl, CollateFunctions cltfunc, glong filedatasize, int next);
	FILE* get_cache_savefile(const gchar *filename, const std::string &url, int next, std::string &cfilename);
};

class idxsyn_file;
class collation_file : public cache_file {
public:
	CollateFunctions CollateFunction;

	collation_file(idxsyn_file *_idx_file);
	bool lookup(const char *str, glong &idx);
	const gchar *GetWord(glong idx);
	glong GetOrigIndex(glong cltidx);
private:
	idxsyn_file *idx_file;
};

class idxsyn_file {
public:
	glong wordcount;
	collation_file *clt_file;

	virtual const gchar *get_key(glong idx) = 0;
	virtual ~idxsyn_file() {}
	void collate_sort(const std::string& url, const std::string& saveurl,
			  CollateFunctions collf, show_progress_t *sp);
};

class index_file : public idxsyn_file {
public:
	guint32 wordentry_offset;
	guint32 wordentry_size;

	virtual bool load(const std::string& url, gulong wc, gulong fsize,
			  bool CreateCacheFile, bool EnableCollation,
			  CollateFunctions _CollateFunction, show_progress_t *sp) = 0;
	virtual void get_data(glong idx) = 0;
	virtual  const gchar *get_key_and_data(glong idx) = 0;
	virtual bool lookup(const char *str, glong &idx) = 0;
};

class synonym_file : public idxsyn_file {
public:
	guint32 wordentry_index;

	synonym_file();
	~synonym_file();
	bool load(const std::string& url, gulong wc, bool CreateCacheFile,
		  bool EnableCollation, CollateFunctions _CollateFunction,
		  show_progress_t *sp);
	const gchar *get_key(glong idx);
	bool lookup(const char *str, glong &idx);
private:
	static const gint ENTR_PER_PAGE=32;
	gulong npages;

	cache_file oft_file;
	FILE *synfile;

	gchar wordentry_buf[256+sizeof(guint32)];
	struct index_entry {
		glong idx;
		std::string keystr;
		void assign(glong i, const std::string& str) {
			idx=i;
			keystr.assign(str);
		}
	};
	index_entry first, last, middle, real_last;

	struct page_entry {
		gchar *keystr;
		guint32 index;
	};
	std::vector<gchar> page_data;
	struct page_t {
		glong idx;
		page_entry entries[ENTR_PER_PAGE];

		page_t(): idx(-1) {}
		void fill(gchar *data, gint nent, glong idx_);
	} page;
	gulong load_page(glong page_idx);
	const gchar *read_first_on_page_key(glong page_idx);
	const gchar *get_first_on_page_key(glong page_idx);
};

class Dict : public DictBase {
private:
	std::string ifo_file_name;
	std::string bookname;

	bool load_ifofile(const std::string& ifofilename, gulong &idxfilesize, glong &wordcount, glong &synwordcount);
public:
	std::auto_ptr<index_file> idx_file;
	std::auto_ptr<synonym_file> syn_file;

	Dict() {}
	bool load(const std::string&, bool, bool, CollateFunctions,
		  show_progress_t *);

	glong narticles() { return idx_file->wordcount; }
	glong nsynarticles();
	const std::string& dict_name() { return bookname; }
	const std::string& ifofilename() { return ifo_file_name; }

	const gchar *get_key(glong index)	{return idx_file->get_key(index);}
	const gchar *GetCltWord(glong index)       {return idx_file->clt_file->GetWord(index);}
	gchar *get_data(glong index)
	{
		idx_file->get_data(index);
		return DictBase::GetWordData(idx_file->wordentry_offset, idx_file->wordentry_size);
	}
	void get_key_and_data(glong index, const gchar **key, guint32 *offset, guint32 *size)
	{
		*key = idx_file->get_key_and_data(index);
		*offset = idx_file->wordentry_offset;
		*size = idx_file->wordentry_size;
	}
	bool Lookup(const char *str, glong &idx)
	{
		return idx_file->lookup(str, idx);
	}
	bool LookupClt(const char *str, glong &idx)
	{
		return idx_file->clt_file->lookup(str, idx);
	}
	bool Lookup2(const char *str, glong &idx, bool isClt)
	{
		if (isClt)
			return idx_file->clt_file->lookup(str, idx);
		else
			return idx_file->lookup(str, idx);
	}
	bool LookupWithRule(GPatternSpec *pspec, glong *aIndex, int iBuffLen);
	bool LookupWithRuleSynonym(GPatternSpec *pspec, glong *aIndex, int iBuffLen);
	bool LookupSynonym(const char *str, glong &synidx);
	bool LookupCltSynonym(const char *str, glong &synidx);
	gint GetWordCount(glong& iWordIndex, bool isidx);
	bool GetWordPrev(glong iWordIndex, glong &pidx, bool isidx, bool isClt);
	void GetWordNext(glong &iWordIndex, bool isidx, bool isClt);
};

struct CurrentIndex {
	glong idx;
	glong synidx;
};

class Libs {
public:
	static show_progress_t default_show_progress;

	Libs(show_progress_t *sp, bool create, bool enable, int function);
	~Libs();
	void set_show_progress(show_progress_t *sp) {
		if (sp)
			show_progress = sp;
		else
			show_progress = &default_show_progress;
	}
	bool enable_coll() { return EnableCollation; }
	void load_dict(const std::string& url, show_progress_t *sp);
	void load(const strlist_t& dicts_dirs, const strlist_t& order_list, const strlist_t& disable_list);
	void reload(const strlist_t& dicts_dirs, const strlist_t& order_list,
		    const strlist_t& disable_list, bool is_coll_enb, int collf);

	glong narticles(int idict) { return oLib[idict]->narticles(); }
	glong nsynarticles(int idict) { return oLib[idict]->nsynarticles(); }
	const std::string& dict_name(int idict) { return oLib[idict]->dict_name(); }
	std::size_t ndicts() { return oLib.size(); }

	const gchar * poGetWord(glong iIndex,int iLib) {
		return oLib[iLib]->get_key(iIndex);
	}
	const gchar * poGetCltWord(glong iIndex,int iLib) {
		return oLib[iLib]->GetCltWord(iIndex);
	}
	const gchar * poGetSynonymWord(glong iSynonymIndex,int iLib) {
		return oLib[iLib]->syn_file->get_key(iSynonymIndex);
	}
	const gchar * poGetCltSynonymWord(glong iSynonymIndex,int iLib) {
		return oLib[iLib]->syn_file->clt_file->GetWord(iSynonymIndex);
	}
	glong poGetSynonymWordIdx(glong iSynonymIndex, int iLib) {
		oLib[iLib]->syn_file->get_key(iSynonymIndex);
		return oLib[iLib]->syn_file->wordentry_index;
	}
	glong CltIndexToOrig(glong cltidx, int iLib) {
		if (cltidx == INVALID_INDEX)
			return cltidx;
		return oLib[iLib]->idx_file->clt_file->GetOrigIndex(cltidx);
	}
	glong CltSynIndexToOrig(glong cltidx, int iLib) {
		if (cltidx == UNSET_INDEX || cltidx == INVALID_INDEX)
			return cltidx;
		return oLib[iLib]->syn_file->clt_file->GetOrigIndex(cltidx);
	}
	gchar * poGetWordData(glong iIndex,int iLib) {
		if (iIndex==INVALID_INDEX)
			return NULL;
		return oLib[iLib]->get_data(iIndex);
	}
	const gchar *poGetCurrentWord(CurrentIndex *iCurrent);
	const gchar *poGetNextWord(const gchar *word, CurrentIndex *iCurrent);
	const gchar *poGetPreWord(const gchar *word, CurrentIndex *iCurrent);
	bool LookupWord(const gchar* sWord, glong& iWordIndex, int iLib) {
		return oLib[iLib]->Lookup(sWord, iWordIndex);
	}
	bool LookupCltWord(const gchar* sWord, glong& iWordIndex, int iLib) {
		return oLib[iLib]->LookupClt(sWord, iWordIndex);
	}
	bool LookupSynonymWord(const gchar* sWord, glong& iSynonymIndex, int iLib) {
		return oLib[iLib]->LookupSynonym(sWord, iSynonymIndex);
	}
	bool LookupCltSynonymWord(const gchar* sWord, glong& iSynonymIndex, int iLib) {
		return oLib[iLib]->LookupCltSynonym(sWord, iSynonymIndex);
	}
	bool LookupSimilarWord(const gchar* sWord, glong &iWordIndex, int iLib);
	bool LookupSynonymSimilarWord(const gchar* sWord, glong &iSynonymWordIndex, int iLib);
	bool SimpleLookupWord(const gchar* sWord, glong &iWordIndex, int iLib);
	bool SimpleLookupSynonymWord(const gchar* sWord, glong &iWordIndex, int iLib);
	gint GetWordCount(glong& iWordIndex, int iLib, bool isidx) {
		return oLib[iLib]->GetWordCount(iWordIndex, isidx);
	}
	bool GetWordPrev(glong iWordIndex, glong &pidx, int iLib, bool isidx, bool isClt) {
		return oLib[iLib]->GetWordPrev(iWordIndex, pidx, isidx, isClt);
	}
	void GetWordNext(glong &iWordIndex, int iLib, bool isidx, bool isClt) {
		oLib[iLib]->GetWordNext(iWordIndex, isidx, isClt);
	}

	bool LookupWithFuzzy(const gchar *sWord, gchar *reslist[], gint reslist_size);
	gint LookupWithRule(const gchar *sWord, gchar *reslist[]);

	typedef void (*updateSearchDialog_func)(gpointer data, gdouble fraction);
	bool LookupData(const gchar *sWord, std::vector<gchar *> *reslist, updateSearchDialog_func func, gpointer data, bool *cancel);
private:
	std::vector<Dict *> oLib; // word Libs.
	int iMaxFuzzyDistance;
	show_progress_t *show_progress;
	bool CreateCacheFile;
	bool EnableCollation;
	CollateFunctions CollateFunction;

	friend class DictLoader;
	friend class DictReLoader;
};


#endif//!_STDDICT_HPP_
