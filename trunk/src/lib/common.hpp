#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <glib.h>
#include <list>
#include <string>

//this structure contain all information about dictionary
struct DictInfo {
  std::string ifo_file_name;
  guint32 wordcount;
  guint32 synwordcount;
  std::string bookname;
  std::string author;
  std::string email;
  std::string website;
  std::string date;
  std::string description;
  guint32 index_file_size;
  std::string sametypesequence;
  bool load_from_ifo_file(const std::string& ifofilename, bool istreedict);
};

typedef std::list<std::string> strlist_t;

#endif//!_COMMON_HPP_
