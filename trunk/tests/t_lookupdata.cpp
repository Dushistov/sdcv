#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>
#include <vector>
#include <iostream>

#include "file.hpp"
#include "lib.h"

int main(int argc, char *argv[])
{
	List dirs;
	gtk_init(&argc, &argv);
	
	dirs.push_back("./");
	
	Libs libs(NULL, false, false, 0);
	libs.load(dirs, List(), List());
	std::vector<gchar *> reslist[libs.ndicts()];
	if (libs.LookupData("letter", reslist, NULL, NULL, NULL)) 
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}
