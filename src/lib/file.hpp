#ifndef _FILE_HPP_
#define _FILE_HPP_

#include <algorithm>
#include <glib.h>
#include <list>
#include <string>


typedef std::list<std::string> List;

template<typename Function>
void __for_each_file(const std::string& dirname, const std::string& suff,
										 const List& order_list, const List& disable_list, 
										 Function f)
{
	GDir *dir = g_dir_open(dirname.c_str(), 0, NULL);	
  if (dir) {
		const gchar *filename;		

    while ((filename = g_dir_read_name(dir))!=NULL) {	
			std::string fullfilename(dirname+G_DIR_SEPARATOR_S+filename);
			if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR))
				__for_each_file(fullfilename, suff, order_list, disable_list, f);
      else if (g_str_has_suffix(filename, suff.c_str()) &&
							 std::find(order_list.begin(), order_list.end(), 
												 fullfilename)==order_list.end()) { 
							 bool disable=std::find(disable_list.begin(), 
																			disable_list.end(), 
																			fullfilename)!=disable_list.end();
							 f(fullfilename, disable);
			}
		}
		g_dir_close(dir);
	}
}

template<typename Function>
void for_each_file(const List& dirs_list, const std::string& suff,
									 const List& order_list, const List& disable_list, 
									 Function f)
{
	List::const_iterator it;
	for (it=order_list.begin(); it!=order_list.end(); ++it) {
		bool disable=std::find(disable_list.begin(), disable_list.end(),
				       *it)!=disable_list.end();
		f(*it, disable);
	}
	for (it=dirs_list.begin(); it!=dirs_list.end(); ++it)
		__for_each_file(*it, suff, order_list, disable_list, f);			
}

#endif//!_FILE_HPP_
