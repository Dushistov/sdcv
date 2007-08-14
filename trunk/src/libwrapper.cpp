/* 
 * This file part of sdcv - console version of Stardict program
 * http://sdcv.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <map>

#include "utils.hpp"

#include "libwrapper.hpp"


static std::string xdxf2text(const char *p)
{
	std::string res;
	for (; *p; ++p) {
		if (*p!='<') {
			if (g_str_has_prefix(p, "&gt;")) {
				res+=">";
				p+=3;
			} else if (g_str_has_prefix(p, "&lt;")) {
				res+="<";
				p+=3;
			} else if (g_str_has_prefix(p, "&amp;")) {
				res+="&";
				p+=4;
			} else if (g_str_has_prefix(p, "&quot;")) {
				res+="\"";
				p+=5;
			} else
				res+=*p;
			continue;
		}

		const char *next=strchr(p, '>');
		if (!next)
			continue;

		std::string name(p+1, next-p-1);

		if (name=="abr")
			res+="";
		else if (name=="/abr")
			res+="";
		else if (name=="k") {
			const char *begin=next;
			if ((next=strstr(begin, "</k>"))!=NULL)
				next+=sizeof("</k>")-1-1;
			else
				next=begin;
		} else if (name=="b")
			res+="";
		else if (name=="/b")
			res+="";
		else if (name=="i")
			res+="";
		else if (name=="/i")
			res+="";
		else if (name=="tr")
			res+="[";
		else if (name=="/tr")
			res+="]";
		else if (name=="ex")
			res+="";
		else if (name=="/ex")
			res+="";
		else if (!name.empty() && name[0]=='c' && name!="co") {
			std::string::size_type pos=name.find("code");
			if (pos!=std::string::size_type(-1)) {
				pos+=sizeof("code=\"")-1;
				std::string::size_type end_pos=name.find("\"");
				std::string color(name, pos, end_pos-pos);
				res+="";
			} else {
				res+="";
			}
		} else if (name=="/c")
			res+="";

		p=next;
	}
	return res;
}

static string parse_data(const gchar *data)
{
	if (!data)
		return "";

	string res;
	guint32 data_size, sec_size=0;
	gchar *m_str;
	const gchar *p=data;
	data_size=*((guint32 *)p);
	p+=sizeof(guint32);
	while (guint32(p - data)<data_size) {
		switch (*p++) {
		case 'm':
		case 'l': //need more work...
			sec_size = strlen(p);
			if (sec_size) {
				res+="\n";
				m_str = g_strndup(p, sec_size);
				res += m_str;
				g_free(m_str);
			}
			sec_size++;
			break;
		case 'g':
		case 'x':
			sec_size = strlen(p);
			if (sec_size) {
				res+="\n";
				m_str = g_strndup(p, sec_size);
				res += xdxf2text(m_str);
				g_free(m_str);
			}
			sec_size++;
			break;
		case 't':
			sec_size = strlen(p);
			if(sec_size){
				res+="\n";
				m_str = g_strndup(p, sec_size);
				res += "["+string(m_str)+"]";
				g_free(m_str);
			}
			sec_size++;
			break;
		case 'y':
			sec_size = strlen(p);
			sec_size++;				
			break;
		case 'W':
		case 'P':
			sec_size=*((guint32 *)p);
			sec_size+=sizeof(guint32);
			break;
		}
		p += sec_size;
	}

  
	return res;
}

void Library::SimpleLookup(const string &str, TSearchResultList& res_list)
{	
	glong ind;
	res_list.reserve(ndicts());
	for (gint idict=0; idict<ndicts(); ++idict)
		if (SimpleLookupWord(str.c_str(), ind, idict))
			res_list.push_back(
				TSearchResult(dict_name(idict), 
					      poGetWord(ind, idict),
					      parse_data(poGetWordData(ind, idict))));
	
}

void Library::LookupWithFuzzy(const string &str, TSearchResultList& res_list)
{
	static const int MAXFUZZY=10;

	gchar *fuzzy_res[MAXFUZZY];
	if (!Libs::LookupWithFuzzy(str.c_str(), fuzzy_res, MAXFUZZY))
		return;
	
	for (gchar **p=fuzzy_res, **end=fuzzy_res+MAXFUZZY; 
	     p!=end && *p; ++p) {
		SimpleLookup(*p, res_list);
		g_free(*p);
	}
}

void Library::LookupWithRule(const string &str, TSearchResultList& res_list)
{
	std::vector<gchar *> match_res((MAX_MATCH_ITEM_PER_LIB) * ndicts());

	gint nfound=Libs::LookupWithRule(str.c_str(), &match_res[0]);
	if (!nfound)
		return;

	for (gint i=0; i<nfound; ++i) {
		SimpleLookup(match_res[i], res_list);
		g_free(match_res[i]);
	}
}

void Library::LookupData(const string &str, TSearchResultList& res_list)
{
	std::vector<gchar *> drl[ndicts()];
	if (!Libs::LookupData(str.c_str(), drl))
		return;
	for (int idict=0; idict<ndicts(); ++idict)
		for (std::vector<gchar *>::size_type j=0; j<drl[idict].size(); ++j) {
			SimpleLookup(drl[idict][j], res_list);
			g_free(drl[idict][j]);
		}
}

void Library::print_search_result(FILE *out, const TSearchResult & res)
{
	string loc_bookname, loc_def, loc_exp;
	if(!utf8_output){
		loc_bookname=utf8_to_locale_ign_err(res.bookname);
		loc_def=utf8_to_locale_ign_err(res.def);
		loc_exp=utf8_to_locale_ign_err(res.exp);
	}

			
	fprintf(out, "-->%s\n-->%s\n%s\n\n",
		utf8_output ? res.bookname.c_str() : loc_bookname.c_str(), 
		utf8_output ? res.def.c_str() : loc_def.c_str(), 
		utf8_output ? res.exp.c_str() : loc_exp.c_str()); 
}

class sdcv_pager {
public:
	sdcv_pager(bool ignore_env=false) {
		output=stdout;
		if (ignore_env)
			return;
		const gchar *pager=g_getenv("SDCV_PAGER");
		if (pager && (output=popen(pager, "w"))==NULL) {
			perror(_("popen failed"));
			output=stdout;
		}
	}
	~sdcv_pager() {
		if (output!=stdout)
			fclose(output);
	}
	FILE *get_stream() { return output; }
private:
	FILE *output;
};

bool Library::process_phrase(const char *loc_str, read_line &io, bool force)
{
	if (NULL==loc_str)
		return true;

	std::string query;

	
	analyze_query(loc_str, query);
	if (!query.empty())
		io.add_to_history(query.c_str());
	


	gsize bytes_read;
	gsize bytes_written;
	GError *err=NULL;
	char *str=NULL;
	if (!utf8_input)
		str=g_locale_to_utf8(loc_str, -1, &bytes_read, &bytes_written, &err);
	else
		str=g_strdup(loc_str);

	if (NULL==str) {
		fprintf(stderr, _("Can not convert %s to utf8.\n"), loc_str);
		fprintf(stderr, "%s\n", err->message);
		g_error_free(err);
		return false;
	}

	if (str[0]=='\0')
		return true;

  
	TSearchResultList res_list;


	switch (analyze_query(str, query)) {
	case qtFUZZY:
		LookupWithFuzzy(query, res_list);
		break;
	case qtREGEXP:
		LookupWithRule(query, res_list);
		break;
	case qtSIMPLE:
		SimpleLookup(str, res_list);
		if (res_list.empty())
			LookupWithFuzzy(str, res_list);
		break;
	case qtDATA:
		LookupData(query, res_list);
		break;
	default:
		/*nothing*/;
	}

	if (!res_list.empty()) {    
		/* try to be more clever, if there are
		   one or zero results per dictionary show all
		*/
		bool show_all_results=true;
		typedef std::map< string, int, std::less<string> > DictResMap;
		if (!force) {
			DictResMap res_per_dict;
			for(TSearchResultList::iterator ptr=res_list.begin(); ptr!=res_list.end(); ++ptr){
				std::pair<DictResMap::iterator, DictResMap::iterator> r = 
					res_per_dict.equal_range(ptr->bookname);
				DictResMap tmp(r.first, r.second);
				if (tmp.empty()) //there are no yet such bookname in map
					res_per_dict.insert(DictResMap::value_type(ptr->bookname, 1));
				else {
					++((tmp.begin())->second);
					if (tmp.begin()->second>1) {
						show_all_results=false;
						break;
					}
				}
			}
		}//if (!force)

		if (!show_all_results && !force) {
			printf(_("Found %d items, similar to %s.\n"), res_list.size(), 
			       utf8_output ? str : utf8_to_locale_ign_err(str).c_str());
			for (size_t i=0; i<res_list.size(); ++i) {
				string loc_bookname, loc_def;
				loc_bookname=utf8_to_locale_ign_err(res_list[i].bookname);
				loc_def=utf8_to_locale_ign_err(res_list[i].def);
				printf("%d)%s-->%s\n", i,
				       utf8_output ?  res_list[i].bookname.c_str() : loc_bookname.c_str(),
				       utf8_output ? res_list[i].def.c_str() : loc_def.c_str());
			}
			int choise;
			for (;;) {
				string str_choise;
				printf(_("Your choice[-1 to abort]: "));
				
				if(!stdio_getline(stdin, str_choise)){
					putchar('\n');
					exit(EXIT_SUCCESS);
				}
				sscanf(str_choise.c_str(), "%d", &choise);
				if (choise>=0 && choise<int(res_list.size())) { 
					sdcv_pager pager;
					print_search_result(pager.get_stream(), res_list[choise]);
					break;
				} else if (choise==-1)
					break;
				else
					printf(_("Invalid choise.\nIt must be from 0 to %d or -1.\n"), 
					       res_list.size()-1);	  
			}		
		} else {
			sdcv_pager pager(force);
			fprintf(pager.get_stream(), _("Found %d items, similar to %s.\n"), 
				res_list.size(), utf8_output ? str : utf8_to_locale_ign_err(str).c_str());
			for (PSearchResult ptr=res_list.begin(); ptr!=res_list.end(); ++ptr)
				print_search_result(pager.get_stream(), *ptr);
		}
    
	} else {
		string loc_str;
		if (!utf8_output)
			loc_str=utf8_to_locale_ign_err(str);
    
		printf(_("Nothing similar to %s, sorry :(\n"), utf8_output ? str : loc_str.c_str());
	}
	g_free(str);

	return true;
}
