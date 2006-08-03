/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2005 Evgeniy <dushistov@mail.ru>
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

#include <cstring>
#include <gtk/gtk.h>

#include "articleview.h"


static gchar* toUtfPhonetic(const gchar *text, gsize len)
{
	std::string p;
	gsize i;
	for (i=0;i<len;i++) {
		switch (text[i]) {
			case 'A':
				p+="æ"; break;
			case 'B':
				p+="ɑ"; break;
			case 'C':
				p+="ɔ"; break;
			case 'Q':
				p+="ʌ"; break;
			case 'E':
				p+="ә"; break;
			case 'Z':
				p+="є"; break;
			case 'N':
				p+="ŋ"; break;
			case 'W':
				p+="θ"; break;
			case 'T':
				p+="ð"; break;
			case 'F':
				p+="ʃ"; break;
			case 'V':
				p+="ʒ"; break;
			case 'L':
				p+="ɚ"; break;
			case 'I':
				p+="i"; break;
			case '^':
				p+="ɡ"; break;
			case '9':
				p+="ˏ"; break;
			case '5':
				p+="'"; break;
			default:
				p+=text[i];
				break;
		}
	}
	return g_markup_escape_text(p.c_str(), -1);
}

static gchar *powerword_markup_escape_text(const gchar *text, gssize length)
{
	const gchar *p;
	const gchar *end;
	p = text;
	end = text + length;

	GString *str;
	str = g_string_sized_new (length);

	const gchar *n;
	bool find;
	bool previous_islink = false;
	std::string marktags;
	guint currentmarktag = 0;
	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);
		switch (*p) {
			case '}':
				if (currentmarktag==0) {
					g_string_append (str, "}");
					previous_islink = false;
				}
				else {
					currentmarktag--;
					switch (marktags[currentmarktag]) {
						case 'b':
						case 'B':
							g_string_append (str, "</b>");
							previous_islink = false;
							break;
						case 'I':
							g_string_append (str, "</i>");
							previous_islink = false;
							break;
						case '+':
							g_string_append (str, "</sup>");
							previous_islink = false;
							break;
						case '-':
							g_string_append (str, "</sub>");
							previous_islink = false;
							break;
						case 'x':
							g_string_append (str, "</span>");
							previous_islink = false;
							break;
						case 'l':
						case 'D':
						case 'L':
						case 'U':
							g_string_append (str, "</span>");
							previous_islink = true;
							break;
						default:
							previous_islink = false;
							break;
					}
				}
				break;
			case '&':
				find = false;
				if (next!=end) {
					n = g_utf8_next_char(next);
					if (n!=end && *n == '{') {
						find=true;
						currentmarktag++;
						if (marktags.length()<currentmarktag)
							marktags+=*next;
						else
							marktags[currentmarktag-1]=*next;
						switch (*next) {
							case 'b':
							case 'B':
								g_string_append (str, "<b>");
								next = n+1;
								break;
							case 'I':
								g_string_append (str, "<i>");
								next = n+1;
								break;
							case '+':
								g_string_append (str, "<sup>");
								next = n+1;
								break;
							case '-':
								g_string_append (str, "<sub>");
								next = n+1;
								break;
							case 'x':
								g_string_append (str, "<span foreground=\"blue\" underline=\"single\">");
								next = n+1;
								break;
							case 'X':
								{
								const gchar *tag_end = n+1;
								while (tag_end!=end) {
									if (*tag_end=='}')
										break;
									else
										tag_end++;
								}
								g_string_append (str, "<span foreground=\"blue\">");
								gchar *tag_str = toUtfPhonetic(n+1, tag_end - (n+1));
								g_string_append (str, tag_str);
								g_free(tag_str);
								g_string_append (str, "</span>");
								currentmarktag--;
								if (tag_end!=end)
									next = tag_end+1;
								else
									next = end;
								previous_islink = false;
								break;
								}
							case 'l':
							case 'D':
								if (previous_islink)
									g_string_append (str, "\t");
								g_string_append (str, "<span foreground=\"blue\" underline=\"single\">");
								next = n+1;
								break;
							case 'L':
							case 'U':
								if (previous_islink)
									g_string_append (str, "\t");
								g_string_append (str, "<span foreground=\"#008080\" underline=\"single\">");
								next = n+1;
								break;
							case '2':
								// Phonetic. Need more work...
								next = n+1;
								break;
							/*case ' ':
							case '9':
							case 'S':*/
							default:
								next = n+1;
								break;
						}
					}
				}
				if (!find) {
					previous_islink = false;
					g_string_append (str, "&amp;");
				}
				break;
			case '<':
				previous_islink = false;
				g_string_append (str, "&lt;");
				break;
			case '>':
				previous_islink = false;
				g_string_append (str, "&gt;");
				break;
			case '\'':
				previous_islink = false;
				g_string_append (str, "&apos;");
				break;
			case '"':
				previous_islink = false;
				g_string_append (str, "&quot;");
				break;
			default:
				previous_islink = false;
				g_string_append_len (str, p, next - p);
				break;
		}
		p = next;
	}
	if (currentmarktag>0) {
		do {
			currentmarktag--;
			switch (marktags[currentmarktag]) {
				case 'b':
				case 'B':
					g_string_append (str, "</b>");
					break;
				case 'I':
					g_string_append (str, "</i>");
					break;
				case '+':
					g_string_append (str, "</sup>");
					break;
				case '-':
					g_string_append (str, "</sub>");
					break;
				case 'x':
				case 'l':
				case 'D':
				case 'L':
				case 'U':
					g_string_append (str, "</span>");
					break;
				default:
					break;
			}
		} while (currentmarktag>0);
	}
	return g_string_free (str, FALSE);
}

typedef struct _PwUserData {
	std::string *res;
	const gchar *oword;
	bool first_jbcy;
} PwUserData;

static void func_parse_passthrough(GMarkupParseContext *context, const gchar *passthrough_text, gsize text_len, gpointer user_data, GError **error)
{
	if (!g_str_has_prefix(passthrough_text, "<![CDATA["))
		return;
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	const gchar *text = passthrough_text+9;
	gsize len = text_len-9-3;
	while (g_ascii_isspace(*text)) {
		text++;
		len--;
	}
	while (len>0 && g_ascii_isspace(*(text+len-1))) {
		len--;
	}
	if (len==0)
		return;
	std::string *res = ((PwUserData*)user_data)->res;
	if (strcmp(element, "词典音标")==0) {
		if (!res->empty())
			*res+='\n';
		*res+="[<span foreground=\"blue\">";
		gchar *str = toUtfPhonetic(text, len);
		*res+=str;
		g_free(str);
		*res+="</span>]";
	} else if (strcmp(element, "单词原型")==0) {
		const gchar *oword = ((PwUserData*)user_data)->oword;
		if (strncmp(oword, text, len)) {
			if (!res->empty())
				*res+='\n';
			*res+="<b>";
			gchar *str = g_markup_escape_text(text, len);
			res->append(str);
			g_free(str);
			*res+="</b>";
		}
	} else if (strcmp(element, "单词词性")==0) {
		if (!res->empty())
			*res+='\n';
		*res+="<i>";
		gchar *str = powerword_markup_escape_text(text, len);
		res->append(str);
		g_free(str);
		*res+="</i>";
	} else if (strcmp(element, "汉语拼音")==0) {
		if (!res->empty())
			*res+='\n';
		*res+="<span foreground=\"blue\" underline=\"single\">";
		gchar *str = powerword_markup_escape_text(text, len);
		res->append(str);
		g_free(str);
		*res+="</span>";
	} else if (strcmp(element, "例句原型")==0) {
		if (!res->empty())
			*res+='\n';
		*res+="<span foreground=\"#008080\">";
		gchar *str = powerword_markup_escape_text(text, len);
		res->append(str);
		g_free(str);
		*res+="</span>";
	} else if (strcmp(element, "例句解释")==0) {
		if (!res->empty())
			*res+='\n';
		*res+="<span foreground=\"#01259A\">";
		gchar *str = powerword_markup_escape_text(text, len);
		res->append(str);
		g_free(str);
		*res+="</span>";
	/*} else if (strcmp(element, "相关词")==0) {
		if (!res->empty())
			*res+='\n';
		std::string tabstr;
		tabstr+=text[0];
		for (gsize i=1;i<len;i++) {
			if (text[i]=='&')
				tabstr+="\t&";
			else
				tabstr+=text[i];
		}
		gchar *str = powerword_markup_escape_text(tabstr.c_str(), tabstr.length());
		res->append(str);
		g_free(str);*/
	} else
	/*} else if (
	strcmp(element, "解释项")==0 ||
	strcmp(element, "跟随解释")==0 ||
	strcmp(element, "相关词")==0 ||
	strcmp(element, "预解释")==0 ||
	strcmp(element, "繁体写法")==0 ||
	strcmp(element, "台湾音标")==0 ||
	strcmp(element, "图片名称")==0 ||
	strcmp(element, "跟随注释")==0 ||
	strcmp(element, "音节分段")==0 ||
	strcmp(element, "AHD音标")==0 ||
	strcmp(element, "国际音标")==0 ||
	strcmp(element, "美国音标")==0 ||
	strcmp(element, "子解释项")==0 ||
	strcmp(element, "同义词")==0 ||
	strcmp(element, "日文发音")==0 ||
	strcmp(element, "惯用型原型")==0 ||
	strcmp(element, "惯用型解释")==0 ||
	strcmp(element, "另见")==0
	) {*/
	{
		if (!res->empty())
			*res+='\n';
		gchar *str = powerword_markup_escape_text(text, len);
		res->append(str);
		g_free(str);
	}
}

static void func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	std::string *res = ((PwUserData*)user_data)->res;
	if (strcmp(element_name, "基本词义")==0) {
		if (((PwUserData*)user_data)->first_jbcy) {
			((PwUserData*)user_data)->first_jbcy = false;
		} else {
			*res+="\n<span foreground=\"blue\">&lt;基本词义&gt;</span>";
		}
	} else if (strcmp(element_name, "继承用法")==0) {
		*res+="\n<span foreground=\"blue\">&lt;继承用法&gt;</span>";
	} else if (strcmp(element_name, "习惯用语")==0) {
		*res+="\n<span foreground=\"blue\">&lt;习惯用语&gt;</span>";
	} else if (strcmp(element_name, "词性变化")==0) {
		*res+="\n<span foreground=\"blue\">&lt;词性变化&gt;</span>";
	} else if (strcmp(element_name, "特殊用法")==0) {
		*res+="\n<span foreground=\"blue\">&lt;特殊用法&gt;</span>";
	} else if (strcmp(element_name, "参考词汇")==0) {
		*res+="\n<span foreground=\"blue\">&lt;参考词汇&gt;</span>";
	} else if (strcmp(element_name, "常用词组")==0) {
		*res+="\n<span foreground=\"blue\">&lt;常用词组&gt;</span>";
	} else if (strcmp(element_name, "语源")==0) {
		*res+="\n<span foreground=\"blue\">&lt;语源&gt;</span>";
	} else if (strcmp(element_name, "派生")==0) {
		*res+="\n<span foreground=\"blue\">&lt;派生&gt;</span>";
	} else if (strcmp(element_name, "用法")==0) {
		*res+="\n<span foreground=\"blue\">&lt;用法&gt;</span>";
	} else if (strcmp(element_name, "注释")==0) {
		*res+="\n<span foreground=\"blue\">&lt;注释&gt;</span>";
	}
}

static std::string powerword2pango(const char *p, guint32 sec_size, const gchar *oword)
{
	std::string res;
	PwUserData Data;
	Data.res = &res;
	Data.oword = oword;
	Data.first_jbcy = true;

	GMarkupParser parser;
	parser.start_element = func_parse_start_element;
	parser.end_element = NULL;
	parser.text = NULL;
	parser.passthrough = func_parse_passthrough;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, p, sec_size, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
	return res;
}

static std::string wiki2pango(const char *p, guint32 sec_size)
{
	//Need more work!
	//There should be a wiki2xml tool in the internet, but I haven't find it yet.
	std::string res(p, sec_size);
	return res;
}

static std::string xdxf2pango(const char *p)
{
  std::string res;
  for (; *p; ++p) {
    if (*p!='<') {
      res+=*p;
      continue;
    }

    const char *next=strchr(p, '>');
    if (!next)
      continue;

    std::string name(p+1, next-p-1);

    if (name=="abr")
      res+="<span foreground=\"green\" style=\"italic\">";
    else if (name=="/abr")
      res+="</span>";
    else if (name=="k") {
      const char *begin=next;
      if ((next=strstr(begin, "</k>"))!=NULL)
	next+=sizeof("</k>")-1-1;
      else
	next=begin;
    } else if (name=="b")
      res+="<b>";
    else if (name=="/b")
      res+="</b>";
    else if (name=="i")
      res+="<i>";
    else if (name=="/i")
      res+="</i>";
	else if (name=="tr")
	  res+="<b>[";
	else if (name=="/tr")
	  res+="]</b>";
    else if (name=="ex")
      res+="<span foreground=\"violet\">";
    else if (name=="/ex")
      res+="</span>";
    else if (!name.empty() && name[0]=='c' && name!="co") {
      std::string::size_type pos=name.find("code");
      if (pos!=std::string::size_type(-1)) {
	pos+=sizeof("code=\"")-1;
	std::string::size_type end_pos=name.find("\"");
	std::string color(name, pos, end_pos-pos);
	res+="<span foreground=\""+color+"\">";
      } else {
	res+="<span foreground=\"blue\">";
      }
    } else if (name=="/c")
      res+="</span>";

    p=next;
  }
  return res;
}

void ArticleView::AppendData(gchar *data, const gchar *oword)
{  
  std::string mark;

  guint32 data_size,sec_size=0;
  const gchar *p=data;
  data_size=*reinterpret_cast<const guint32 *>(p);
  p+=sizeof(guint32);
  bool first_time = true;
  while (guint32(p - data)<data_size) {
    if (first_time)
      first_time=false;
    else
      mark+= "\n";
    switch (*p) {
    case 'm':
    case 'l'://need more work...
      p++;
      sec_size = strlen(p);
      if (sec_size) {
	gchar *m_str = g_markup_escape_text(p, sec_size);
	mark+=m_str;
	g_free(m_str);
      }
      sec_size++;
      break;
    case 'g':
      p++;
      sec_size=strlen(p);
      if (sec_size) {
	//AppendPangoText(p);
	mark+=p;
      }
      sec_size++;
      break;
    case 'x':
      p++;
      sec_size = strlen(p);
      if (sec_size) {
	std::string res=xdxf2pango(p);
	mark+=res;
      }
      sec_size++;
      break;
    case 'k':
      p++;
      sec_size = strlen(p);
      if (sec_size) {
        std::string res=powerword2pango(p, sec_size, oword);
        mark+=res;
      }
      sec_size++;
      break;
    case 'w':
      p++;
      sec_size = strlen(p);
      if (sec_size) {
        std::string res=wiki2pango(p, sec_size);
        mark+=res;
      }
      sec_size++;
      break;
    case 't':
      p++;
      sec_size = strlen(p);
      if (sec_size) {
	mark += "[<span foreground=\"blue\">";
	gchar *m_str = g_markup_escape_text(p, sec_size);
	mark += m_str;
	g_free(m_str);
	mark += "</span>]";
      }
      sec_size++;						
      break;
    case 'y':
      p++;
      sec_size = strlen(p);
      if (sec_size) {
	mark += "[<span foreground=\"red\">";
	gchar *m_str = g_markup_escape_text(p, sec_size);
	mark += m_str;
	g_free(m_str);
	mark += "</span>]";
      }
      sec_size++;
      break;
    case 'W':
      p++;
      sec_size=g_ntohl(*reinterpret_cast<const guint32 *>(p));
      //enbale sound button.
      sec_size += sizeof(guint32);
      break;
    case 'P':
      p++;
      sec_size=g_ntohl(*reinterpret_cast<const guint32 *>(p));
      //show this picture.
      sec_size += sizeof(guint32);
      break;
    default:
      if (g_ascii_isupper(*p)) {
        p++;
        sec_size=g_ntohl(*reinterpret_cast<const guint32 *>(p));
        sec_size += sizeof(guint32);
      } else {
        p++;
        sec_size = strlen(p)+1;
      }
      break;
    }
    p += sec_size;
  }
  AppendPangoText(mark.c_str());
}

void ArticleView::AppendNewline()
{
	AppendPangoText("\n");
}

void ArticleView::AppendDataSeparate()
{
	AppendPangoText("\n");
}

void ArticleView::AppendHeader(const std::string& dict_name, int i)
{
	if (!for_float_win) {
		gchar *mark = g_strdup_printf("%d", i);
		AppendMark(mark);
		g_free(mark);
	}
	std::string mark= "<span foreground=\"blue\">";
#ifdef CONFIG_GPE
	mark+= "&lt;- ";
#else
	mark+= "&lt;--- ";
#endif
	gchar *m_str = g_markup_escape_text(dict_name.c_str(), -1);
	mark += m_str;
	g_free(m_str);
#ifdef CONFIG_GPE
	mark += " -&gt;</span>\n";
#else
	mark += " ---&gt;</span>\n";
#endif
	AppendPangoText(mark.c_str());
}

void ArticleView::AppendWord(const gchar *word)
{
	std::string mark;
	mark += for_float_win ? "<span foreground=\"purple\">" : "<b><span size=\"x-large\">";
	gchar *m_str = g_markup_escape_text(word, -1);
	mark += m_str;
	g_free(m_str);
	mark += for_float_win ? "</span>\n" : "</span></b>\n";
	AppendPangoText(mark.c_str());
}
