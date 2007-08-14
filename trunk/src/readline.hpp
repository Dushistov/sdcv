#ifndef _READLINE_HPP_
#define _READLINE_HPP_

#include <string>
using std::string;

class read_line {
public:
	virtual ~read_line() {}
	virtual bool read(const string &banner, string& line)=0;
	virtual void add_to_history(const std::string& phrase) {}
};

extern read_line *create_readline_object();

#endif//!_READLINE_HPP_
