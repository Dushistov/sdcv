#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <string>

#include "conf.h"
#include "utils.h"

#include "readword.h"

ReadWord::ReadWord()
{
	const std::string &path = conf->get_string_at("dictionary/tts_path");
	loadpath(path.c_str());
}

void ReadWord::loadpath(const gchar *path)
{
	std::list<std::string> paths;
	std::string str;
	const gchar *p, *p1;
	p = path;
	do {
		p1 = strchr(p, '\n');
		if (p1) {
			str.assign(p, p1-p);
			if (!str.empty())
				paths.push_back(str);
			p = p1+1;
		}
	} while (p1);
	str = p;
	if (!str.empty())
		paths.push_back(str);

	ttspath.clear();
	std::list<std::string>::const_iterator it;
	for (it=paths.begin(); it!=paths.end(); ++it) {
#ifdef _WIN32
		if (it->length()>1 && (*it)[1]==':') {
			if (g_file_test(it->c_str(), G_FILE_TEST_EXISTS))
				ttspath.push_back(*it);
		} else {
			str = gStarDictDataDir + G_DIR_SEPARATOR_S + *it;
			if (g_file_test(str.c_str(), G_FILE_TEST_EXISTS))
				ttspath.push_back(str);
		}
#else
		if (g_file_test(it->c_str(), G_FILE_TEST_EXISTS))
			ttspath.push_back(*it);
#endif
	}
}

bool ReadWord::canRead(const gchar *word)
{	
	bool return_val = false;
	if (!ttspath.empty() && word && g_ascii_isalpha(word[0])) {
		std::string lowerword;
		const gchar *p = word;
		while (*p) {
			if (*p!=' ')
				lowerword+=g_ascii_tolower(*p);
			p++;
		}
		std::string filename;
		std::list<std::string>::const_iterator it;
		for (it=ttspath.begin(); it!=ttspath.end(); ++it) {
			filename = *it + G_DIR_SEPARATOR_S + lowerword[0] + G_DIR_SEPARATOR_S + lowerword + ".wav";
			return_val = g_file_test(filename.c_str(), G_FILE_TEST_EXISTS);
			if (return_val)
				break;
		}
	}
	return return_val;
}

void ReadWord::read(const gchar *word)
{
	if (!ttspath.empty() && word && g_ascii_isalpha(word[0])) {
		std::string lowerword;
		const gchar *p = word;
		while (*p) {
			if (*p!=' ')
				lowerword+=g_ascii_tolower(*p);
			p++;
		}
		std::string filename;
		std::list<std::string>::const_iterator it;
		for (it=ttspath.begin(); it!=ttspath.end(); ++it) {
			filename = *it + G_DIR_SEPARATOR_S + lowerword[0] + G_DIR_SEPARATOR_S + lowerword + ".wav";
			if (g_file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
				play_wav_file(filename);
				break;
			}
		}

	}
}
