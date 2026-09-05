#pragma once

#include <cstring>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "dictziplib.hpp"
#include "mapfile.hpp"

const int MAX_MATCH_ITEM_PER_LIB = 100;
const int MAX_FUZZY_DISTANCE = 3; // at most MAX_FUZZY_DISTANCE-1 differences allowed when find similar words

inline guint32 get_uint32(const gchar *addr)
{
    guint32 result;
    memcpy(&result, addr, sizeof(guint32));
    return result;
}

inline void set_uint32(gchar *addr, guint32 val)
{
    memcpy(addr, &val, sizeof(guint32));
}

struct cacheItem {
    guint32 offset;
    gchar *data;
    // write code here to make it inline
    cacheItem() { data = nullptr; }
    ~cacheItem() { g_free(data); }
};

const int WORDDATA_CACHE_NUM = 10;
const int INVALID_INDEX = -100;

class DictBase
{
public:
    DictBase() {}
    ~DictBase()
    {
        if (dictfile)
            fclose(dictfile);
    }
    DictBase(const DictBase &) = delete;
    DictBase &operator=(const DictBase &) = delete;
    gchar *GetWordData(guint32 idxitem_offset, guint32 idxitem_size);
    bool containSearchData() const
    {
        if (sametypesequence.empty())
            return true;
        return sametypesequence.find_first_of("mlgxty") != std::string::npos;
    }
    bool SearchData(std::vector<std::string> &SearchWords, guint32 idxitem_offset, guint32 idxitem_size, gchar *origin_data);

protected:
    std::string sametypesequence;
    FILE *dictfile = nullptr;
    std::unique_ptr<DictData> dictdzfile;

private:
    cacheItem cache[WORDDATA_CACHE_NUM];
    gint cache_cur = 0;
};

// this structure contain all information about dictionary
struct DictInfo {
    std::string ifo_file_name;
    guint32 wordcount;
    guint32 syn_wordcount;
    std::string bookname;
    std::string author;
    std::string email;
    std::string website;
    std::string date;
    std::string description;
    off_t index_file_size;
    off_t syn_file_size;
    std::string sametypesequence;

    bool load_from_ifo_file(const std::string &ifofilename, bool istreedict);
};

class IIndexFile
{
public:
    guint32 wordentry_offset;
    guint32 wordentry_size;

    virtual ~IIndexFile() {}
    virtual bool load(const std::string &url, gulong wc, off_t fsize, bool verbose) = 0;
    virtual const gchar *get_key(glong idx) = 0;
    virtual void get_data(glong idx) = 0;
    virtual const gchar *get_key_and_data(glong idx) = 0;
    virtual bool lookup(const char *str, std::set<glong> &idxs, glong &next_idx) = 0;
    virtual bool lookup(const char *str, std::set<glong> &idxs)
    {
        glong unused_next_idx;
        return lookup(str, idxs, unused_next_idx);
    };
};

// The table of file offsets that lets a lookup binary-search an .idx or .syn
// without scanning it. Building the table means walking every entry of the
// index, so it is cached in a sibling .oft file.
//
// It is read straight out of that cache's mapping. A large inflection index
// has millions of entries, and copying the table into a vector on every sdcv
// launch - sdcv runs once per lookup - costs that many megabytes of anonymous
// memory for the twenty-odd offsets a binary search actually touches. Mapped,
// the same search faults in a handful of pages, they are shared between
// processes, and the kernel can drop them under pressure.
class OffsetTable
{
public:
    // Map <url>.oft if it is fresh and holds exactly nelem offsets.
    bool load(const std::string &url, size_t nelem);
    // Take a table that had to be rebuilt by scanning the index.
    void adopt(std::vector<guint32> &&built);
    bool save(const std::string &url, bool verbose) const;
    void clear();

    guint32 operator[](size_t i) const { return ptr[i]; }
    size_t size() const { return nelem_; }
    bool empty() const { return nelem_ == 0; }

private:
    MapFile cache;             // holds the mapping ptr points into, when cached
    std::vector<guint32> owned; // holds the table instead, when freshly built
    const guint32 *ptr = nullptr;
    size_t nelem_ = 0;
};

class SynFile
{
public:
    SynFile() {}
    ~SynFile() {}
    bool load(const std::string &url, gulong wc, bool verbose);
    bool lookup(const char *str, std::set<glong> &idxs, glong &next_idx);
    bool lookup(const char *str, std::set<glong> &idxs);
    const gchar *get_key(glong idx) { return synfile.begin() + wordoffset[idx]; }

private:
    MapFile synfile;
    OffsetTable wordoffset;
};

class Dict : public DictBase
{
public:
    Dict() {}
    Dict(const Dict &) = delete;
    Dict &operator=(const Dict &) = delete;
    bool load(const std::string &ifofilename, bool verbose);

    gulong narticles() const { return wordcount; }
    const std::string &dict_name() const { return bookname; }
    const std::string &ifofilename() const { return ifo_file_name; }

    const gchar *get_key(glong index) { return idx_file->get_key(index); }
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
    bool Lookup(const char *str, std::set<glong> &idxs, glong &next_idx);
    bool Lookup(const char *str, std::set<glong> &idxs)
    {
        glong unused_next_idx;
        return Lookup(str, idxs, unused_next_idx);
    }

    bool LookupWithRule(GPatternSpec *pspec, glong *aIndex, int iBuffLen);

private:
    std::string ifo_file_name;
    gulong wordcount;
    gulong syn_wordcount;
    std::string bookname;

    std::unique_ptr<IIndexFile> idx_file;
    std::unique_ptr<SynFile> syn_file;

    bool load_ifofile(const std::string &ifofilename, off_t &idxfilesize);
};

class Libs
{
public:
    Libs(std::function<void(void)> f = std::function<void(void)>())
    {
        progress_func = f;
        iMaxFuzzyDistance = MAX_FUZZY_DISTANCE; // need to read from cfg.
    }
    void setVerbose(bool verbose) { verbose_ = verbose; }
    void setFuzzy(bool fuzzy) { fuzzy_ = fuzzy; }
    ~Libs();
    Libs(const Libs &) = delete;
    Libs &operator=(const Libs &) = delete;

    void load_dict(const std::string &url);
    void load(const std::list<std::string> &dicts_dirs,
              const std::list<std::string> &order_list,
              const std::list<std::string> &disable_list);
    glong narticles(int idict) const { return oLib[idict]->narticles(); }
    const std::string &dict_name(int idict) const { return oLib[idict]->dict_name(); }
    gint ndicts() const { return oLib.size(); }

    const gchar *poGetWord(glong iIndex, int iLib)
    {
        return oLib[iLib]->get_key(iIndex);
    }
    gchar *poGetWordData(glong iIndex, int iLib)
    {
        if (iIndex == INVALID_INDEX)
            return nullptr;
        return oLib[iLib]->get_data(iIndex);
    }
    bool LookupWord(const gchar *sWord, std::set<glong> &iWordIndices, int iLib)
    {
        return oLib[iLib]->Lookup(sWord, iWordIndices);
    }
    bool LookupSimilarWord(const gchar *sWord, std::set<glong> &iWordIndices, int iLib);
    bool SimpleLookupWord(const gchar *sWord, std::set<glong> &iWordIndices, int iLib);

    bool LookupWithFuzzy(const gchar *sWord, gchar *reslist[], gint reslist_size);
    gint LookupWithRule(const gchar *sWord, gchar *reslist[]);
    bool LookupData(const gchar *sWord, std::vector<gchar *> *reslist);

protected:
    bool fuzzy_;

private:
    std::vector<Dict *> oLib; // word Libs.
    int iMaxFuzzyDistance;
    std::function<void(void)> progress_func;
    bool verbose_;
};

enum query_t {
    qtSIMPLE,
    qtREGEXP,
    qtFUZZY,
    qtDATA
};

extern query_t analyze_query(const char *s, std::string &res);
