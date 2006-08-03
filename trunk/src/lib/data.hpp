#ifndef _DATA_HPP_
#define _DATA_HPP_

#include <glib.h>
#include <vector>
#include <string>
#include <memory>

#include "dictziplib.hpp"

struct cacheItem {
  guint32 offset;
	gchar *data;
  //write code here to make it inline
  cacheItem() {data= NULL;}
  ~cacheItem() {g_free(data);}
};

const int WORDDATA_CACHE_NUM = 10;
const int UNSET_INDEX = -1;
const int INVALID_INDEX=-100;

class DictBase {
public:
	DictBase();
	~DictBase();
	gchar * GetWordData(guint32 idxitem_offset, guint32 idxitem_size);
	bool containSearchData() {
		if (sametypesequence.empty())
			return true;

		return sametypesequence.find_first_of("mlgxtykw") !=
			std::string::npos;
	}
	bool SearchData(std::vector<std::string> &SearchWords, guint32 idxitem_offset, guint32 idxitem_size, gchar *origin_data);
protected:
	std::string sametypesequence;
	FILE *dictfile;
	std::auto_ptr<dictData> dictdzfile;
private:
  cacheItem cache[WORDDATA_CACHE_NUM];
	gint cache_cur;
};

#endif//!_DATA_HPP_
