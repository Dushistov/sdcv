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

/*
 * Original code I get from gtk bugzilla:
 * http://bugzilla.gnome.org/show_bug.cgi?id=59390
 * hope that this code will be in official version of gtk soon
 */


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gtktextviewpango.h"

static void
gtk_text_buffer_real_insert_markup (GtkTextBuffer *buffer,
                                    GtkTextIter   *textiter,
                                    const gchar   *markup,
                                    GtkTextTag    *extratag)
{
  PangoAttrIterator  *paiter;
  PangoAttrList      *attrlist;
  GtkTextMark        *mark;
  GError             *error = NULL;
  gchar              *text;

  g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (textiter != NULL);
  g_return_if_fail (markup != NULL);
  g_return_if_fail (gtk_text_iter_get_buffer (textiter) == buffer);

  if (*markup == '\000')
    return;

  if (!pango_parse_markup(markup, -1, 0, &attrlist, &text, NULL, &error))
    {
      g_warning("Invalid markup string: %s", error->message);
      g_error_free(error);
      return;
    }

  if (attrlist == NULL)
    {
      gtk_text_buffer_insert(buffer, textiter, text, -1);
      g_free(text);
      return;
    }

  /* create mark with right gravity */
  mark = gtk_text_buffer_create_mark(buffer, NULL, textiter, FALSE);

  paiter = pango_attr_list_get_iterator(attrlist);

  do
    {
      PangoAttribute *attr;
      GtkTextTag     *tag;
      gint            start, end;

      pango_attr_iterator_range(paiter, &start, &end);

      if (end == G_MAXINT)  /* last chunk */
        end = start-1; /* resulting in -1 to be passed to _insert */

      tag = gtk_text_tag_new(NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_LANGUAGE)))
        g_object_set(tag, "language", pango_language_to_string(((PangoAttrLanguage*)attr)->value), NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FAMILY)))
        g_object_set(tag, "family", ((PangoAttrString*)attr)->value, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STYLE)))
        g_object_set(tag, "style", ((PangoAttrInt*)attr)->value, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_WEIGHT)))
        g_object_set(tag, "weight", ((PangoAttrInt*)attr)->value, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_VARIANT)))
        g_object_set(tag, "variant", ((PangoAttrInt*)attr)->value, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STRETCH)))
        g_object_set(tag, "stretch", ((PangoAttrInt*)attr)->value, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SIZE)))
        g_object_set(tag, "size", ((PangoAttrInt*)attr)->value, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FONT_DESC)))
        g_object_set(tag, "font-desc", ((PangoAttrFontDesc*)attr)->desc, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FOREGROUND)))
        {
          GdkColor col = { 0,
                           ((PangoAttrColor*)attr)->color.red,
                           ((PangoAttrColor*)attr)->color.green,
                           ((PangoAttrColor*)attr)->color.blue
                         };

          g_object_set(tag, "foreground-gdk", &col, NULL);
        }

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_BACKGROUND)))
        {
          GdkColor col = { 0,
                           ((PangoAttrColor*)attr)->color.red,
                           ((PangoAttrColor*)attr)->color.green,
                           ((PangoAttrColor*)attr)->color.blue
                         };

          g_object_set(tag, "background-gdk", &col, NULL);
        }

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_UNDERLINE)))
        g_object_set(tag, "underline", ((PangoAttrInt*)attr)->value, NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STRIKETHROUGH)))
        g_object_set(tag, "strikethrough", (gboolean)(((PangoAttrInt*)attr)->value != 0), NULL);

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_RISE)))
        g_object_set(tag, "rise", ((PangoAttrInt*)attr)->value, NULL);

      /* PANGO_ATTR_SHAPE cannot be defined via markup text */

      if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SCALE)))
        g_object_set(tag, "scale", ((PangoAttrFloat*)attr)->value, NULL);

      gtk_text_tag_table_add(gtk_text_buffer_get_tag_table(buffer), tag);

      if (extratag)
        {
          gtk_text_buffer_insert_with_tags(buffer, textiter, text+start, end - start, tag, extratag, NULL);
        }
      else
        {
          gtk_text_buffer_insert_with_tags(buffer, textiter, text+start, end - start, tag, NULL);
        }

      /* mark had right gravity, so it should be
       *  at the end of the inserted text now */
      gtk_text_buffer_get_iter_at_mark(buffer, textiter, mark);
    }
  while (pango_attr_iterator_next(paiter));

  gtk_text_buffer_delete_mark(buffer, mark);
  pango_attr_iterator_destroy(paiter);
  pango_attr_list_unref(attrlist);
  g_free(text);
}


/**
 * gtk_text_buffer_insert_markup:
 * @buffer: a #GtkTextBuffer
 * @iter: a position in the buffer
 * @markup: nul-terminated UTF-8 format text with pango markup to insert
 *
 * Inserts the text in @markup at position @iter. @markup
 * will be inserted in its entirety and must be nul-terminated
 * and valid UTF-8. Emits the "insert_text" signal, possibly multiple
 * times; insertion actually occurs in the default handler for
 * the signal. @iter will point to the end of the inserted text
 * on return
 *
 * Since: 2.6
 **/

void
gtk_text_buffer_insert_markup (GtkTextBuffer *buffer,
                               GtkTextIter   *iter,
                               const gchar   *markup)
{
  gtk_text_buffer_real_insert_markup (buffer, iter, markup, NULL);
}


/**
 * gtk_text_buffer_insert_markup_with_tag:
 * @buffer: a #GtkTextBuffer
 * @iter: a position in the buffer
 * @markup: nul-terminated UTF-8 format text in pango markup format to insert
 * @tag: additional text tag to apply to the whole text
 *
 * Just like <literal>gtk_text_buffer_insert_markup</literal>, only
 * that an additional tag can be specified that is applied to the
 * whole text to be inserted. This is useful to pass formatting
 * options to the text buffer that cannot be specified with
 * pango markup (e.g. text justification or wrap mode).
 * @markup will be inserted in its entirety and must be
 * nul-terminated and valid UTF-8 format
 *
 * Since: 2.6
 **/

void
gtk_text_buffer_insert_markup_with_tag (GtkTextBuffer *buffer,
                                        GtkTextIter   *iter,
                                        const gchar   *markup,
                                        GtkTextTag    *tag)
{
  gtk_text_buffer_real_insert_markup (buffer, iter, markup, tag);
}


/**
 * gtk_text_buffer_set_markup:
 * @buffer: a #GtkTextBuffer
 * @markup: nul-terminated UTF-8 text with pango markup to insert
 *
 * Deletes current contents of @buffer, and inserts the text
 * in @markup instead, which may contain pango markup.
 * @markup will be inserted in its entirety and must be
 * nul-terminated and valid UTF-8.
 *
 * Since: 2.6
 **/

void
gtk_text_buffer_set_markup_with_tag (GtkTextBuffer *buffer,
                                     const gchar   *markup,
                                     GtkTextTag    *tag)
{
  GtkTextIter  start, end;

  g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (markup != NULL);

  if (*markup == '\000')
    return;

  gtk_text_buffer_get_bounds (buffer, &start, &end);

  gtk_text_buffer_delete (buffer, &start, &end);

  gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
  gtk_text_buffer_insert_markup_with_tag (buffer, &start, markup, tag);
}

/**
 * gtk_text_buffer_set_markup:
 * @buffer: a #GtkTextBuffer
 * @markup: nul-terminated UTF-8 text with pango markup to insert
 *
 * Deletes current contents of @buffer, and inserts the text
 * in @markup instead, which may contain pango markup and must
 * be valid UTF-8. @markup will be inserted in its entirety and
 * must be nul-terminated.
 *
 * Since: 2.6
 **/

void
gtk_text_buffer_set_markup (GtkTextBuffer *buffer,
                            const gchar   *markup)
{
  gtk_text_buffer_set_markup_with_tag(buffer, markup, NULL);
}
