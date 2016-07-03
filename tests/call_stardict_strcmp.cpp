//g++ `pkg-config --cflags glib-2.0` call_stardict_strcmp.cpp `pkg-config --libs glib-2.0`
#include <glib.h>
#include <locale.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

static inline gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
        const gint a = g_ascii_strcasecmp(s1, s2);
        if (a == 0)
		return strcmp(s1, s2);
        else
		return a;
}

int main()
{
	setlocale(LC_ALL, "");
	std::cin.sync_with_stdio(false);
	std::string line1, line2;
	while (std::getline(std::cin, line1) &&
	       std::getline(std::cin, line2)) {
		std::cout << stardict_strcmp(line1.c_str(), line2.c_str()) << "\n";
	}
	return EXIT_SUCCESS;
}
