#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "lib.h"

query_t analyse_query(const char *s, std::string& res)
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
