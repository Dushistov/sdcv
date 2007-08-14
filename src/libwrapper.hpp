#ifndef _LIBWRAPPER_HPP_
#define _LIBWRAPPER_HPP_

#include <string>
#include <vector>

#include "file.hpp"
#include "lib.h"
#include "readline.hpp"

using std::string;
using std::vector;

//this structure is wrapper and it need for unification
//results of search whith return Dicts class
struct TSearchResult {
	string bookname;
	string def;
	string exp;

	TSearchResult(const string& bookname_, const string& def_, const string& exp_)
		: bookname(bookname_), def(def_), exp(exp_)
		{
		}
};

typedef vector<TSearchResult> TSearchResultList;
typedef TSearchResultList::iterator PSearchResult;

//this class is wrapper around Dicts class for easy use
//of it
class Library : public Libs {
public:
	Library(bool uinput, bool uoutput) : 
		utf8_input(uinput), utf8_output(uoutput) {}

	bool process_phrase(const char *loc_str, read_line &io, bool force=false);
private:
	bool utf8_input, utf8_output;

	void SimpleLookup(const string &str, TSearchResultList& res_list);
	void LookupWithFuzzy(const string &str, TSearchResultList& res_list);
	void LookupWithRule(const string &str, TSearchResultList& res_lsit);
	void LookupData(const string &str, TSearchResultList& res_list);
	void print_search_result(FILE *out, const TSearchResult & res);
};

#endif//!_LIBWRAPPER_HPP_
