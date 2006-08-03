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

/*
 * Implementation of class to work with standard StarDict's dictionaries
 * lookup word, get articles and so on.
 *
 * Notice: read doc/DICTFILE_FORMAT for the dictionary
 * file's format information!
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "common.hpp"
#include "distance.h"
#include "file.hpp"
#include "kmp.h"
#include "mapfile.hpp"

#include "stddict.hpp"

static inline gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
  gint a=g_ascii_strcasecmp(s1, s2);
  if (a == 0)
    return strcmp(s1, s2);
  else
    return a;
}

static gint stardict_collate(const gchar *str1, const gchar *str2, CollateFunctions func)
{
	gint x = utf8_collate(str1, str2, func);
	if (x == 0)
		return strcmp(str1, str2);
	else
		return x;
}

static inline bool bIsVowel(gchar inputchar)
{
  gchar ch = g_ascii_toupper(inputchar);
  return( ch=='A' || ch=='E' || ch=='I' || ch=='O' || ch=='U' );
}


bool bIsPureEnglish(const gchar *str)
{
  // i think this should work even when it is UTF8 string :).
  for (int i=0; str[i]!=0; i++)
    //if(str[i]<0)
    //if(str[i]<32 || str[i]>126) // tab equal 9,so this is not OK.
    // Better use isascii() but not str[i]<0 while char is default unsigned in arm
    if (!isascii(str[i]))
            return false;
  return true;
}

gint stardict_casecmp(const gchar *s1, const gchar *s2, bool isClt, CollateFunctions func)
{
	if (isClt)
		return utf8_collate(s1, s2, func);
	else
		return g_ascii_strcasecmp(s1, s2);
}

class offset_index : public index_file {
public:
	offset_index();
	~offset_index();
	bool load(const std::string& url, gulong wc, gulong fsize,
		  bool CreateCacheFile, bool EnableCollation,
		  CollateFunctions _CollateFunction, show_progress_t *sp);
	const gchar *get_key(glong idx);
	void get_data(glong idx);
	const gchar *get_key_and_data(glong idx);
	bool lookup(const char *str, glong &idx);
private:
	static const gint ENTR_PER_PAGE=32;

	cache_file oft_file;
	FILE *idxfile;
	gulong npages;

	gchar wordentry_buf[256+sizeof(guint32)*2]; // The length of "word_str" should be less than 256. See doc/DICTFILE_FORMAT.
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
		guint32 off, size;
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

class wordlist_index : public index_file {
public:
	wordlist_index();
	~wordlist_index();
	bool load(const std::string& url, gulong wc, gulong fsize,
		  bool CreateCacheFile, bool EnableCollation,
		  CollateFunctions _CollateFunction, show_progress_t *sp);
	const gchar *get_key(glong idx);
	void get_data(glong idx);
	const gchar *get_key_and_data(glong idx);
	bool lookup(const char *str, glong &idx);
private:
	gchar *idxdatabuf;
	std::vector<gchar *> wordlist;
};

offset_index::offset_index() : oft_file(true)
{
	clt_file = NULL;
	idxfile = NULL;
}

offset_index::~offset_index()
{
	delete clt_file;
	if (idxfile)
		fclose(idxfile);
}

void offset_index::page_t::fill(gchar *data, gint nent, glong idx_)
{
	idx=idx_;
	gchar *p=data;
	glong len;
	for (gint i=0; i<nent; ++i) {
		entries[i].keystr=p;
		len=strlen(p);
		p+=len+1;
		entries[i].off=g_ntohl(*reinterpret_cast<guint32 *>(p));
		p+=sizeof(guint32);
		entries[i].size=g_ntohl(*reinterpret_cast<guint32 *>(p));
		p+=sizeof(guint32);
	}
}

inline const gchar *offset_index::read_first_on_page_key(glong page_idx)
{
	fseek(idxfile, oft_file.wordoffset[page_idx], SEEK_SET);
	guint32 page_size=oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx];
	gulong minsize = sizeof(wordentry_buf);
	if (page_size < minsize)
		minsize = page_size;
	fread(wordentry_buf, minsize, 1, idxfile); //TODO: check returned values, deal with word entry that strlen>255.
	return wordentry_buf;
}

inline const gchar *offset_index::get_first_on_page_key(glong page_idx)
{
	if (page_idx<middle.idx) {
		if (page_idx==first.idx)
			return first.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else if (page_idx>middle.idx) {
		if (page_idx==last.idx)
			return last.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else
		return middle.keystr.c_str();
}

cache_file::cache_file(bool _isoftfile)
{
	wordoffset = NULL;
	mf = NULL;
	isoftfile = _isoftfile;
}


cache_file::~cache_file()
{
	if (mf)
		delete mf;
	else
		g_free(wordoffset);
}

#define OFFSETFILE_MAGIC_DATA "StarDict's oft file\nversion=2.4.8\n"
#define COLLATIONFILE_MAGIC_DATA "StarDict's clt file\nversion=2.4.8\n"

MapFile* cache_file::get_cache_loadfile(const gchar *filename, const std::string &url, const std::string &saveurl, CollateFunctions cltfunc, glong filedatasize, int next)
{
	struct stat cachestat;
	if (g_stat(filename, &cachestat)!=0)
		return NULL;
	MapFile *mf = new MapFile;
	if (!mf->open(filename, cachestat.st_size)) {
		delete mf;
		return NULL;
	}

	gchar *p = mf->begin();
	bool has_prefix;
	if (isoftfile)
		has_prefix = g_str_has_prefix(p, OFFSETFILE_MAGIC_DATA);
	else
		has_prefix = g_str_has_prefix(p, COLLATIONFILE_MAGIC_DATA);
	if (!has_prefix) {
		delete mf;
		return NULL;
	}
	if (isoftfile)
		p+= sizeof(OFFSETFILE_MAGIC_DATA)-1-1;
	else
		p+= sizeof(COLLATIONFILE_MAGIC_DATA)-1-1;
	gchar *p2;
	p2 = strstr(p, "\nurl=");
	if (!p2) {
		delete mf;
		return NULL;
	}
	p2+=sizeof("\nurl=")-1;
	gchar *p3;
	p3 = strchr(p2, '\n');
	if (!p3) {
		delete mf;
		return NULL;
	}
	gchar *tmpstr;
	tmpstr = (gchar *)g_memdup(p2, p3-p2+1);
	tmpstr[p3-p2] = '\0';
	if (saveurl == tmpstr) {
		g_free(tmpstr);
		if (!isoftfile) {
			p2 = strstr(p, "\nfunc=");
			if (!p2) {
				delete mf;
				return NULL;
			}
			p2+=sizeof("\nfunc=")-1;
			p3 = strchr(p2, '\n');
			if (!p3) {
				delete mf;
				return NULL;
			}
			tmpstr = (gchar *)g_memdup(p2, p3-p2+1);
			tmpstr[p3-p2] = '\0';
			if (atoi(tmpstr)!=cltfunc) {
				g_free(tmpstr);
				delete mf;
				return NULL;
			}
			g_free(tmpstr);
		}
		if (cachestat.st_size!=glong(filedatasize + strlen(mf->begin()) +1)) {
			delete mf;
			return NULL;
		}
		struct stat idxstat;
		if (g_stat(url.c_str(), &idxstat)!=0) {
			delete mf;
			return NULL;
		}
		if (cachestat.st_mtime<idxstat.st_mtime) {
			delete mf;
			return NULL;
		}
		//g_print("Using map file: %s\n", filename);
		return mf;
	}
	g_free(tmpstr);
	delete mf;
	gchar *basename = g_path_get_basename(saveurl.c_str());
	p = strrchr(basename, '.');
	if (!p) {
		g_free(basename);
		return NULL;
	}
	*p='\0';
	gchar *extendname = p+1;
	gchar *dirname = g_path_get_dirname(filename);
	gchar *nextfilename;
	if (isoftfile)
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.oft", dirname, basename, next, extendname);
	else
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.clt", dirname, basename, next, extendname);
	MapFile *out = get_cache_loadfile(nextfilename, url, saveurl, cltfunc, filedatasize, next+1);
	g_free(basename);
	g_free(dirname);
	g_free(nextfilename);
	return out;
}

bool cache_file::load_cache(const std::string& url, const std::string& saveurl, CollateFunctions cltfunc, glong filedatasize)
{
	std::string oftfilename;
	if (isoftfile)
		oftfilename=saveurl+".oft";
	else
		oftfilename=saveurl+".clt";
	for (int i=0;i<2;i++) {
		if (i==1) {
			if (!get_cache_filename(saveurl, oftfilename, false))
				break;
		}
		mf = get_cache_loadfile(oftfilename.c_str(), url, saveurl, cltfunc, filedatasize, 2);
		if (!mf)
			continue;
		wordoffset = (guint32 *)(mf->begin()+strlen(mf->begin())+1);
		return true;
	}
	return false;
}

bool cache_file::get_cache_filename(const std::string& url, std::string &cachefilename, bool create)
{
	if (create) {
		if (!g_file_test(g_get_user_cache_dir(), G_FILE_TEST_EXISTS) &&
		    g_mkdir(g_get_user_cache_dir(), 0700)==-1)
			return false;
	}

	std::string cache_dir=g_get_user_cache_dir();
	cache_dir += G_DIR_SEPARATOR_S "stardict";

	if (create) {
		if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_EXISTS)) {
			if (g_mkdir(cache_dir.c_str(), 0700)==-1)
				return false;
		} else if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_IS_DIR))
			return false;
	}

	gchar *base=g_path_get_basename(url.c_str());
	if (isoftfile)
		cachefilename = cache_dir+G_DIR_SEPARATOR_S+base+".oft";
	else
		cachefilename = cache_dir+G_DIR_SEPARATOR_S+base+".clt";
	g_free(base);
	return true;
}

FILE* cache_file::get_cache_savefile(const gchar *filename, const std::string &url, int next, std::string &cfilename)
{
	cfilename = filename;
	struct stat oftstat;
	if (g_stat(filename, &oftstat)!=0) {
		return fopen(filename, "wb");
	}
	MapFile mf;
	if (!mf.open(filename, oftstat.st_size)) {
		return fopen(filename, "wb");
	}
	gchar *p = mf.begin();
	bool has_prefix;
	if (isoftfile)
		has_prefix = g_str_has_prefix(p, OFFSETFILE_MAGIC_DATA);
	else
		has_prefix = g_str_has_prefix(p, COLLATIONFILE_MAGIC_DATA);
	if (!has_prefix) {
		mf.close();
		return fopen(filename, "wb");
	}
	if (isoftfile)
		p+= sizeof(OFFSETFILE_MAGIC_DATA)-1-1;
	else
		p+= sizeof(COLLATIONFILE_MAGIC_DATA)-1-1;
	gchar *p2;
	p2 = strstr(p, "\nurl=");
	if (!p2) {
		mf.close();
		return fopen(filename, "wb");
	}
	p2+=sizeof("\nurl=")-1;
	gchar *p3;
	p3 = strchr(p2, '\n');
	if (!p3) {
		mf.close();
		return fopen(filename, "wb");
	}
	gchar *tmpstr;
	tmpstr = (gchar *)g_memdup(p2, p3-p2+1);
	tmpstr[p3-p2] = '\0';
	if (url == tmpstr) {
		g_free(tmpstr);
		mf.close();
		return fopen(filename, "wb");
	}
	g_free(tmpstr);
	mf.close();
	gchar *basename = g_path_get_basename(url.c_str());
	p = strrchr(basename, '.');
	if (!p) {
		g_free(basename);
		return NULL;
	}
	*p='\0';
	gchar *extendname = p+1;
	gchar *dirname = g_path_get_dirname(filename);
	gchar *nextfilename;
	if (isoftfile)
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.oft", dirname, basename, next, extendname);
	else
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.clt", dirname, basename, next, extendname);
	FILE *out = get_cache_savefile(nextfilename, url, next+1, cfilename);
	g_free(basename);
	g_free(dirname);
	g_free(nextfilename);
	return out;
}

bool cache_file::save_cache(const std::string& url, CollateFunctions cltfunc, gulong npages)
{
	std::string oftfilename;
	if (isoftfile)
		oftfilename=url+".oft";
	else
		oftfilename=url+".clt";
	for (int i=0;i<2;i++) {
		if (i==1) {
			if (!get_cache_filename(url, oftfilename, true))
				break;
		}
		std::string cfilename;
		FILE *out= get_cache_savefile(oftfilename.c_str(), url, 2, cfilename);
		if (!out)
			continue;
		if (isoftfile)
			fwrite(OFFSETFILE_MAGIC_DATA, 1, sizeof(OFFSETFILE_MAGIC_DATA)-1, out);
		else
			fwrite(COLLATIONFILE_MAGIC_DATA, 1, sizeof(COLLATIONFILE_MAGIC_DATA)-1, out);
		fwrite("url=", 1, sizeof("url=")-1, out);
		fwrite(url.c_str(), 1, url.length(), out);
		if (!isoftfile)
			fprintf(out, "\nfunc=%d", cltfunc);
		fwrite("\n", 1, 2, out);
		fwrite(wordoffset, sizeof(guint32), npages, out);
		fclose(out);
		g_print("Save cache file: %s\n", cfilename.c_str());
		return true;
	}
	return false;
}

collation_file::collation_file(idxsyn_file *_idx_file) : cache_file(false)
{
	idx_file = _idx_file;
}

const gchar *collation_file::GetWord(glong idx)
{
	return idx_file->get_key(wordoffset[idx]);
}

glong collation_file::GetOrigIndex(glong cltidx)
{
	return wordoffset[cltidx];
}

bool collation_file::lookup(const char *sWord, glong &idx)
{
	bool bFound=false;
	glong iTo=idx_file->wordcount-1;
	if (stardict_collate(sWord, GetWord(0), CollateFunction)<0) {
		idx = 0;
	} else if (stardict_collate(sWord, GetWord(iTo), CollateFunction) >0) {
		idx = INVALID_INDEX;
	} else {
		glong iThisIndex=0;
		glong iFrom=0;
		gint cmpint;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_collate(sWord, GetWord(iThisIndex), CollateFunction);
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if (!bFound) {
			idx = iFrom;    //next
		} else {
			idx = iThisIndex;
		}
	}
	return bFound;
}

static gint sort_collation_index(gconstpointer a, gconstpointer b, gpointer user_data)
{
	idxsyn_file *idx_file = (idxsyn_file *)user_data;
	gchar *str1 = g_strdup(idx_file->get_key(*((glong *)a)));
	const gchar *str2 = idx_file->get_key(*((glong *)b));
	gint x = stardict_collate(str1, str2, idx_file->clt_file->CollateFunction);
	g_free(str1);
	if (x==0)
		return *((glong *)a) - *((glong *)b);
	else
		return x;
}

bool offset_index::load(const std::string& url, gulong wc, gulong fsize,
			bool CreateCacheFile, bool EnableCollation,
			CollateFunctions _CollateFunction, show_progress_t *sp)
{
	wordcount=wc;
	npages=(wc-1)/ENTR_PER_PAGE+2;
	if (!oft_file.load_cache(url, url, _CollateFunction, npages*sizeof(guint32))) {
		MapFile map_file;
		if (!map_file.open(url.c_str(), fsize))
			return false;
		const gchar *idxdatabuffer=map_file.begin();
		oft_file.wordoffset = (guint32 *)g_malloc(npages*sizeof(guint32));
		const gchar *p1 = idxdatabuffer;
		gulong index_size;
		guint32 j=0;
		for (guint32 i=0; i<wc; i++) {
			index_size=strlen(p1) +1 + 2*sizeof(guint32);
			if (i % ENTR_PER_PAGE==0) {
				oft_file.wordoffset[j]=p1-idxdatabuffer;
				++j;
			}
			p1 += index_size;
		}
		oft_file.wordoffset[j]=p1-idxdatabuffer;
		map_file.close();
		if (CreateCacheFile) {
			if (!oft_file.save_cache(url, _CollateFunction, npages))
				g_printerr("Cache update failed.\n");
		}
	}

	if (!(idxfile = fopen(url.c_str(), "rb"))) {
		return false;
	}

	first.assign(0, read_first_on_page_key(0));
	last.assign(npages-2, read_first_on_page_key(npages-2));
	middle.assign((npages-2)/2, read_first_on_page_key((npages-2)/2));
	real_last.assign(wc-1, get_key(wc-1));

	if (EnableCollation)
		collate_sort(url, url, _CollateFunction, sp);

	return true;
}

inline gulong offset_index::load_page(glong page_idx)
{
	gulong nentr=ENTR_PER_PAGE;
	if (page_idx==glong(npages-2))
		if ((nentr=wordcount%ENTR_PER_PAGE)==0)
			nentr=ENTR_PER_PAGE;


	if (page_idx!=page.idx) {
		page_data.resize(oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx]);
		fseek(idxfile, oft_file.wordoffset[page_idx], SEEK_SET);
		fread(&page_data[0], 1, page_data.size(), idxfile);
		page.fill(&page_data[0], nentr, page_idx);
	}

	return nentr;
}

const gchar *offset_index::get_key(glong idx)
{
	load_page(idx/ENTR_PER_PAGE);
	glong idx_in_page=idx%ENTR_PER_PAGE;
	wordentry_offset=page.entries[idx_in_page].off;
	wordentry_size=page.entries[idx_in_page].size;

	return page.entries[idx_in_page].keystr;
}

void offset_index::get_data(glong idx)
{
	get_key(idx);
}

const gchar *offset_index::get_key_and_data(glong idx)
{
	return get_key(idx);
}

bool offset_index::lookup(const char *str, glong &idx)
{
	bool bFound=false;
	glong iFrom;
	glong iTo=npages-2;
	gint cmpint;
	glong iThisIndex;
	if (stardict_strcmp(str, first.keystr.c_str())<0) {
		idx = 0;
		return false;
	} else if (stardict_strcmp(str, real_last.keystr.c_str()) >0) {
		idx = INVALID_INDEX;
		return false;
	} else {
		iFrom=0;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, get_first_on_page_key(iThisIndex));
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if (!bFound)
			idx = iTo;    //prev
		else
			idx = iThisIndex;
	}
	if (!bFound) {
		gulong netr=load_page(idx);
		iFrom=1; // Needn't search the first word anymore.
		iTo=netr-1;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, page.entries[iThisIndex].keystr);
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		idx*=ENTR_PER_PAGE;
		if (!bFound)
			idx += iFrom;    //next
		else
			idx += iThisIndex;
	} else {
		idx*=ENTR_PER_PAGE;
	}
	return bFound;
}

wordlist_index::wordlist_index()
{
	clt_file = NULL;
	idxdatabuf = NULL;
}

wordlist_index::~wordlist_index()
{
	delete clt_file;
	g_free(idxdatabuf);
}

bool wordlist_index::load(const std::string& url, gulong wc, gulong fsize,
			  bool CreateCacheFile, bool EnableCollation,
			  CollateFunctions _CollateFunction, show_progress_t *sp)
{
	wordcount=wc;
	gzFile in = gzopen(url.c_str(), "rb");
	if (in == NULL)
		return false;

	idxdatabuf = (gchar *)g_malloc(fsize);

	gulong len = gzread(in, idxdatabuf, fsize);
	gzclose(in);
	if (len < 0)
		return false;

	if (len != fsize)
		return false;

	wordlist.resize(wc+1);
	gchar *p1 = idxdatabuf;
	guint32 i;
	for (i=0; i<wc; i++) {
		wordlist[i] = p1;
		p1 += strlen(p1) +1 + 2*sizeof(guint32);
	}
	wordlist[wc] = p1;

	if (EnableCollation) {
		std::string saveurl = url;
		saveurl.erase(saveurl.length()-sizeof(".gz")+1, sizeof(".gz")-1);

		collate_sort(url, saveurl, _CollateFunction, sp);
	}
	return true;
}

const gchar *wordlist_index::get_key(glong idx)
{
	return wordlist[idx];
}

void wordlist_index::get_data(glong idx)
{
	gchar *p1 = wordlist[idx]+strlen(wordlist[idx])+sizeof(gchar);
	wordentry_offset = g_ntohl(*reinterpret_cast<guint32 *>(p1));
	p1 += sizeof(guint32);
	wordentry_size = g_ntohl(*reinterpret_cast<guint32 *>(p1));
}

const gchar *wordlist_index::get_key_and_data(glong idx)
{
	get_data(idx);
	return get_key(idx);
}

bool wordlist_index::lookup(const char *str, glong &idx)
{
	bool bFound=false;
	glong iTo=wordlist.size()-2;

	if (stardict_strcmp(str, get_key(0))<0) {
		idx = 0;
	} else if (stardict_strcmp(str, get_key(iTo)) >0) {
		idx = INVALID_INDEX;
	} else {
		glong iThisIndex=0;
		glong iFrom=0;
		gint cmpint;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, get_key(iThisIndex));
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if (!bFound)
			idx = iFrom;    //next
		else
			idx = iThisIndex;
	}
	return bFound;
}

//===================================================================
void synonym_file::page_t::fill(gchar *data, gint nent, glong idx_)
{
	idx=idx_;
	gchar *p=data;
	glong len;
	for (gint i=0; i<nent; ++i) {
		entries[i].keystr=p;
		len=strlen(p);
		p+=len+1;
		entries[i].index=g_ntohl(*reinterpret_cast<guint32 *>(p));
		p+=sizeof(guint32);
	}
}

synonym_file::synonym_file() : oft_file(true)
{
	clt_file = NULL;
}

synonym_file::~synonym_file()
{
	delete clt_file;
	if (synfile)
		fclose(synfile);
}

inline const gchar *synonym_file::read_first_on_page_key(glong page_idx)
{
	fseek(synfile, oft_file.wordoffset[page_idx], SEEK_SET);
	guint32 page_size=oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx];
	gulong minsize = sizeof(wordentry_buf);
        if (page_size < minsize)
                minsize = page_size;
	fread(wordentry_buf, minsize, 1, synfile); //TODO: check returned values, deal with word entry that strlen>255.
	return wordentry_buf;
}

inline const gchar *synonym_file::get_first_on_page_key(glong page_idx)
{
	if (page_idx<middle.idx) {
		if (page_idx==first.idx)
			return first.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else if (page_idx>middle.idx) {
		if (page_idx==last.idx)
			return last.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else
		return middle.keystr.c_str();
}

void idxsyn_file::collate_sort(const std::string& url,
			       const std::string& saveurl,
			       CollateFunctions collf,
			       show_progress_t *sp)
{
	clt_file = new collation_file(this);
	clt_file->CollateFunction = collf;
	if (!clt_file->load_cache(url, saveurl, collf, wordcount*sizeof(guint32))) {
		sp->notify_about_start(_("Sorting, please wait..."));
		clt_file->wordoffset = (guint32 *)g_malloc(wordcount*sizeof(guint32));
		for (glong i=0; i<wordcount; i++)
			clt_file->wordoffset[i] = i;
		g_qsort_with_data(clt_file->wordoffset, wordcount, sizeof(guint32), sort_collation_index, this);
		if (!clt_file->save_cache(saveurl, collf, wordcount))
			g_printerr("Cache update failed.\n");
	}
}

bool synonym_file::load(const std::string& url, gulong wc, bool CreateCacheFile,
			bool EnableCollation, CollateFunctions _CollateFunction,
			show_progress_t *sp)
{
	wordcount=wc;
	npages=(wc-1)/ENTR_PER_PAGE+2;
	if (!oft_file.load_cache(url, url, _CollateFunction, npages*sizeof(guint32))) {
		struct stat stats;
		if (stat (url.c_str(), &stats) == -1)
			return false;
		MapFile map_file;
		if (!map_file.open(url.c_str(), stats.st_size))
			return false;
		const gchar *syndatabuffer=map_file.begin();
		oft_file.wordoffset = (guint32 *)g_malloc(npages*sizeof(guint32));
		const gchar *p1 = syndatabuffer;
		gulong index_size;
		guint32 j=0;
		for (guint32 i=0; i<wc; i++) {
			index_size=strlen(p1) +1 + sizeof(guint32);
			if (i % ENTR_PER_PAGE==0) {
				oft_file.wordoffset[j]=p1-syndatabuffer;
				++j;
			}
			p1 += index_size;
		}
		oft_file.wordoffset[j]=p1-syndatabuffer;
		map_file.close();
		if (CreateCacheFile) {
			if (!oft_file.save_cache(url, _CollateFunction, npages))
				g_printerr("Cache update failed.\n");
		}
	}

	if (!(synfile = fopen(url.c_str(), "rb"))) {
		return false;
	}

	first.assign(0, read_first_on_page_key(0));
	last.assign(npages-2, read_first_on_page_key(npages-2));
	middle.assign((npages-2)/2, read_first_on_page_key((npages-2)/2));
	real_last.assign(wc-1, get_key(wc-1));

	if (EnableCollation)
		collate_sort(url, url, _CollateFunction, sp);

	return true;
}

inline gulong synonym_file::load_page(glong page_idx)
{
	gulong nentr=ENTR_PER_PAGE;
	if (page_idx==glong(npages-2))
		if ((nentr=wordcount%ENTR_PER_PAGE)==0)
			nentr=ENTR_PER_PAGE;


	if (page_idx!=page.idx) {
		page_data.resize(oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx]);
		fseek(synfile, oft_file.wordoffset[page_idx], SEEK_SET);
		fread(&page_data[0], 1, page_data.size(), synfile);
		page.fill(&page_data[0], nentr, page_idx);
	}

	return nentr;
}

const gchar *synonym_file::get_key(glong idx)
{
	load_page(idx/ENTR_PER_PAGE);
	glong idx_in_page=idx%ENTR_PER_PAGE;
	wordentry_index=page.entries[idx_in_page].index;

	return page.entries[idx_in_page].keystr;
}

bool synonym_file::lookup(const char *str, glong &idx)
{
	bool bFound=false;
	glong iFrom;
	glong iTo=npages-2;
	gint cmpint;
	glong iThisIndex;
	if (stardict_strcmp(str, first.keystr.c_str())<0) {
		idx = 0;
		return false;
	} else if (stardict_strcmp(str, real_last.keystr.c_str()) >0) {
		idx = INVALID_INDEX;
		return false;
	} else {
		iFrom=0;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, get_first_on_page_key(iThisIndex));
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if (!bFound)
			idx = iTo;    //prev
		else
			idx = iThisIndex;
	}
	if (!bFound) {
		gulong netr=load_page(idx);
		iFrom=1; // Needn't search the first word anymore.
		iTo=netr-1;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, page.entries[iThisIndex].keystr);
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		idx*=ENTR_PER_PAGE;
		if (!bFound)
			idx += iFrom;    //next
		else
			idx += iThisIndex;
	} else {
		idx*=ENTR_PER_PAGE;
	}
	return bFound;
}

//===================================================================
bool Dict::load(const std::string& ifofilename, bool CreateCacheFile,
		bool EnableCollation, CollateFunctions CollateFunction,
		show_progress_t *sp)
{
	gulong idxfilesize;
	glong wordcount, synwordcount;
	if (!load_ifofile(ifofilename, idxfilesize, wordcount, synwordcount))
		return false;
	sp->notify_about_start(_("Loading..."));
	std::string fullfilename(ifofilename);
	fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "dict.dz");

	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		dictdzfile.reset(new dictData);
		if (!dictdzfile->open(fullfilename, 0)) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	} else {
		fullfilename.erase(fullfilename.length()-sizeof(".dz")+1, sizeof(".dz")-1);
		dictfile = fopen(fullfilename.c_str(),"rb");
		if (!dictfile) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	}

	fullfilename=ifofilename;
	fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "idx.gz");

	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		idx_file.reset(new wordlist_index);
	} else {
		fullfilename.erase(fullfilename.length()-sizeof(".gz")+1, sizeof(".gz")-1);
		idx_file.reset(new offset_index);
	}

	if (!idx_file->load(fullfilename, wordcount, idxfilesize,
			    CreateCacheFile, EnableCollation,
			    CollateFunction, sp))
		return false;

	if (synwordcount) {
		fullfilename=ifofilename;
		fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "syn");
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
			syn_file.reset(new synonym_file);
			if (!syn_file->load(fullfilename, synwordcount,
					    CreateCacheFile, EnableCollation,
					    CollateFunction, sp))
				return false;
		}
	}

	g_print("bookname: %s , wordcount %lu\n", bookname.c_str(), wordcount);
	return true;
}

bool Dict::load_ifofile(const std::string& ifofilename, gulong &idxfilesize, glong &wordcount, glong &synwordcount)
{
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(ifofilename, false))
		return false;
	if (dict_info.wordcount==0)
		return false;

	ifo_file_name=dict_info.ifo_file_name;
	bookname=dict_info.bookname;

	idxfilesize=dict_info.index_file_size;
	wordcount=dict_info.wordcount;
	synwordcount=dict_info.synwordcount;

	sametypesequence=dict_info.sametypesequence;

	return true;
}

glong Dict::nsynarticles()
{
	if (syn_file.get() == NULL)
		return 0;
	return syn_file->wordcount;
}

bool Dict::GetWordPrev(glong idx, glong &pidx, bool isidx, bool isClt)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	if (idx==INVALID_INDEX) {
		pidx = is_file->wordcount-1;
		return true;
	}
	pidx = idx;
	gchar *cWord;
	if (isClt)
		cWord = g_strdup(is_file->clt_file->GetWord(pidx));
	else
		cWord = g_strdup(is_file->get_key(pidx));
	const gchar *pWord;
	bool found=false;
	while (pidx>0) {
		if (isClt)
			pWord = is_file->clt_file->GetWord(pidx-1);
		else
			pWord = is_file->get_key(pidx-1);
		if (strcmp(pWord, cWord)!=0) {
			found=true;
			break;
		}
		pidx--;
	}
	g_free(cWord);
	if (found) {
		pidx--;
		return true;
	} else {
		return false;
	}
}

void Dict::GetWordNext(glong &idx, bool isidx, bool isClt)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	gchar *cWord;
	if (isClt)
		cWord = g_strdup(is_file->clt_file->GetWord(idx));
	else
		cWord = g_strdup(is_file->get_key(idx));
	const gchar *pWord;
	bool found=false;
	while (idx < is_file->wordcount-1) {
		if (isClt)
			pWord = is_file->clt_file->GetWord(idx+1);
		else
			pWord = is_file->get_key(idx+1);
		if (strcmp(pWord, cWord)!=0) {
			found=true;
			break;
		}
		idx++;
	}
	g_free(cWord);
	if (found)
		idx++;
	else
		idx=INVALID_INDEX;
}

gint Dict::GetWordCount(glong& idx, bool isidx)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	gchar *cWord = g_strdup(is_file->get_key(idx));
	const gchar *pWord;
	gint count = 1;
	glong idx1 = idx;
	while (idx1>0) {
		pWord = is_file->get_key(idx1-1);
		if (strcmp(pWord, cWord)!=0)
			break;
		count++;
		idx1--;
	}
	glong idx2=idx;
	while (idx2<is_file->wordcount-1) {
		pWord = is_file->get_key(idx2+1);
		if (strcmp(pWord, cWord)!=0)
			break;
		count++;
		idx2++;
	}
	idx=idx1;
	g_free(cWord);
	return count;
}

bool Dict::LookupSynonym(const char *str, glong &synidx)
{
	if (syn_file.get() == NULL) {
		synidx = UNSET_INDEX;
		return false;
	}
	return syn_file->lookup(str, synidx);
}

bool Dict::LookupCltSynonym(const char *str, glong &synidx)
{
	if (syn_file.get() == NULL) {
		synidx = UNSET_INDEX;
		return false;
	}
	return syn_file->clt_file->lookup(str, synidx);
}

bool Dict::LookupWithRule(GPatternSpec *pspec, glong *aIndex, int iBuffLen)
{
	int iIndexCount=0;
	for (glong i=0; i<narticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_pattern_match_string(pspec, get_key(i)))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

bool Dict::LookupWithRuleSynonym(GPatternSpec *pspec, glong *aIndex, int iBuffLen)
{
	if (syn_file.get() == NULL)
		return false;
	int iIndexCount=0;
	for (glong i=0; i<nsynarticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_pattern_match_string(pspec, syn_file->get_key(i)))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

//===================================================================
show_progress_t Libs::default_show_progress;

Libs::Libs(show_progress_t *sp, bool create, bool enable, int function)
{
	set_show_progress(sp);
	CreateCacheFile = create;
	EnableCollation = enable;
	CollateFunction = (CollateFunctions)function;
	iMaxFuzzyDistance  = MAX_FUZZY_DISTANCE; //need to read from cfg.
	if (EnableCollation)
		utf8_collate_init(CollateFunction);
}

Libs::~Libs()
{
	for (std::vector<Dict *>::iterator p=oLib.begin(); p!=oLib.end(); ++p)
		delete *p;
	if (EnableCollation)
		utf8_collate_end();
}

void Libs::load_dict(const std::string& url, show_progress_t *sp)
{
	Dict *lib=new Dict;
	if (lib->load(url, CreateCacheFile, EnableCollation,
		      CollateFunction, sp))
		oLib.push_back(lib);
	else
		delete lib;
}

class DictLoader {
public:
	DictLoader(Libs& lib_): lib(lib_) {}
	void operator()(const std::string& url, bool disable) {
		if (!disable)
			lib.load_dict(url, lib.show_progress);
	}
private:
	Libs& lib;
};

void Libs::load(const strlist_t& dicts_dirs,
		const strlist_t& order_list,
		const strlist_t& disable_list)
{
	for_each_file(dicts_dirs, ".ifo", order_list, disable_list,
		      DictLoader(*this));
}

class DictReLoader {
public:
	DictReLoader(std::vector<Dict *> &p, std::vector<Dict *> &f,
		     Libs& lib_) : prev(p), future(f), lib(lib_)
		{
		}
	void operator()(const std::string& url, bool disable) {
		if (!disable) {
			Dict *dict=find(url);
			if (dict)
				future.push_back(dict);
			else
				lib.load_dict(url, lib.show_progress);
		}
	}
private:
	std::vector<Dict *> &prev;
	std::vector<Dict *> &future;
	Libs& lib;

	Dict *find(const std::string& url) {
		std::vector<Dict *>::iterator it;
		for (it=prev.begin(); it!=prev.end(); ++it)
			if ((*it)->ifofilename()==url)
				break;
		if (it!=prev.end()) {
			Dict *res=*it;
			prev.erase(it);
			return res;
		}
		return NULL;
	}
};

void Libs::reload(const strlist_t& dicts_dirs, const strlist_t& order_list,
		  const strlist_t& disable_list, bool is_coll_enb, int collf)
{
	if (is_coll_enb == EnableCollation && collf == CollateFunction) {
		std::vector<Dict *> prev(oLib);
		oLib.clear();
		for_each_file(dicts_dirs, ".ifo", order_list, disable_list, DictReLoader(prev, oLib, *this));
		for (std::vector<Dict *>::iterator it=prev.begin(); it!=prev.end(); ++it)
			delete *it;
	} else {
		for (std::vector<Dict *>::iterator it = oLib.begin();
		     it != oLib.end(); ++it)
			delete *it;
		oLib.clear();
		EnableCollation = is_coll_enb;
		CollateFunction = CollateFunctions(collf);
		load(dicts_dirs, order_list, disable_list);
	}
}

const gchar *Libs::poGetCurrentWord(CurrentIndex * iCurrent)
{
	bool isClt = EnableCollation;
	const gchar *poCurrentWord = NULL;
	const gchar *word;
	std::vector<Dict *>::size_type iLib;
	for (iLib=0; iLib<oLib.size(); iLib++) {
		if (iCurrent[iLib].idx==INVALID_INDEX)
			continue;
		if ( iCurrent[iLib].idx>=narticles(iLib) || iCurrent[iLib].idx<0)
			continue;
		if ( poCurrentWord == NULL ) {
			if (isClt)
				poCurrentWord = poGetCltWord(iCurrent[iLib].idx, iLib);
			else
				poCurrentWord = poGetWord(iCurrent[iLib].idx, iLib);
		} else {
			gint x;
			if (isClt) {
				word = poGetCltWord(iCurrent[iLib].idx, iLib);
				x = stardict_collate(poCurrentWord, word, CollateFunction);
			} else {
				word = poGetWord(iCurrent[iLib].idx, iLib);
				x = stardict_strcmp(poCurrentWord, word);
			}
			if (x > 0) {
				poCurrentWord = word;
			}
		}
	}
	for (iLib=0; iLib<oLib.size(); iLib++) {
		if (iCurrent[iLib].synidx==UNSET_INDEX)
			continue;
		if (iCurrent[iLib].synidx==INVALID_INDEX)
			continue;
		if ( iCurrent[iLib].synidx>=nsynarticles(iLib) || iCurrent[iLib].synidx<0)
			continue;
		if ( poCurrentWord == NULL ) {
			if (isClt)
				poCurrentWord = poGetCltSynonymWord(iCurrent[iLib].synidx, iLib);
			else
				poCurrentWord = poGetSynonymWord(iCurrent[iLib].synidx, iLib);
		} else {
			gint x;
			if (isClt) {
				word = poGetCltSynonymWord(iCurrent[iLib].synidx, iLib);
				x = stardict_collate(poCurrentWord, word, CollateFunction);
			} else {
				word = poGetSynonymWord(iCurrent[iLib].synidx, iLib);
				x = stardict_strcmp(poCurrentWord, word);
			}
			if (x > 0) {
				poCurrentWord = word;
			}
		}
	}
	return poCurrentWord;
}

const gchar *
Libs::poGetNextWord(const gchar *sWord, CurrentIndex *iCurrent)
{
	bool isClt = EnableCollation;
	// the input can be:
	// (word,iCurrent),read word,write iNext to iCurrent,and return next word. used by TopWin::NextCallback();
	// (NULL,iCurrent),read iCurrent,write iNext to iCurrent,and return next word. used by AppCore::ListWords();
	const gchar *poCurrentWord = NULL;
	std::vector<Dict *>::size_type iCurrentLib=0;
	bool isLib;
	const gchar *word;

	std::vector<Dict *>::size_type iLib;
	for (iLib=0;iLib<oLib.size();iLib++) {
		if (sWord) {
			if (isClt)
				oLib[iLib]->LookupClt(sWord, iCurrent[iLib].idx);
			else
				oLib[iLib]->Lookup(sWord, iCurrent[iLib].idx);
		}
		if (iCurrent[iLib].idx==INVALID_INDEX)
			continue;
		if (iCurrent[iLib].idx>=narticles(iLib) || iCurrent[iLib].idx<0)
			continue;
		if (poCurrentWord == NULL ) {
			if (isClt)
				poCurrentWord = poGetCltWord(iCurrent[iLib].idx, iLib);
			else
				poCurrentWord = poGetWord(iCurrent[iLib].idx, iLib);
			iCurrentLib = iLib;
			isLib=true;
		} else {
			gint x;
			if (isClt) {
				word = poGetCltWord(iCurrent[iLib].idx, iLib);
				x = stardict_collate(poCurrentWord, word, CollateFunction);
			} else {
				word = poGetWord(iCurrent[iLib].idx, iLib);
				x = stardict_strcmp(poCurrentWord, word);
			}
			if (x > 0) {
				poCurrentWord = word;
				iCurrentLib = iLib;
				isLib=true;
			}
		}
	}
	for (iLib=0;iLib<oLib.size();iLib++) {
		if (sWord) {
			if (isClt)
				oLib[iLib]->LookupCltSynonym(sWord, iCurrent[iLib].synidx);
			else
				oLib[iLib]->LookupSynonym(sWord, iCurrent[iLib].synidx);
		}
		if (iCurrent[iLib].synidx==UNSET_INDEX)
			continue;
		if (iCurrent[iLib].synidx==INVALID_INDEX)
			continue;
		if (iCurrent[iLib].synidx>=nsynarticles(iLib) || iCurrent[iLib].synidx<0)
			continue;
		if (poCurrentWord == NULL ) {
			if (isClt)
				poCurrentWord = poGetCltSynonymWord(iCurrent[iLib].synidx, iLib);
			else
				poCurrentWord = poGetSynonymWord(iCurrent[iLib].synidx, iLib);
			iCurrentLib = iLib;
			isLib=false;
		} else {
			gint x;
			if (isClt) {
				word = poGetCltSynonymWord(iCurrent[iLib].synidx, iLib);
				x = stardict_collate(poCurrentWord, word, CollateFunction);
			} else {
				word = poGetSynonymWord(iCurrent[iLib].synidx, iLib);
				x = stardict_strcmp(poCurrentWord, word);
			}
			if (x > 0 ) {
				poCurrentWord = word;
				iCurrentLib = iLib;
				isLib=false;
			}
		}
	}
	if (poCurrentWord) {
		for (iLib=0;iLib<oLib.size();iLib++) {
			if (isLib && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].idx==INVALID_INDEX)
				continue;
			if (iCurrent[iLib].idx>=narticles(iLib) || iCurrent[iLib].idx<0)
				continue;
			if (isClt)
				word = poGetCltWord(iCurrent[iLib].idx, iLib);
			else
				word = poGetWord(iCurrent[iLib].idx, iLib);
			if (strcmp(poCurrentWord, word) == 0) {
				GetWordNext(iCurrent[iLib].idx, iLib, true, isClt);
			}
		}
		for (iLib=0;iLib<oLib.size();iLib++) {
			if ((!isLib) && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].synidx==UNSET_INDEX)
				continue;
			if (iCurrent[iLib].synidx==INVALID_INDEX)
				continue;
			if (iCurrent[iLib].synidx>=nsynarticles(iLib) || iCurrent[iLib].synidx<0)
				continue;
			if (isClt)
				word = poGetCltSynonymWord(iCurrent[iLib].synidx, iLib);
			else
				word = poGetSynonymWord(iCurrent[iLib].synidx, iLib);
			if (strcmp(poCurrentWord, word) == 0) {
				GetWordNext(iCurrent[iLib].synidx, iLib, false, isClt);
			}
		}
		//GetWordNext will change poCurrentWord's content, so do it at the last.
		if (isLib) {
			GetWordNext(iCurrent[iCurrentLib].idx, iCurrentLib, true, isClt);
		} else {
			GetWordNext(iCurrent[iCurrentLib].synidx, iCurrentLib, false, isClt);
		}
		poCurrentWord = poGetCurrentWord(iCurrent);
	}
	return poCurrentWord;
}

const gchar *
Libs::poGetPreWord(const gchar *sWord, CurrentIndex* iCurrent)
{
	bool isClt = EnableCollation;
	// used by TopWin::PreviousCallback(); the iCurrent is cached by AppCore::TopWinWordChange();
	const gchar *poCurrentWord = NULL;
	std::vector<Dict *>::size_type iCurrentLib=0;
	bool isLib;

	const gchar *word;
	glong pidx;
	std::vector<Dict *>::size_type iLib;
	for (iLib=0;iLib<oLib.size();iLib++) {
		if (sWord) {
			if (isClt)
				oLib[iLib]->LookupClt(sWord, iCurrent[iLib].idx);
			else
				oLib[iLib]->Lookup(sWord, iCurrent[iLib].idx);
		}
		if (iCurrent[iLib].idx!=INVALID_INDEX) {
			if ( iCurrent[iLib].idx>=narticles(iLib) || iCurrent[iLib].idx<=0)
				continue;
		}
		if ( poCurrentWord == NULL ) {
			if (GetWordPrev(iCurrent[iLib].idx, pidx, iLib, true, isClt)) {
				if (isClt)
					poCurrentWord = poGetCltWord(pidx, iLib);
				else
					poCurrentWord = poGetWord(pidx, iLib);
				iCurrentLib = iLib;
				isLib=true;
			}
		} else {
			if (GetWordPrev(iCurrent[iLib].idx, pidx, iLib, true, isClt)) {
				gint x;
				if (isClt) {
					word = poGetCltWord(pidx, iLib);
					x = stardict_collate(poCurrentWord, word, CollateFunction);
				} else {
					word = poGetWord(pidx, iLib);
					x = stardict_strcmp(poCurrentWord, word);
				}
				if (x < 0 ) {
					poCurrentWord = word;
					iCurrentLib = iLib;
					isLib=true;
				}
			}
		}
	}
	for (iLib=0;iLib<oLib.size();iLib++) {
		if (sWord) {
			if (isClt)
				oLib[iLib]->LookupCltSynonym(sWord, iCurrent[iLib].synidx);
			else
				oLib[iLib]->LookupSynonym(sWord, iCurrent[iLib].synidx);
		}
		if (iCurrent[iLib].synidx==UNSET_INDEX)
			continue;
		if (iCurrent[iLib].synidx!=INVALID_INDEX) {
			if ( iCurrent[iLib].synidx>=nsynarticles(iLib) || iCurrent[iLib].synidx<=0)
				continue;
		}
		if ( poCurrentWord == NULL ) {
			if (GetWordPrev(iCurrent[iLib].synidx, pidx, iLib, false, isClt)) {
				if (isClt)
					poCurrentWord = poGetCltSynonymWord(pidx, iLib);
				else
					poCurrentWord = poGetSynonymWord(pidx, iLib);
				iCurrentLib = iLib;
				isLib=false;
			}
		} else {
			if (GetWordPrev(iCurrent[iLib].synidx, pidx, iLib, false, isClt)) {
				gint x;
				if (isClt) {
					word = poGetCltSynonymWord(pidx,iLib);
					x = stardict_collate(poCurrentWord, word, CollateFunction);
				} else {
					word = poGetSynonymWord(pidx,iLib);
					x = stardict_strcmp(poCurrentWord, word);
				}
				if (x < 0 ) {
					poCurrentWord = word;
					iCurrentLib = iLib;
					isLib=false;
				}
			}
		}
	}
	if (poCurrentWord) {
		for (iLib=0;iLib<oLib.size();iLib++) {
			if (isLib && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].idx!=INVALID_INDEX) {
				if (iCurrent[iLib].idx>=narticles(iLib) || iCurrent[iLib].idx<=0)
					continue;
			}
			if (GetWordPrev(iCurrent[iLib].idx, pidx, iLib, true, isClt)) {
				if (isClt)
					word = poGetCltWord(pidx,iLib);
				else
					word = poGetWord(pidx,iLib);
				if (strcmp(poCurrentWord, word) == 0) {
					iCurrent[iLib].idx=pidx;
				}
			}
		}
		for (iLib=0;iLib<oLib.size();iLib++) {
			if ((!isLib) && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].synidx==UNSET_INDEX)
				continue;
			if (iCurrent[iLib].idx!=INVALID_INDEX) {
				if (iCurrent[iLib].idx>=narticles(iLib) || iCurrent[iLib].idx<=0)
					continue;
			}
			if (GetWordPrev(iCurrent[iLib].synidx, pidx, iLib, false, isClt)) {
				if (isClt)
					word = poGetCltSynonymWord(pidx,iLib);
				else
					word = poGetSynonymWord(pidx,iLib);
				if (strcmp(poCurrentWord, word) == 0) {
					iCurrent[iLib].synidx=pidx;
				}
			}
		}
		if (isLib) {
			GetWordPrev(iCurrent[iCurrentLib].idx, pidx, iCurrentLib, true, isClt);
			iCurrent[iCurrentLib].idx = pidx;
		} else {
			GetWordPrev(iCurrent[iCurrentLib].synidx, pidx, iCurrentLib, false, isClt);
			iCurrent[iCurrentLib].synidx = pidx;
		}
	}
	return poCurrentWord;
}

bool Libs::LookupSynonymSimilarWord(const gchar* sWord, glong &iSynonymWordIndex, int iLib)
{
	bool isClt = EnableCollation;

	if (oLib[iLib]->syn_file.get() == NULL)
		return false;

	glong iIndex;
	bool bFound=false;
	gchar *casestr;
	bool bLookup;

	if (!bFound) {
		// to lower case.
		casestr = g_utf8_strdown(sWord, -1);
		if (strcmp(casestr, sWord)) {
			if (isClt)
				bLookup = oLib[iLib]->LookupCltSynonym(casestr, iIndex);
			else
				bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex);
			if(bLookup)
				bFound=true;
		}
		g_free(casestr);
		// to upper case.
		if (!bFound) {
			casestr = g_utf8_strup(sWord, -1);
			if (strcmp(casestr, sWord)) {
				if (isClt)
					bLookup = oLib[iLib]->LookupCltSynonym(casestr, iIndex);
				else
					bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex);
				if(bLookup)
					bFound=true;
			}
			g_free(casestr);
		}
		// Upper the first character and lower others.
		if (!bFound) {
			gchar *nextchar = g_utf8_next_char(sWord);
			gchar *firstchar = g_utf8_strup(sWord, nextchar - sWord);
			nextchar = g_utf8_strdown(nextchar, -1);
			casestr = g_strdup_printf("%s%s", firstchar, nextchar);
			g_free(firstchar);
			g_free(nextchar);
			if (strcmp(casestr, sWord)) {
				if (isClt)
					bLookup = oLib[iLib]->LookupCltSynonym(casestr, iIndex);
				else
					bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex);
				if(bLookup)
					bFound=true;
			}
			g_free(casestr);
		}
		if (!bFound) {
			iIndex = iSynonymWordIndex;
			glong pidx;
			const gchar *cword;
			do {
				if (GetWordPrev(iIndex, pidx, iLib, false, isClt)) {
					if (isClt)
						cword = poGetCltSynonymWord(pidx, iLib);
					else
						cword = poGetSynonymWord(pidx, iLib);
					if (stardict_casecmp(cword, sWord, isClt, CollateFunction)==0) {
						iIndex = pidx;
						bFound=true;
					} else {
						break;
					}
				} else {
					break;
				}
			} while (true);
			if (!bFound) {
				if (iIndex!=INVALID_INDEX) {
					if (isClt)
						cword = poGetCltSynonymWord(iIndex, iLib);
					else
						cword = poGetSynonymWord(iIndex, iLib);
					if (stardict_casecmp(cword, sWord, isClt, CollateFunction)==0) {
						bFound=true;
					}
				}
			}
		}
	}
	if (bFound)
		iSynonymWordIndex = iIndex;
	return bFound;
}

bool Libs::LookupSimilarWord(const gchar* sWord, glong & iWordIndex, int iLib)
{
	glong iIndex;
	bool bFound=false;
	gchar *casestr;
	bool isClt = EnableCollation;

	if (!bFound) {
		// to lower case.
		casestr = g_utf8_strdown(sWord, -1);
		if (strcmp(casestr, sWord)) {
			if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
				bFound=true;
		}
		g_free(casestr);
		// to upper case.
		if (!bFound) {
			casestr = g_utf8_strup(sWord, -1);
			if (strcmp(casestr, sWord)) {
				if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
					bFound=true;
			}
			g_free(casestr);
		}
		// Upper the first character and lower others.
		if (!bFound) {
			gchar *nextchar = g_utf8_next_char(sWord);
			gchar *firstchar = g_utf8_strup(sWord, nextchar - sWord);
			nextchar = g_utf8_strdown(nextchar, -1);
			casestr = g_strdup_printf("%s%s", firstchar, nextchar);
			g_free(firstchar);
			g_free(nextchar);
			if (strcmp(casestr, sWord)) {
				if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
					bFound=true;
			}
			g_free(casestr);
		}
		if (!bFound) {
			iIndex = iWordIndex;
			glong pidx;
			const gchar *cword;
			do {
				if (GetWordPrev(iIndex, pidx, iLib, true, isClt)) {
					if (isClt)
						cword = poGetCltWord(pidx, iLib);
					else
						cword = poGetWord(pidx, iLib);
					if (stardict_casecmp(cword, sWord, isClt, CollateFunction)==0) {
						iIndex = pidx;
						bFound=true;
					} else {
						break;
					}
				} else {
					break;
				}
			} while (true);
			if (!bFound) {
				if (iIndex!=INVALID_INDEX) {
					if (isClt)
						cword = poGetCltWord(iIndex, iLib);
					else
						cword = poGetWord(iIndex, iLib);
					if (stardict_casecmp(cword, sWord, isClt, CollateFunction)==0) {
						bFound=true;
					}
				}
			}
		}
	}

	if (bIsPureEnglish(sWord)) {
		// If not Found , try other status of sWord.
		int iWordLen=strlen(sWord);
		bool isupcase;

		gchar *sNewWord = (gchar *)g_malloc(iWordLen + 1);

		//cut one char "s" or "d"
		if(!bFound && iWordLen>1) {
			isupcase = sWord[iWordLen-1]=='S' || !strncmp(&sWord[iWordLen-2],"ED",2);
			if (isupcase || sWord[iWordLen-1]=='s' || !strncmp(&sWord[iWordLen-2],"ed",2)) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-1]='\0'; // cut "s" or "d"
				if (oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		//cut "ly"
		if(!bFound && iWordLen>2) {
			isupcase = !strncmp(&sWord[iWordLen-2],"LY",2);
			if (isupcase || (!strncmp(&sWord[iWordLen-2],"ly",2))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';  // cut "ly"
				if (iWordLen>5 && sNewWord[iWordLen-3]==sNewWord[iWordLen-4]
				    && !bIsVowel(sNewWord[iWordLen-4]) &&
				    bIsVowel(sNewWord[iWordLen-5])) {//doubled

					sNewWord[iWordLen-3]='\0';
					if(oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if (oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
								bFound=true;
						}
						g_free(casestr);
					}
				}
			}
		}

		//cut "ing"
		if(!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3],"ING",3);
			if (isupcase || !strncmp(&sWord[iWordLen-3],"ing",3) ) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if ( iWordLen>6 && (sNewWord[iWordLen-4]==sNewWord[iWordLen-5])
				     && !bIsVowel(sNewWord[iWordLen-5]) &&
				     bIsVowel(sNewWord[iWordLen-6])) {  //doubled
					sNewWord[iWordLen-4]='\0';
					if (oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-4]=sNewWord[iWordLen-5];  //restore
					}
				}
				if( !bFound ) {
					if (oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
								bFound=true;
						}
						g_free(casestr);
					}
				}
				if(!bFound) {
					if (isupcase)
						strcat(sNewWord,"E"); // add a char "E"
					else
						strcat(sNewWord,"e"); // add a char "e"
					if(oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
								bFound=true;
						}
						g_free(casestr);
					}
				}
			}
		}

		//cut two char "es"
		if(!bFound && iWordLen>3) {
			isupcase = (!strncmp(&sWord[iWordLen-2],"ES",2) &&
				    (sWord[iWordLen-3] == 'S' ||
				     sWord[iWordLen-3] == 'X' ||
				     sWord[iWordLen-3] == 'O' ||
				     (iWordLen >4 && sWord[iWordLen-3] == 'H' &&
				      (sWord[iWordLen-4] == 'C' ||
				       sWord[iWordLen-4] == 'S'))));
			if (isupcase ||
			    (!strncmp(&sWord[iWordLen-2],"es",2) &&
			     (sWord[iWordLen-3] == 's' || sWord[iWordLen-3] == 'x' ||
			      sWord[iWordLen-3] == 'o' ||
			      (iWordLen >4 && sWord[iWordLen-3] == 'h' &&
			       (sWord[iWordLen-4] == 'c' || sWord[iWordLen-4] == 's'))))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';
				if(oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		//cut "ed"
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-2],"ED",2);
			if (isupcase || !strncmp(&sWord[iWordLen-2],"ed",2)) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';
				if (iWordLen>5 && (sNewWord[iWordLen-3]==sNewWord[iWordLen-4])
				    && !bIsVowel(sNewWord[iWordLen-4]) &&
				    bIsVowel(sNewWord[iWordLen-5])) {//doubled
					sNewWord[iWordLen-3]='\0';
					if (oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if (oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
								bFound=true;
						}
						g_free(casestr);
					}
				}
			}
		}

		// cut "ied" , add "y".
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3],"IED",3);
			if (isupcase || (!strncmp(&sWord[iWordLen-3],"ied",3))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if (isupcase)
					strcat(sNewWord,"Y"); // add a char "Y"
				else
					strcat(sNewWord,"y"); // add a char "y"
				if (oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		// cut "ies" , add "y".
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3],"IES",3);
			if (isupcase || (!strncmp(&sWord[iWordLen-3],"ies",3))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if (isupcase)
					strcat(sNewWord,"Y"); // add a char "Y"
				else
					strcat(sNewWord,"y"); // add a char "y"
				if(oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		// cut "er".
		if (!bFound && iWordLen>2) {
			isupcase = !strncmp(&sWord[iWordLen-2],"ER",2);
			if (isupcase || (!strncmp(&sWord[iWordLen-2],"er",2))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';
				if(oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		// cut "est".
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3], "EST", 3);
			if (isupcase || (!strncmp(&sWord[iWordLen-3],"est", 3))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if(oLib[iLib]->Lookup2(sNewWord, iIndex, isClt))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup2(casestr, iIndex, isClt))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		g_free(sNewWord);
	}

	if (bFound)
		iWordIndex = iIndex;
#if 0
	else {
		//don't change iWordIndex here.
		//when LookupSimilarWord all failed too, we want to use the old LookupWord index to list words.
		//iWordIndex = INVALID_INDEX;
	}
#endif
	return bFound;
}

bool Libs::SimpleLookupWord(const gchar* sWord, glong & iWordIndex, int iLib)
{
	bool bFound;

	if (EnableCollation)
		bFound = oLib[iLib]->LookupClt(sWord, iWordIndex);
	else
		bFound = oLib[iLib]->Lookup(sWord, iWordIndex);
	if (!bFound)
		bFound = LookupSimilarWord(sWord, iWordIndex, iLib);
	return bFound;
}

bool Libs::SimpleLookupSynonymWord(const gchar* sWord, glong & iWordIndex, int iLib)
{
	bool bFound;
	if (EnableCollation)
		bFound = oLib[iLib]->LookupCltSynonym(sWord, iWordIndex);
	else
		bFound = oLib[iLib]->LookupSynonym(sWord, iWordIndex);
	if (!bFound)
		bFound = LookupSynonymSimilarWord(sWord, iWordIndex, iLib);
	return bFound;
}

struct Fuzzystruct {
	char * pMatchWord;
	int iMatchWordDistance;
};

inline bool operator<(const Fuzzystruct & lh, const Fuzzystruct & rh) {
	if (lh.iMatchWordDistance!=rh.iMatchWordDistance)
		return lh.iMatchWordDistance<rh.iMatchWordDistance;

	if (lh.pMatchWord && rh.pMatchWord)
		return stardict_strcmp(lh.pMatchWord, rh.pMatchWord)<0;

	return false;
}

static inline void unicode_strdown(gunichar *str)
{
	while (*str) {
		*str=g_unichar_tolower(*str);
		++str;
	}
}

bool Libs::LookupWithFuzzy(const gchar *sWord, gchar *reslist[], gint reslist_size)
{
	if (sWord[0] == '\0')
		return false;

	Fuzzystruct oFuzzystruct[reslist_size];

	for (int i=0; i<reslist_size; i++) {
		oFuzzystruct[i].pMatchWord = NULL;
		oFuzzystruct[i].iMatchWordDistance = iMaxFuzzyDistance;
	}
	int iMaxDistance = iMaxFuzzyDistance;
	int iDistance;
	bool Found = false;
	EditDistance oEditDistance;

	glong iCheckWordLen;
	const char *sCheck;
	gunichar *ucs4_str1, *ucs4_str2;
	glong ucs4_str2_len;

	ucs4_str2 = g_utf8_to_ucs4_fast(sWord, -1, &ucs4_str2_len);
	unicode_strdown(ucs4_str2);

	for (std::vector<Dict *>::size_type iLib=0; iLib<oLib.size(); iLib++) {
		for (gint synLib=0; synLib<2; synLib++) {
			if (synLib==1) {
				if (oLib[iLib]->syn_file.get()==NULL)
					break;
			}
			show_progress->notify_about_work();

			//if (stardict_strcmp(sWord, poGetWord(0,iLib))>=0 && stardict_strcmp(sWord, poGetWord(narticles(iLib)-1,iLib))<=0) {
			//there are Chinese dicts and English dicts...
			if (TRUE) {
				glong iwords;
				if (synLib==0)
					iwords = narticles(iLib);
				else
					iwords = nsynarticles(iLib);
				for (glong index=0; index<iwords; index++) {
					// Need to deal with same word in index? But this will slow down processing in most case.
					if (synLib==0)
						sCheck = poGetWord(index,iLib);
					else
						sCheck = poGetSynonymWord(index,iLib);
					// tolower and skip too long or too short words
					iCheckWordLen = g_utf8_strlen(sCheck, -1);
					if (iCheckWordLen-ucs4_str2_len>=iMaxDistance ||
					    ucs4_str2_len-iCheckWordLen>=iMaxDistance)
						continue;
					ucs4_str1 = g_utf8_to_ucs4_fast(sCheck, -1, NULL);
					if (iCheckWordLen > ucs4_str2_len)
						ucs4_str1[ucs4_str2_len]=0;
					unicode_strdown(ucs4_str1);

					iDistance = oEditDistance.CalEditDistance(ucs4_str1, ucs4_str2, iMaxDistance);
					g_free(ucs4_str1);
					if (iDistance<iMaxDistance && iDistance < ucs4_str2_len) {
						// when ucs4_str2_len=1,2 we need less fuzzy.
						Found = true;
						bool bAlreadyInList = false;
						int iMaxDistanceAt=0;
						for (int j=0; j<reslist_size; j++) {
							if (oFuzzystruct[j].pMatchWord &&
							    strcmp(oFuzzystruct[j].pMatchWord,sCheck)==0 ) {//already in list
								bAlreadyInList = true;
								break;
							}
							//find the position,it will certainly be found (include the first time) as iMaxDistance is set by last time.
							if (oFuzzystruct[j].iMatchWordDistance == iMaxDistance ) {
								iMaxDistanceAt = j;
							}
						}
						if (!bAlreadyInList) {
							if (oFuzzystruct[iMaxDistanceAt].pMatchWord)
								g_free(oFuzzystruct[iMaxDistanceAt].pMatchWord);
							oFuzzystruct[iMaxDistanceAt].pMatchWord = g_strdup(sCheck);
							oFuzzystruct[iMaxDistanceAt].iMatchWordDistance = iDistance;
							// calc new iMaxDistance
							iMaxDistance = iDistance;
							for (int j=0; j<reslist_size; j++) {
								if (oFuzzystruct[j].iMatchWordDistance > iMaxDistance)
									iMaxDistance = oFuzzystruct[j].iMatchWordDistance;
							} // calc new iMaxDistance
						}   // add to list
					}   // find one
				}   // each word
			}   // ok for search
		}  // synLib
	}   // each lib
	g_free(ucs4_str2);

	if (Found)// sort with distance
		std::sort(oFuzzystruct, oFuzzystruct+reslist_size);

	for (gint i=0; i<reslist_size; ++i)
		reslist[i]=oFuzzystruct[i].pMatchWord;

	return Found;
}

inline bool less_for_compare(const char *lh, const char *rh) {
	return stardict_strcmp(lh, rh)<0;
}

gint Libs::LookupWithRule(const gchar *word, gchar **ppMatchWord)
{
	glong aiIndex[MAX_MATCH_ITEM_PER_LIB+1];
	gint iMatchCount = 0;
	GPatternSpec *pspec = g_pattern_spec_new(word);

	const gchar * sMatchWord;
	bool bAlreadyInList;
	for (std::vector<Dict *>::size_type iLib=0; iLib<oLib.size(); iLib++) {
		//if(oLibs.LookdupWordsWithRule(pspec,aiIndex,MAX_MATCH_ITEM_PER_LIB+1-iMatchCount,iLib))
		// -iMatchCount,so save time,but may got less result and the word may repeat.

		if (oLib[iLib]->LookupWithRule(pspec, aiIndex, MAX_MATCH_ITEM_PER_LIB+1)) {
			show_progress->notify_about_work();
			for (int i=0; aiIndex[i]!=-1; i++) {
				sMatchWord = poGetWord(aiIndex[i],iLib);
				bAlreadyInList = false;
				for (int j=0; j<iMatchCount; j++) {
					if (strcmp(ppMatchWord[j],sMatchWord)==0) {//already in list
						bAlreadyInList = true;
						break;
					}
				}
				if (!bAlreadyInList)
					ppMatchWord[iMatchCount++] = g_strdup(sMatchWord);
			}
		}
		if (oLib[iLib]->LookupWithRuleSynonym(pspec, aiIndex, MAX_MATCH_ITEM_PER_LIB+1)) {
			show_progress->notify_about_work();
			for (int i=0; aiIndex[i]!=-1; i++) {
				sMatchWord = poGetSynonymWord(aiIndex[i],iLib);
				bAlreadyInList = false;
				for (int j=0; j<iMatchCount; j++) {
					if (strcmp(ppMatchWord[j],sMatchWord)==0) {//already in list
						bAlreadyInList = true;
						break;
					}
				}
				if (!bAlreadyInList)
					ppMatchWord[iMatchCount++] = g_strdup(sMatchWord);
			}
		}
	}
	g_pattern_spec_free(pspec);

	if (iMatchCount)// sort it.
		std::sort(ppMatchWord, ppMatchWord+iMatchCount, less_for_compare);
	return iMatchCount;
}

bool Libs::LookupData(const gchar *sWord, std::vector<gchar *> *reslist, updateSearchDialog_func search_func, gpointer search_data, bool *cancel)
{
	std::vector<std::string> SearchWords;
	std::string SearchWord;
        const char *p=sWord;
        while (*p) {
                if (*p=='\\') {
                        p++;
			switch (*p) {
			case ' ':
				SearchWord+=' ';
				break;
			case '\\':
				SearchWord+='\\';
				break;
			case 't':
				SearchWord+='\t';
				break;
			case 'n':
				SearchWord+='\n';
				break;
			default:
				SearchWord+=*p;
			}
                } else if (*p == ' ') {
			if (!SearchWord.empty()) {
				SearchWords.push_back(SearchWord);
				SearchWord.clear();
			}
		} else {
			SearchWord+=*p;
		}
		p++;
        }
	if (!SearchWord.empty()) {
		SearchWords.push_back(SearchWord);
		SearchWord.clear();
	}
	if (SearchWords.empty())
		return false;

	glong search_count=0;
	glong total_count=0;
	if (search_func) {
		for (std::vector<Dict *>::size_type i=0; i<oLib.size(); ++i) {
			total_count += narticles(i);
		}
	}

	guint32 max_size =0;
	gchar *origin_data = NULL;
	for (std::vector<Dict *>::size_type i=0; i<oLib.size(); ++i) {
		if (!oLib[i]->containSearchData())
			continue;
		const gulong iwords = narticles(i);
		const gchar *key;
		guint32 offset, size;
		for (gulong j=0; j<iwords; ++j) {
			if (search_func) {
				if (*cancel)
					goto search_out;
				if (search_count % 10000 == 0) {
					search_func(search_data, (gdouble)search_count/(gdouble)total_count);
				}
				search_count++;
			}
			oLib[i]->get_key_and_data(j, &key, &offset, &size);
			if (size>max_size) {
				origin_data = (gchar *)g_realloc(origin_data, size);
				max_size = size;
			}
			if (oLib[i]->SearchData(SearchWords, offset, size, origin_data)) {
				if (reslist[i].empty() || strcmp(reslist[i].back(), key))
					reslist[i].push_back(g_strdup(key));
			}
		}
	}
search_out:
	g_free(origin_data);
	KMP_end();

	std::vector<Dict *>::size_type i;
	for (i=0; i<oLib.size(); ++i)
		if (!reslist[i].empty())
			break;

	return i!=oLib.size();
}

