#ifndef __SD_READWORD_H__
#define __SD_READWORD_H__

#include <glib.h>
#include <list>
#include <string>

class ReadWord {
public:
	ReadWord();
	void loadpath(const gchar *path);
	bool canRead(const gchar *word);
	void read(const gchar *word);
private:
	std::list<std::string> ttspath;
};

#endif
