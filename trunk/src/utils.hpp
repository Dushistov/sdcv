#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <string>
using std::string;

extern bool stdio_getline(FILE *in, string &str);
extern char *locale_to_utf8(const char *locstr);
extern string utf8_to_locale_ign_err(const string& utf8_str);

#endif//!_UTILS_HPP_
