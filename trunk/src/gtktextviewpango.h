#ifndef TEXT_VIEW_PANGO_H
#define TEXT_VIEW_PANGO_H

#include <gtk/gtk.h>

extern void
gtk_text_buffer_insert_markup (GtkTextBuffer *buffer,
                               GtkTextIter   *iter,
                               const gchar   *markup);

#endif//tvpango.h
