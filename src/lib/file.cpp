#include <algorithm>

#include <glib.h>

#include "file.hpp"

static void __for_each_file(const std::string& dirname, const std::string& suff,
                            const List& order_list, const List& disable_list, 
                            const std::function<void (const std::string&, bool)>& f)
{
	GDir *dir = g_dir_open(dirname.c_str(), 0, nullptr);	
    if (dir) {
		const gchar *filename;

        while ((filename = g_dir_read_name(dir))!=nullptr) {	
			const std::string fullfilename(dirname+G_DIR_SEPARATOR_S+filename);
			if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR))
				__for_each_file(fullfilename, suff, order_list, disable_list, f);
            else if (g_str_has_suffix(filename, suff.c_str()) &&
                     std::find(order_list.begin(), order_list.end(), 
                               fullfilename)==order_list.end()) { 
                const bool disable = std::find(disable_list.begin(), 
                                         disable_list.end(), 
                                         fullfilename)!=disable_list.end();
                f(fullfilename, disable);
			}
		}
		g_dir_close(dir);
	}
}


void for_each_file(const List& dirs_list, const std::string& suff,
                   const List& order_list, const List& disable_list, 
                   const std::function<void (const std::string&, bool)>& f)
{
	for (const std::string & item : order_list) {
		const bool disable = std::find(disable_list.begin(), disable_list.end(), item) != disable_list.end();
		f(item, disable);
	}
	for (const std::string& item : dirs_list)
		__for_each_file(item, suff, order_list, disable_list, f);			
}
