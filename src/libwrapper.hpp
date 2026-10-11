#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "readline.hpp"
#include "stardict_lib.hpp"

//this structure is wrapper and it need for unification
//results of search whith return Dicts class
struct TSearchResult {
    std::string bookname;
    std::string def;
    std::string exp;

    TSearchResult(const std::string &bookname_, const std::string &def_, const std::string &exp_)
        : bookname(bookname_)
        , def(def_)
        , exp(exp_)
    {
    }
};

typedef std::vector<TSearchResult> TSearchResultList;

// Sink for results as they are found, without accumulating them all.
using TSearchResultSink = std::function<void(TSearchResult &&)>;

//possible return values for Library.process_phase()
enum search_result {
    SEARCH_SUCCESS = 0,
    SEARCH_FAILURE,
    SEARCH_NO_RESULT
};

//this class is wrapper around Dicts class for easy use
//of it
class Library : public Libs
{
public:
    Library(bool uinput, bool uoutput, bool colorize_output, bool use_json, bool no_fuzzy)
        : utf8_input_(uinput)
        , utf8_output_(uoutput)
        , colorize_output_(colorize_output)
        , json_(use_json)
    {
        setVerbose(!use_json);
        setFuzzy(!no_fuzzy);
    }

    search_result process_phrase(const char *loc_str, IReadLine &io, bool force = false);

private:
    bool utf8_input_;
    bool utf8_output_;
    bool colorize_output_;
    bool json_;

    // All lookups hand entries to the sink as found; the sink decides
    // whether to print them right away or collect them into a list.
    void SimpleLookup(const std::string &str, const TSearchResultSink &sink);
    void LookupWithFuzzy(const std::string &str, const TSearchResultSink &sink);
    void LookupWithRule(const std::string &str, const TSearchResultSink &sink);
    void LookupData(const std::string &str, const TSearchResultSink &sink);
    void print_search_result(FILE *out, const TSearchResult &res, bool &first_result);
};
