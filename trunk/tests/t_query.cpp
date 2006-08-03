#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>
#include <vector>
#include <utility>
#include <iostream>

#include "lib.h"

static inline bool 
test_analyse_query(const char *s, const std::pair<query_t, std::string>& res)
{
	std::string resstr;
	query_t qt=analyse_query(s, resstr);
	if (qt!=res.first || resstr!=res.second) {
		std::cerr<<"query: "<<s<<std::endl
						 <<"expected: type="<<res.first<<", result="<<res.second<<std::endl
						 <<"get: type="<<qt<<", result="<<resstr<<std::endl;
		return false;
	}

	return true;
}

static inline void
add_analyse_query_test(std::vector<std::pair<std::string, std::pair<query_t, std::string> > >& v, const std::string& s, query_t r, const std::string& rs)
{
	v.push_back(std::pair<std::string, std::pair<query_t, std::string> >(s, std::pair<query_t, std::string>(r, rs)));
}

int main()
{
	std::vector<std::pair<std::string, std::pair<query_t, std::string> > > aq_tests;

	add_analyse_query_test(aq_tests, "/fuzzy", qtFUZZY, "fuzzy");
	add_analyse_query_test(aq_tests, "|data", qtDATA, "data");
	add_analyse_query_test(aq_tests, "\\|data", qtSIMPLE, "|data");
	add_analyse_query_test(aq_tests, "\\/fuzzy", qtSIMPLE, "/fuzzy");
	add_analyse_query_test(aq_tests, "re?ex*", qtREGEXP, "re?ex*");
	add_analyse_query_test(aq_tests, "re\\?ex\\*", qtSIMPLE, "re?ex*");
	add_analyse_query_test(aq_tests, "re?ex\\*", qtREGEXP, "re?ex*");
	add_analyse_query_test(aq_tests, "re\\?ex*", qtREGEXP, "re?ex*");
	add_analyse_query_test(aq_tests, "\\|data\\?", qtSIMPLE, "|data?");
	add_analyse_query_test(aq_tests, "", qtSIMPLE, "");
	add_analyse_query_test(aq_tests, "\\/", qtSIMPLE, "/");
	add_analyse_query_test(aq_tests, "/", qtFUZZY, "");


	for (std::vector<std::pair<std::string, std::pair<query_t, std::string> > >::const_iterator p=aq_tests.begin();
			 p!=aq_tests.end(); ++p)
		if (!test_analyse_query(p->first.c_str(), p->second))		
			return EXIT_FAILURE;
		
	return EXIT_SUCCESS;
}
