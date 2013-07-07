#pragma once

#include <string>

class IReadLine {
public:
	virtual ~IReadLine() {}
	virtual bool read(const std::string &banner, std::string& line) = 0;
	virtual void add_to_history(const std::string&) {}
};

extern std::string sdcv_readline;
extern IReadLine *create_readline_object();

