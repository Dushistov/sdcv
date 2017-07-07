#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cctype>

#include <sys/stat.h>
#include <zlib.h>
#include <glib/gstdio.h>

#include "distance.hpp"
#include "mapfile.hpp"
#include "utils.hpp"

#include "stardict_lib.hpp"


#define TO_STR2(xstr) #xstr
#define TO_STR1(xstr) TO_STR2(xstr)

#define THROW_IF_ERROR(expr) do {                                   \
        assert((expr));                                             \
        if (!(expr))                                                \
        throw std::runtime_error(#expr " not true at " __FILE__ ": " TO_STR1(__LINE__));  \
    } while (false)

// Notice: read src/tools/DICTFILE_FORMAT for the dictionary
// file's format information!

namespace {
    struct Fuzzystruct {
        char * pMatchWord;
        int iMatchWordDistance;
    };

    static inline bool bIsVowel(gchar inputchar)
    {
        gchar ch = g_ascii_toupper(inputchar);
        return( ch=='A' || ch=='E' || ch=='I' || ch=='O' || ch=='U' );
    }

    static bool bIsPureEnglish(const gchar *str)
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

    static inline gint stardict_strcmp(const gchar *s1, const gchar *s2)
    {
        const gint a = g_ascii_strcasecmp(s1, s2);
        if (a == 0)
            return strcmp(s1, s2);
        else
            return a;
    }

    static void unicode_strdown(gunichar *str)
    {
        while (*str) {
            *str = g_unichar_tolower(*str);
            ++str;
        }
    }

}

bool DictInfo::load_from_ifo_file(const std::string& ifofilename,
                                  bool istreedict)
{
  ifo_file_name = ifofilename;
  glib::CharStr buffer;
  if (!g_file_get_contents(ifofilename.c_str(), get_addr(buffer), nullptr, nullptr))
    return false;

  static const char TREEDICT_MAGIC_DATA[] = "StarDict's treedict ifo file";
  static const char DICT_MAGIC_DATA[] = "StarDict's dict ifo file";

  const gchar *magic_data = istreedict ? TREEDICT_MAGIC_DATA : DICT_MAGIC_DATA;
  static const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF, '\0'};
  if (!g_str_has_prefix(
          g_str_has_prefix(get_impl(buffer), (const gchar *)(utf8_bom)) ? get_impl(buffer) + 3 : get_impl(buffer),
          magic_data)) {
    return false;
  }

  gchar *p1 = get_impl(buffer) + strlen(magic_data)-1;

  gchar *p2 = strstr(p1, "\nwordcount=");
  if (p2 == nullptr)
    return false;  

  gchar *p3 = strchr(p2 + sizeof("\nwordcount=") - 1, '\n');

  wordcount = atol(std::string(p2+sizeof("\nwordcount=")-1, p3-(p2+sizeof("\nwordcount=")-1)).c_str());

  if (istreedict) {
    p2 = strstr(p1,"\ntdxfilesize=");
    if (p2 == nullptr)
      return false;

    p3 = strchr(p2+ sizeof("\ntdxfilesize=")-1,'\n');

    index_file_size = atol(std::string(p2+sizeof("\ntdxfilesize=")-1, p3-(p2+sizeof("\ntdxfilesize=")-1)).c_str());

  } else {

    p2 = strstr(p1,"\nidxfilesize=");
    if (p2 == nullptr)
      return false;

    p3 = strchr(p2+ sizeof("\nidxfilesize=")-1,'\n');
    index_file_size = atol(std::string(p2+sizeof("\nidxfilesize=")-1, p3-(p2+sizeof("\nidxfilesize=")-1)).c_str());
  }

  p2 = strstr(p1,"\nbookname=");

  if (p2 == nullptr)
    return false;

  p2 = p2 + sizeof("\nbookname=") -1;
  p3 = strchr(p2, '\n');
  bookname.assign(p2, p3-p2);

  p2 = strstr(p1,"\nauthor=");
  if (p2) {
    p2 = p2 + sizeof("\nauthor=") -1;
    p3 = strchr(p2, '\n');
    author.assign(p2, p3-p2);
  }

  p2 = strstr(p1,"\nemail=");
  if (p2) {
    p2 = p2 + sizeof("\nemail=") -1;
    p3 = strchr(p2, '\n');
    email.assign(p2, p3-p2);
  }

  p2 = strstr(p1,"\nwebsite=");
  if (p2) {
    p2 = p2 + sizeof("\nwebsite=") -1;
    p3 = strchr(p2, '\n');
    website.assign(p2, p3-p2);
  }

  p2 = strstr(p1,"\ndate=");
  if (p2) {
    p2 = p2 + sizeof("\ndate=") -1;
    p3 = strchr(p2, '\n');
    date.assign(p2, p3-p2);
  }

  p2 = strstr(p1,"\ndescription=");
  if (p2) {
    p2 = p2 + sizeof("\ndescription=")-1;
    p3 = strchr(p2, '\n');
    description.assign(p2, p3-p2);
  }

  p2 = strstr(p1,"\nsametypesequence=");
  if (p2) {
    p2+=sizeof("\nsametypesequence=")-1;
    p3 = strchr(p2, '\n');
    sametypesequence.assign(p2, p3-p2);
  }

  p2 = strstr(p1,"\nsynwordcount=");
  syn_wordcount = 0;
  if (p2) {
    p2+=sizeof("\nsynwordcount=")-1;
    p3 = strchr(p2, '\n');
    syn_wordcount = atol(std::string(p2, p3-p2).c_str());
  }

  return true;
}

gchar* DictBase::GetWordData(guint32 idxitem_offset, guint32 idxitem_size)
{
  for (int i=0; i<WORDDATA_CACHE_NUM; i++)
    if (cache[i].data && cache[i].offset == idxitem_offset)
      return cache[i].data;

  if (dictfile)
    fseek(dictfile, idxitem_offset, SEEK_SET);

  gchar *data;
  if (!sametypesequence.empty()) {
      glib::CharStr origin_data((gchar *)g_malloc(idxitem_size));

      if (dictfile) {
          const size_t nitems = fread(get_impl(origin_data), idxitem_size, 1, dictfile);
          THROW_IF_ERROR(nitems == 1);
      } else
          dictdzfile->read(get_impl(origin_data), idxitem_offset, idxitem_size);

    guint32 data_size;
    gint sametypesequence_len = sametypesequence.length();
    //there have sametypesequence_len char being omitted.
    data_size = idxitem_size + sizeof(guint32) + sametypesequence_len;
    //if the last item's size is determined by the end up '\0',then +=sizeof(gchar);
    //if the last item's size is determined by the head guint32 type data,then +=sizeof(guint32);
    switch (sametypesequence[sametypesequence_len-1]) {
    case 'm':
    case 't':
    case 'y':
    case 'l':
    case 'g':
    case 'x':
    case 'k':
      data_size += sizeof(gchar);
      break;
    case 'W':
    case 'P':
      data_size += sizeof(guint32);
      break;
    default:
      if (g_ascii_isupper(sametypesequence[sametypesequence_len-1]))
        data_size += sizeof(guint32);
      else
        data_size += sizeof(gchar);
      break;
    }
    data = (gchar *)g_malloc(data_size);
    gchar *p1,*p2;
    p1 = data + sizeof(guint32);
    p2 = get_impl(origin_data);
    guint32 sec_size;
    //copy the head items.
    for (int i=0; i<sametypesequence_len-1; i++) {
      *p1=sametypesequence[i];
      p1+=sizeof(gchar);
      switch (sametypesequence[i]) {
      case 'm':
      case 't':
      case 'y':
      case 'l':
      case 'g':
      case 'x':
      case 'k':
				sec_size = strlen(p2)+1;
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
				break;
      case 'W':
      case 'P':
				sec_size = get_uint32(p2);
				sec_size += sizeof(guint32);
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
				break;
      default:
				if (g_ascii_isupper(sametypesequence[i])) {
					sec_size = get_uint32(p2);
					sec_size += sizeof(guint32);
				} else {
					sec_size = strlen(p2)+1;
				}
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
				break;
      }
    }
    //calculate the last item 's size.
    sec_size = idxitem_size - (p2-get_impl(origin_data));
    *p1=sametypesequence[sametypesequence_len-1];
    p1+=sizeof(gchar);
    switch (sametypesequence[sametypesequence_len-1]) {
    case 'm':
    case 't':
    case 'y':
    case 'l':
    case 'g':
    case 'x':
    case 'k':
      memcpy(p1, p2, sec_size);
      p1 += sec_size;
      *p1='\0';//add the end up '\0';
      break;
    case 'W':
    case 'P':
	    set_uint32(p1, sec_size);
      p1 += sizeof(guint32);
      memcpy(p1, p2, sec_size);
      break;
    default:
      if (g_ascii_isupper(sametypesequence[sametypesequence_len-1])) {
	      set_uint32(p1, sec_size);
        p1 += sizeof(guint32);
        memcpy(p1, p2, sec_size);
      } else {
        memcpy(p1, p2, sec_size);
        p1 += sec_size;
        *p1='\0';
      }
      break;
    }
    set_uint32(data, data_size);
  } else {
    data = (gchar *)g_malloc(idxitem_size + sizeof(guint32));
    if (dictfile) {
      const size_t nitems = fread(data+sizeof(guint32), idxitem_size, 1, dictfile);
      THROW_IF_ERROR(nitems == 1);
    } else
      dictdzfile->read(data+sizeof(guint32), idxitem_offset, idxitem_size);
    set_uint32(data, idxitem_size+sizeof(guint32));
  }
  g_free(cache[cache_cur].data);

  cache[cache_cur].data = data;
  cache[cache_cur].offset = idxitem_offset;
  cache_cur++;
  if (cache_cur==WORDDATA_CACHE_NUM)
    cache_cur = 0;
  return data;
}

bool DictBase::SearchData(std::vector<std::string> &SearchWords, guint32 idxitem_offset, guint32 idxitem_size, gchar *origin_data)
{
	int nWord = SearchWords.size();
	std::vector<bool> WordFind(nWord, false);
	int nfound=0;

	if (dictfile)
		fseek(dictfile, idxitem_offset, SEEK_SET);
	if (dictfile) {
		const size_t nitems = fread(origin_data, idxitem_size, 1, dictfile);
        THROW_IF_ERROR(nitems == 1);
	} else
		dictdzfile->read(origin_data, idxitem_offset, idxitem_size);
	gchar *p = origin_data;
	guint32 sec_size;
	int j;
	if (!sametypesequence.empty()) {
		gint sametypesequence_len = sametypesequence.length();
		for (int i=0; i<sametypesequence_len-1; i++) {
			switch (sametypesequence[i]) {
			case 'm':
			case 't':
			case 'y':
			case 'l':
			case 'g':
			case 'x':
            case 'k':
				for (j=0; j<nWord; j++)
					if (!WordFind[j] && strstr(p, SearchWords[j].c_str())) {
							WordFind[j] = true;
							++nfound;
					}


				if (nfound==nWord)
					return true;
				sec_size = strlen(p)+1;
				p+=sec_size;
				break;
			default:
				if (g_ascii_isupper(sametypesequence[i])) {
					sec_size = get_uint32(p);
					sec_size += sizeof(guint32);
				} else {
					sec_size = strlen(p)+1;
				}
				p+=sec_size;
			}
		}
		switch (sametypesequence[sametypesequence_len-1]) {
		case 'm':
		case 't':
		case 'y':
		case 'l':
		case 'g':
		case 'x':
        case 'k':
			sec_size = idxitem_size - (p-origin_data);
			for (j=0; j<nWord; j++)
				if (!WordFind[j] &&
				    g_strstr_len(p, sec_size, SearchWords[j].c_str())) {
						WordFind[j] = true;
						++nfound;
				}


			if (nfound==nWord)
				return true;
			break;
		}
	} else {
		while (guint32(p - origin_data)<idxitem_size) {
			switch (*p) {
			case 'm':
			case 't':
			case 'y':
			case 'l':
			case 'g':
			case 'x':
            case 'k':
				for (j=0; j<nWord; j++)
					if (!WordFind[j] && strstr(p, SearchWords[j].c_str())) {
							WordFind[j] = true;
							++nfound;
					}

				if (nfound==nWord)
					return true;
				sec_size = strlen(p)+1;
				p+=sec_size;
				break;
                        default:
                                if (g_ascii_isupper(*p)) {
                                        sec_size = get_uint32(p);
					sec_size += sizeof(guint32);
                                } else {
                                        sec_size = strlen(p)+1;
                                }
                                p+=sec_size;
			}
		}
	}
	return false;
}

namespace {
    class OffsetIndex : public IIndexFile {
    public:
        OffsetIndex() : idxfile(nullptr) {}
        ~OffsetIndex() {
            if (idxfile)
                fclose(idxfile);
        }
        bool load(const std::string& url, gulong wc, gulong fsize, bool verbose) override;
        const gchar *get_key(glong idx) override;
        void get_data(glong idx) override { get_key(idx); }
        const gchar *get_key_and_data(glong idx) override {
            return get_key(idx);
        }
        bool lookup(const char *str, glong &idx) override;
    private:
        static const gint ENTR_PER_PAGE = 32;
        static const char *CACHE_MAGIC;

        std::vector<guint32> wordoffset;
        FILE *idxfile;
        gulong wordcount;

        gchar wordentry_buf[256+sizeof(guint32)*2]; // The length of "word_str" should be less than 256. See src/tools/DICTFILE_FORMAT.
        struct index_entry {
            glong idx;
            std::string keystr;
            void assign(glong i, const std::string& str) {
                idx = i;
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
            glong idx = -1;
            page_entry entries[ENTR_PER_PAGE];

            page_t() {}
            void fill(gchar *data, gint nent, glong idx_);
        } page;
        gulong load_page(glong page_idx);
        const gchar *read_first_on_page_key(glong page_idx);
        const gchar *get_first_on_page_key(glong page_idx);
        bool load_cache(const std::string& url);
        bool save_cache(const std::string& url, bool verbose);
        static std::list<std::string> get_cache_variant(const std::string& url);
    };

    const char *OffsetIndex::CACHE_MAGIC = "StarDict's Cache, Version: 0.1";


    class WordListIndex : public IIndexFile {
    public:
        WordListIndex() : idxdatabuf(nullptr)	{}
        ~WordListIndex() { g_free(idxdatabuf); }
        bool load(const std::string& url, gulong wc, gulong fsize, bool verbose) override;
        const gchar *get_key(glong idx) override { return wordlist[idx]; }
        void get_data(glong idx) override;
        const gchar *get_key_and_data(glong idx) override {
            get_data(idx);
            return get_key(idx);
        }
        bool lookup(const char *str, glong &idx) override;
    private:
        gchar *idxdatabuf;
        std::vector<gchar *> wordlist;
    };

    void OffsetIndex::page_t::fill(gchar *data, gint nent, glong idx_)
    {
        idx=idx_;
        gchar *p=data;
        glong len;
        for (gint i=0; i<nent; ++i) {
            entries[i].keystr=p;
            len=strlen(p);
            p+=len+1;
            entries[i].off=g_ntohl(get_uint32(p));
            p+=sizeof(guint32);
            entries[i].size=g_ntohl(get_uint32(p));
            p+=sizeof(guint32);
        }
    }

    inline const gchar *OffsetIndex::read_first_on_page_key(glong page_idx)
    {
        fseek(idxfile, wordoffset[page_idx], SEEK_SET);
        guint32 page_size = wordoffset[page_idx + 1] - wordoffset[page_idx];
        const size_t nitems = fread(wordentry_buf,
                                    std::min(sizeof(wordentry_buf), static_cast<size_t>(page_size)),
                                    1, idxfile);
        THROW_IF_ERROR(nitems == 1);
        //TODO: check returned values, deal with word entry that strlen>255.
        return wordentry_buf;
    }

    inline const gchar *OffsetIndex::get_first_on_page_key(glong page_idx)
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

    bool OffsetIndex::load_cache(const std::string& url)
    {
        const std::list<std::string> vars = get_cache_variant(url);

        for (const std::string& item : vars) {
            struct ::stat idxstat, cachestat;
            if (g_stat(url.c_str(), &idxstat)!=0 ||
                g_stat(item.c_str(), &cachestat)!=0)
                continue;
            if (cachestat.st_mtime<idxstat.st_mtime)
                continue;
            MapFile mf;
            if (!mf.open(item.c_str(), cachestat.st_size))
                continue;
            if (strncmp(mf.begin(), CACHE_MAGIC, strlen(CACHE_MAGIC))!=0)
                continue;
            memcpy(&wordoffset[0], mf.begin()+strlen(CACHE_MAGIC), wordoffset.size()*sizeof(wordoffset[0]));
            return true;

        }

        return false;
    }

    std::list<std::string> OffsetIndex::get_cache_variant(const std::string& url)
    {
        std::list<std::string> res = {url + ".oft"};
        if (!g_file_test(g_get_user_cache_dir(), G_FILE_TEST_EXISTS) &&
            g_mkdir(g_get_user_cache_dir(), 0700)==-1)
            return res;

        const std::string cache_dir = std::string(g_get_user_cache_dir())+G_DIR_SEPARATOR_S+"sdcv";

        if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_EXISTS)) {
            if (g_mkdir(cache_dir.c_str(), 0700)==-1)
                return res;
        } else if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_IS_DIR))
            return res;

        gchar *base = g_path_get_basename(url.c_str());
        res.push_back(cache_dir+G_DIR_SEPARATOR_S+base+".oft");
        g_free(base);
        return res;
    }

    bool OffsetIndex::save_cache(const std::string& url, bool verbose)
    {
        const std::list<std::string> vars = get_cache_variant(url);
        for (const std::string&  item : vars) {
            FILE *out=fopen(item.c_str(), "wb");
            if (!out)
                continue;
            if (fwrite(CACHE_MAGIC, 1, strlen(CACHE_MAGIC), out)!=strlen(CACHE_MAGIC))
                continue;
            if (fwrite(&wordoffset[0], sizeof(wordoffset[0]), wordoffset.size(), out)!=wordoffset.size())
                continue;
            fclose(out);
            if(verbose) {
              printf("save to cache %s\n", url.c_str());
            }
            return true;
        }
        return false;
    }

    bool OffsetIndex::load(const std::string& url, gulong wc, gulong fsize, bool verbose)
    {
        wordcount=wc;
        gulong npages=(wc-1)/ENTR_PER_PAGE+2;
        wordoffset.resize(npages);
        if (!load_cache(url)) {//map file will close after finish of block
            MapFile map_file;
            if (!map_file.open(url.c_str(), fsize))
                return false;
            const gchar *idxdatabuffer=map_file.begin();

            const gchar *p1 = idxdatabuffer;
            gulong index_size;
            guint32 j=0;
            for (guint32 i=0; i<wc; i++) {
                index_size=strlen(p1) +1 + 2*sizeof(guint32);
                if (i % ENTR_PER_PAGE==0) {
                    wordoffset[j]=p1-idxdatabuffer;
                    ++j;
                }
                p1 += index_size;
            }
            wordoffset[j]=p1-idxdatabuffer;
            if (!save_cache(url, verbose))
                fprintf(stderr, "cache update failed\n");
        }

        if (!(idxfile = fopen(url.c_str(), "rb"))) {
            wordoffset.resize(0);
            return false;
        }

        first.assign(0, read_first_on_page_key(0));
        last.assign(wordoffset.size()-2, read_first_on_page_key(wordoffset.size()-2));
        middle.assign((wordoffset.size()-2)/2, read_first_on_page_key((wordoffset.size()-2)/2));
        real_last.assign(wc-1, get_key(wc-1));

        return true;
    }

    inline gulong OffsetIndex::load_page(glong page_idx)
    {
        gulong nentr = ENTR_PER_PAGE;
        if (page_idx == glong(wordoffset.size()-2))
            if ((nentr = (wordcount % ENTR_PER_PAGE)) == 0)
                nentr = ENTR_PER_PAGE;


        if (page_idx != page.idx) {
            page_data.resize(wordoffset[page_idx+1]-wordoffset[page_idx]);
            fseek(idxfile, wordoffset[page_idx], SEEK_SET);
            const size_t nitems = fread(&page_data[0], 1, page_data.size(), idxfile);
            THROW_IF_ERROR(nitems == page_data.size());
            
            page.fill(&page_data[0], nentr, page_idx);
        }

        return nentr;
    }

    const gchar *OffsetIndex::get_key(glong idx)
    {
        load_page(idx/ENTR_PER_PAGE);
        glong idx_in_page=idx%ENTR_PER_PAGE;
        wordentry_offset=page.entries[idx_in_page].off;
        wordentry_size=page.entries[idx_in_page].size;

        return page.entries[idx_in_page].keystr;
    }

    bool OffsetIndex::lookup(const char *str, glong &idx)
    {
        bool bFound=false;
        glong iFrom;
        glong iTo=wordoffset.size()-2;
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
            gulong netr = load_page(idx);
            iFrom = 1; // Needn't search the first word anymore.
            iTo = netr-1;
            iThisIndex = 0;
            while (iFrom <= iTo) {
                iThisIndex = (iFrom + iTo) / 2;
                cmpint = stardict_strcmp(str, page.entries[iThisIndex].keystr);
                if (cmpint > 0)
                    iFrom = iThisIndex+1;
                else if (cmpint < 0)
                    iTo = iThisIndex-1;
                else {
                    bFound = true;
                    break;
                }
            }
            idx *= ENTR_PER_PAGE;
            if (!bFound)
                idx += iFrom;    //next
            else
                idx += iThisIndex;
        } else {
            idx *= ENTR_PER_PAGE;
        }
        return bFound;
    }

    bool WordListIndex::load(const std::string& url, gulong wc, gulong fsize, bool verbose)
    {
        gzFile in = gzopen(url.c_str(), "rb");
        if (in == nullptr)
            return false;

        idxdatabuf = (gchar *)g_malloc(fsize);

        const int len = gzread(in, idxdatabuf, fsize);
        gzclose(in);
        if (len < 0)
            return false;

        if (gulong(len) != fsize)
            return false;

        wordlist.resize(wc+1);
        gchar *p1 = idxdatabuf;
        guint32 i;
        for (i=0; i<wc; i++) {
            wordlist[i] = p1;
            p1 += strlen(p1) +1 + 2*sizeof(guint32);
        }
        wordlist[wc] = p1;

        return true;
    }

    void WordListIndex::get_data(glong idx)
    {
        gchar *p1 = wordlist[idx]+strlen(wordlist[idx])+sizeof(gchar);
        wordentry_offset = g_ntohl(get_uint32(p1));
        p1 += sizeof(guint32);
        wordentry_size = g_ntohl(get_uint32(p1));
    }

    bool WordListIndex::lookup(const char *str, glong &idx)
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
}

bool SynFile::load(const std::string& url, gulong wc) {
	struct stat stat_buf;
	if(!stat(url.c_str(), &stat_buf)) {
		MapFile syn;
		if(!syn.open(url.c_str(), stat_buf.st_size))
			return false;
		const gchar *current = syn.begin();
		for(unsigned long i = 0; i < wc; i++) {
			// each entry in a syn-file is:
			// - 0-terminated string
			// 4-byte index into .dict file in network byte order
			gchar *lower_string = g_utf8_casefold(current, -1);
			std::string synonym(lower_string);
			g_free(lower_string);
			current += synonym.length()+1;
			unsigned int idx = * reinterpret_cast<const unsigned int*>(current);
			idx = g_ntohl(idx);
			current += sizeof(idx);
			synonyms[synonym] = idx;
		}
		return true;
	} else {
		return false;
	}
}

bool SynFile::lookup(const char *str, glong &idx) {
	gchar *lower_string = g_utf8_casefold(str, -1);
	auto it = synonyms.find(lower_string);
	if(it != synonyms.end()) {
		g_free(lower_string);
		idx = it->second;
		return true;
	}
	g_free(lower_string);
	return false;
}

bool Dict::Lookup(const char *str, glong &idx) {
	if(syn_file->lookup(str, idx)) {
		return true;
	}
	return idx_file->lookup(str, idx);
}

bool Dict::load(const std::string& ifofilename, bool verbose)
{
	gulong idxfilesize;
	if (!load_ifofile(ifofilename, idxfilesize))
		return false;

	std::string fullfilename(ifofilename);
	fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "dict.dz");

	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		dictdzfile.reset(new DictData);
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
		idx_file.reset(new WordListIndex);
	} else {
		fullfilename.erase(fullfilename.length()-sizeof(".gz")+1, sizeof(".gz")-1);
		idx_file.reset(new OffsetIndex);
	}

	if (!idx_file->load(fullfilename, wordcount, idxfilesize, verbose))
		return false;

	fullfilename=ifofilename;
	fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "syn");
	syn_file.reset(new SynFile);
	syn_file->load(fullfilename, syn_wordcount);

	//g_print("bookname: %s , wordcount %lu\n", bookname.c_str(), narticles());
	return true;
}

bool Dict::load_ifofile(const std::string& ifofilename, gulong &idxfilesize)
{
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(ifofilename, false))
		return false;
	if (dict_info.wordcount==0)
		return false;

	ifo_file_name=dict_info.ifo_file_name;
	wordcount=dict_info.wordcount;
	syn_wordcount=dict_info.syn_wordcount;
	bookname=dict_info.bookname;

	idxfilesize=dict_info.index_file_size;

	sametypesequence=dict_info.sametypesequence;

	return true;
}

bool Dict::LookupWithRule(GPatternSpec *pspec, glong *aIndex, int iBuffLen)
{
	int iIndexCount = 0;

    for (guint32 i=0; i < narticles() && iIndexCount < (iBuffLen - 1); i++)
        if (g_pattern_match_string(pspec, get_key(i)))
			aIndex[iIndexCount++] = i;

    aIndex[iIndexCount] = -1; // -1 is the end.

  return iIndexCount > 0;
}

Libs::~Libs()
{
	for (Dict *p : oLib)
		delete p;
}

void Libs::load_dict(const std::string& url)
{
	Dict *lib=new Dict;
	if (lib->load(url, verbose_))
		oLib.push_back(lib);
	else
		delete lib;
}

void Libs::load(const std::list<std::string>& dicts_dirs,
		const std::list<std::string>& order_list,
		const std::list<std::string>& disable_list)
{
	for_each_file(dicts_dirs, ".ifo", order_list, disable_list,
                  [this](const std::string& url, bool disable) -> void {
                      if (!disable)
                          load_dict(url);
                  });
}

const gchar *Libs::poGetCurrentWord(glong * iCurrent)
{
  const gchar *poCurrentWord = nullptr;
  const gchar *word;
  for (std::vector<Dict *>::size_type iLib=0; iLib<oLib.size(); iLib++) {
    if (iCurrent[iLib]==INVALID_INDEX)
      continue;
    if ( iCurrent[iLib]>=narticles(iLib) || iCurrent[iLib]<0)
      continue;
    if ( poCurrentWord == nullptr ) {
      poCurrentWord = poGetWord(iCurrent[iLib],iLib);
    } else {
      word = poGetWord(iCurrent[iLib],iLib);

      if (stardict_strcmp(poCurrentWord, word) > 0 )
				poCurrentWord = word;
    }
  }
  return poCurrentWord;
}

const gchar *Libs::poGetNextWord(const gchar *sWord, glong *iCurrent)
{
	// the input can be:
	// (word,iCurrent),read word,write iNext to iCurrent,and return next word. used by TopWin::NextCallback();
	// (nullptr,iCurrent),read iCurrent,write iNext to iCurrent,and return next word. used by AppCore::ListWords();
	const gchar *poCurrentWord = nullptr;
    size_t iCurrentLib = 0;
	const gchar *word;

	for (size_t iLib = 0; iLib < oLib.size(); ++iLib) {
		if (sWord)
			oLib[iLib]->Lookup(sWord, iCurrent[iLib]);
		if (iCurrent[iLib]==INVALID_INDEX)
			continue;
		if (iCurrent[iLib]>=narticles(iLib) || iCurrent[iLib]<0)
			continue;
		if (poCurrentWord == nullptr ) {
			poCurrentWord = poGetWord(iCurrent[iLib],iLib);
			iCurrentLib = iLib;
		}	else {
			word = poGetWord(iCurrent[iLib],iLib);

			if (stardict_strcmp(poCurrentWord, word) > 0 ) {
				poCurrentWord = word;
				iCurrentLib = iLib;
			}
		}
	}
	if (poCurrentWord) {
		iCurrent[iCurrentLib]++;
		for (std::vector<Dict *>::size_type iLib=0;iLib<oLib.size();iLib++) {
			if (iLib == iCurrentLib)
				continue;
			if (iCurrent[iLib]==INVALID_INDEX)
				continue;
			if ( iCurrent[iLib]>=narticles(iLib) || iCurrent[iLib]<0)
				continue;
			if (strcmp(poCurrentWord, poGetWord(iCurrent[iLib],iLib)) == 0 )
				iCurrent[iLib]++;
		}
		poCurrentWord = poGetCurrentWord(iCurrent);
	}
	return poCurrentWord;
}


const gchar *
Libs::poGetPreWord(glong * iCurrent)
{
	// used by TopWin::PreviousCallback(); the iCurrent is cached by AppCore::TopWinWordChange();
	const gchar *poCurrentWord = nullptr;
	std::vector<Dict *>::size_type iCurrentLib=0;
	const gchar *word;

	for (std::vector<Dict *>::size_type iLib=0;iLib<oLib.size();iLib++) {
		if (iCurrent[iLib]==INVALID_INDEX)
			iCurrent[iLib]=narticles(iLib);
		else {
			if ( iCurrent[iLib]>narticles(iLib) || iCurrent[iLib]<=0)
				continue;
		}
		if ( poCurrentWord == nullptr ) {
			poCurrentWord = poGetWord(iCurrent[iLib]-1,iLib);
			iCurrentLib = iLib;
		} else {
			word = poGetWord(iCurrent[iLib]-1,iLib);
			if (stardict_strcmp(poCurrentWord, word) < 0 ) {
				poCurrentWord = word;
				iCurrentLib = iLib;
			}
		}
	}

	if (poCurrentWord) {
		iCurrent[iCurrentLib]--;
		for (std::vector<Dict *>::size_type iLib=0;iLib<oLib.size();iLib++) {
			if (iLib == iCurrentLib)
				continue;
			if (iCurrent[iLib]>narticles(iLib) || iCurrent[iLib]<=0)
				continue;
			if (strcmp(poCurrentWord, poGetWord(iCurrent[iLib]-1,iLib)) == 0) {
				iCurrent[iLib]--;
			} else {
				if (iCurrent[iLib]==narticles(iLib))
					iCurrent[iLib]=INVALID_INDEX;
			}
		}
	}
	return poCurrentWord;
}

bool Libs::LookupSimilarWord(const gchar* sWord, glong & iWordIndex, int iLib)
{
	glong iIndex;
    bool bFound = false;
	gchar *casestr;

	if (!bFound) {
		// to lower case.
		casestr = g_utf8_strdown(sWord, -1);
		if (strcmp(casestr, sWord)) {
			if(oLib[iLib]->Lookup(casestr, iIndex))
				bFound=true;
		}
		g_free(casestr);
		// to upper case.
		if (!bFound) {
			casestr = g_utf8_strup(sWord, -1);
			if (strcmp(casestr, sWord)) {
				if(oLib[iLib]->Lookup(casestr, iIndex))
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
				if(oLib[iLib]->Lookup(casestr, iIndex))
					bFound=true;
			}
			g_free(casestr);
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
				if (oLib[iLib]->Lookup(sNewWord, iIndex))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex))
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
					if( oLib[iLib]->Lookup(sNewWord, iIndex) )
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup(casestr, iIndex))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if (oLib[iLib]->Lookup(sNewWord, iIndex))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex))
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
					if (oLib[iLib]->Lookup(sNewWord, iIndex))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup(casestr, iIndex))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-4]=sNewWord[iWordLen-5];  //restore
					}
				}
				if( !bFound ) {
					if (oLib[iLib]->Lookup(sNewWord, iIndex))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex))
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
					if(oLib[iLib]->Lookup(sNewWord, iIndex))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex))
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
					if (oLib[iLib]->Lookup(sNewWord, iIndex))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup(casestr, iIndex))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if (oLib[iLib]->Lookup(sNewWord, iIndex))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex))
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
				if (oLib[iLib]->Lookup(sNewWord, iIndex))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex))
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
    bool bFound = oLib[iLib]->Lookup(sWord, iWordIndex);
	if (!bFound)
		bFound = LookupSimilarWord(sWord, iWordIndex, iLib);
	return bFound;
}

bool Libs::LookupWithFuzzy(const gchar *sWord, gchar *reslist[], gint reslist_size)
{
  if (sWord[0] == '\0')
      return false;

  Fuzzystruct oFuzzystruct[reslist_size];

  for (int i = 0; i < reslist_size; i++) {
    oFuzzystruct[i].pMatchWord = nullptr;
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

  for (size_t iLib = 0; iLib < oLib.size(); ++iLib) {
    if (progress_func)
        progress_func();

    //if (stardict_strcmp(sWord, poGetWord(0,iLib))>=0 && stardict_strcmp(sWord, poGetWord(narticles(iLib)-1,iLib))<=0) {
      //there are Chinese dicts and English dicts...

    const int iwords = narticles(iLib);
    for (int index=0; index<iwords; index++) {
        sCheck = poGetWord(index,iLib);
        // tolower and skip too long or too short words
        iCheckWordLen = g_utf8_strlen(sCheck, -1);
        if (iCheckWordLen-ucs4_str2_len>=iMaxDistance ||
            ucs4_str2_len-iCheckWordLen>=iMaxDistance)
            continue;
        ucs4_str1 = g_utf8_to_ucs4_fast(sCheck, -1, nullptr);
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

  }   // each lib
  g_free(ucs4_str2);

  if (Found)// sort with distance
      std::sort(oFuzzystruct, oFuzzystruct + reslist_size, [](const Fuzzystruct& lh, const Fuzzystruct& rh) -> bool {
              if (lh.iMatchWordDistance!=rh.iMatchWordDistance)
                  return lh.iMatchWordDistance<rh.iMatchWordDistance;

              if (lh.pMatchWord && rh.pMatchWord)
                  return stardict_strcmp(lh.pMatchWord, rh.pMatchWord)<0;

              return false;
          });

  for (gint i = 0; i < reslist_size; ++i)
      reslist[i] = oFuzzystruct[i].pMatchWord;

  return Found;
}

gint Libs::LookupWithRule(const gchar *word, gchar **ppMatchWord)
{
  glong aiIndex[MAX_MATCH_ITEM_PER_LIB+1];
  gint iMatchCount = 0;
  GPatternSpec *pspec = g_pattern_spec_new(word);

  for (std::vector<Dict *>::size_type iLib=0; iLib<oLib.size(); iLib++) {
    //if(oLibs.LookdupWordsWithRule(pspec,aiIndex,MAX_MATCH_ITEM_PER_LIB+1-iMatchCount,iLib))
    // -iMatchCount,so save time,but may got less result and the word may repeat.

    if (oLib[iLib]->LookupWithRule(pspec,aiIndex, MAX_MATCH_ITEM_PER_LIB+1)) {
      if (progress_func)
				progress_func();
      for (int i=0; aiIndex[i]!=-1; i++) {
				const gchar * sMatchWord = poGetWord(aiIndex[i],iLib);
				bool bAlreadyInList = false;
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
      std::sort(ppMatchWord, ppMatchWord+iMatchCount, [](const char *lh, const char *rh) -> bool {
              return stardict_strcmp(lh, rh)<0;
          });

  return iMatchCount;
}

bool Libs::LookupData(const gchar *sWord, std::vector<gchar *> *reslist)
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

	guint32 max_size =0;
	gchar *origin_data = nullptr;
	for (std::vector<Dict *>::size_type i=0; i<oLib.size(); ++i) {
		if (!oLib[i]->containSearchData())
			continue;
		if (progress_func)
			progress_func();
		const gulong iwords = narticles(i);
		const gchar *key;
		guint32 offset, size;
		for (gulong j=0; j<iwords; ++j) {
			oLib[i]->get_key_and_data(j, &key, &offset, &size);
			if (size>max_size) {
				origin_data = (gchar *)g_realloc(origin_data, size);
				max_size = size;
			}
			if (oLib[i]->SearchData(SearchWords, offset, size, origin_data))
				reslist[i].push_back(g_strdup(key));
		}
	}
	g_free(origin_data);

	std::vector<Dict *>::size_type i;
	for (i = 0; i < oLib.size(); ++i)
		if (!reslist[i].empty())
			break;

	return i != oLib.size();
}

/**************************************************/
query_t analyze_query(const char *s, std::string& res)
{
	if (!s || !*s) {
		res="";
		return qtSIMPLE;
	}
	if (*s=='/') {
		res=s+1;
		return qtFUZZY;
	}

	if (*s=='|') {
		res=s+1;
		return qtDATA;
	}

	bool regexp=false;
	const char *p=s;
	res="";
	for (; *p; res+=*p, ++p) {
		if (*p=='\\') {
			++p;
			if (!*p)
				break;
			continue;
		}
		if (*p=='*' || *p=='?')
			regexp=true;
	}
	if (regexp)
		return qtREGEXP;

	return qtSIMPLE;
}
