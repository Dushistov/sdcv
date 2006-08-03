/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
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

/* implementation of class to work with dictionary data */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "kmp.h"

#include "data.hpp"


DictBase::DictBase()
{
	dictfile = NULL;
	cache_cur =0;
}

DictBase::~DictBase()
{
	if (dictfile)
		fclose(dictfile);
}

gchar* DictBase::GetWordData(guint32 idxitem_offset, guint32 idxitem_size)
{
	for (int i=0; i<WORDDATA_CACHE_NUM; i++)
		if (cache[i].data && cache[i].offset == idxitem_offset)
			return cache[i].data;

	if (dictfile)
		fseek(dictfile, idxitem_offset, SEEK_SET);

	gchar *data;
	if (!sametypesequence.empty()) {
		gchar *origin_data = (gchar *)g_malloc(idxitem_size);

		if (dictfile)
			fread(origin_data, idxitem_size, 1, dictfile);
		else
			dictdzfile->read(origin_data, idxitem_offset, idxitem_size);

		guint32 data_size;
		gint sametypesequence_len = sametypesequence.length();
		//there have sametypesequence_len char being omitted.
		data_size = idxitem_size + sizeof(guint32) + sametypesequence_len;
		//if the last item's size is determined by the end up '\0',then +=sizeof(gchar);
		//if the last item's size is determined by the head guint32 type data,then +=sizeof(guint32);
		switch (sametypesequence[sametypesequence_len-1]) {
		case 'm':
		case 't':
		case 'y':
		case 'l':
		case 'g':
		case 'x':
		case 'k':
		case 'w':
			data_size += sizeof(gchar);
			break;
		case 'W':
		case 'P':
			data_size += sizeof(guint32);
			break;
		default:
			if (g_ascii_isupper(sametypesequence[sametypesequence_len-1]))
				data_size += sizeof(guint32);
			else
				data_size += sizeof(gchar);
			break;
		}
		data = (gchar *)g_malloc(data_size);
		gchar *p1,*p2;
		p1 = data + sizeof(guint32);
		p2 = origin_data;
		guint32 sec_size;
		//copy the head items.
		for (int i=0; i<sametypesequence_len-1; i++) {
			*p1=sametypesequence[i];
			p1+=sizeof(gchar);
			switch (sametypesequence[i]) {
			case 'm':
			case 't':
			case 'y':
			case 'l':
			case 'g':
			case 'x':
			case 'k':
			case 'w':
				sec_size = strlen(p2)+1;
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
				break;
			case 'W':
			case 'P':
				sec_size = *reinterpret_cast<guint32 *>(p2);
				sec_size += sizeof(guint32);
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
				break;
			default:
				if (g_ascii_isupper(sametypesequence[i])) {
					sec_size = *reinterpret_cast<guint32 *>(p2);
					sec_size += sizeof(guint32);
				} else {
					sec_size = strlen(p2)+1;
				}
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
				break;
			}
		}
		//calculate the last item 's size.
		sec_size = idxitem_size - (p2-origin_data);
		*p1=sametypesequence[sametypesequence_len-1];
		p1+=sizeof(gchar);
		switch (sametypesequence[sametypesequence_len-1]) {
		case 'm':
		case 't':
		case 'y':
		case 'l':
		case 'g':
		case 'x':
		case 'k':
		case 'w':
			memcpy(p1, p2, sec_size);
			p1 += sec_size;
			*p1='\0';//add the end up '\0';
			break;
		case 'W':
		case 'P':
			*reinterpret_cast<guint32 *>(p1)=sec_size;
			p1 += sizeof(guint32);
			memcpy(p1, p2, sec_size);
			break;
		default:
			if (g_ascii_isupper(sametypesequence[sametypesequence_len-1])) {
				*reinterpret_cast<guint32 *>(p1)=sec_size;
				p1 += sizeof(guint32);
				memcpy(p1, p2, sec_size);
			} else {
				memcpy(p1, p2, sec_size);
				p1 += sec_size;
				*p1='\0';
			}
			break;
		}
		g_free(origin_data);
		*reinterpret_cast<guint32 *>(data)=data_size;
	} else {
		data = (gchar *)g_malloc(idxitem_size + sizeof(guint32));
		if (dictfile)
			fread(data+sizeof(guint32), idxitem_size, 1, dictfile);
		else
			dictdzfile->read(data+sizeof(guint32), idxitem_offset, idxitem_size);
		*reinterpret_cast<guint32 *>(data)=idxitem_size+sizeof(guint32);
	}
	g_free(cache[cache_cur].data);

	cache[cache_cur].data = data;
	cache[cache_cur].offset = idxitem_offset;
	cache_cur++;
	if (cache_cur==WORDDATA_CACHE_NUM)
		cache_cur = 0;
	return data;
}

bool DictBase::SearchData(std::vector<std::string> &SearchWords, guint32 idxitem_offset, guint32 idxitem_size, gchar *origin_data)
{
	int nWord = SearchWords.size();
	std::vector<bool> WordFind(nWord, false);
	int nfound=0;

	if (dictfile)
		fseek(dictfile, idxitem_offset, SEEK_SET);
	if (dictfile)
		fread(origin_data, idxitem_size, 1, dictfile);
	else
		dictdzfile->read(origin_data, idxitem_offset, idxitem_size);
	gchar *p = origin_data;
	guint32 sec_size;
	int j;
	if (!sametypesequence.empty()) {
		gint sametypesequence_len = sametypesequence.length();
		for (int i=0; i<sametypesequence_len-1; i++) {
			switch (sametypesequence[i]) {
			case 'm':
			case 't':
			case 'y':
			case 'l':
			case 'g':
			case 'x':
			case 'k':
			case 'w':
				for (j=0; j<nWord; j++)
					// KMP() is faster than strstr() in theory. Really? Always be true?
					//if (!WordFind[j] && strstr(p, SearchWords[j].c_str())) {
					if (!WordFind[j] && KMP(p, strlen(p), SearchWords[j].c_str())!=-1) {
						WordFind[j] = true;
						++nfound;
					}

				if (nfound==nWord)
					return true;
				sec_size = strlen(p)+1;
				p+=sec_size;
				break;
			default:
				if (g_ascii_isupper(sametypesequence[i])) {
					sec_size = *reinterpret_cast<guint32 *>(p);
					sec_size += sizeof(guint32);
				} else {
					sec_size = strlen(p)+1;
				}
				p+=sec_size;
			}
		}
		switch (sametypesequence[sametypesequence_len-1]) {
		case 'm':
		case 't':
		case 'y':
		case 'l':
		case 'g':
		case 'x':
		case 'k':
		case 'w':
			sec_size = idxitem_size - (p-origin_data);
			for (j=0; j<nWord; j++)
				//if (!WordFind[j] && g_strstr_len(p, sec_size, SearchWords[j].c_str())) {
				if (!WordFind[j] && KMP(p, sec_size, SearchWords[j].c_str())!=-1) {
					WordFind[j] = true;
					++nfound;
				}

			if (nfound==nWord)
				return true;
			break;
		}
	} else {
		while (guint32(p - origin_data)<idxitem_size) {
			switch (*p) {
			case 'm':
			case 't':
			case 'y':
			case 'l':
			case 'g':
			case 'x':
			case 'k':
			case 'w':
				for (j=0; j<nWord; j++)
					if (!WordFind[j] && strstr(p, SearchWords[j].c_str())) {
						WordFind[j] = true;
						++nfound;
					}

				if (nfound==nWord)
					return true;
				sec_size = strlen(p)+1;
				p+=sec_size;
				break;
                        default:
                                if (g_ascii_isupper(*p)) {
                                        sec_size = *reinterpret_cast<guint32 *>(p);
					sec_size += sizeof(guint32);
                                } else {
                                        sec_size = strlen(p)+1;
                                }
                                p+=sec_size;
			}
		}
	}
	return false;
}

