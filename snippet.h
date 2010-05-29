/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet.h
    Copyright (C) Dragos Dena 2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#ifndef __ANJUTA_SNIPPET_H__
#define __ANJUTA_SNIPPET_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_SNIPPET            (snippet_get_type ())
#define ANJUTA_SNIPPET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPET, AnjutaSnippet))
#define ANJUTA_SNIPPET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPET, AnjutaSnippetClass))
#define ANJUTA_IS_SNIPPET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPET))
#define ANJUTA_IS_SNIPPET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPET))

typedef struct _AnjutaSnippet AnjutaSnippet;
typedef struct _AnjutaSnippetPrivate AnjutaSnippetPrivate;
typedef struct _AnjutaSnippetClass AnjutaSnippetClass;

struct _AnjutaSnippet
{
	GObject parent_instance;

	/* A pointer to an AnjutaSnippetsGroup object. */
	GObject *parent_snippets_group;
	
	/*< private >*/
	AnjutaSnippetPrivate *priv;
};

struct _AnjutaSnippetClass
{
	GObjectClass parent_class;

};

GType           snippet_get_type                        (void) G_GNUC_CONST;
AnjutaSnippet*  snippet_new                             (const gchar* trigger_key,
                                                         const gchar* snippet_language,
                                                         const gchar* snippet_name,
                                                         const gchar* snippet_content,
                                                         GList* variable_names,
                                                         GList* variable_default_values,
                                                         GList* variable_globals,
                                                         GList* keywords);
gchar*          snippet_get_key                         (AnjutaSnippet* snippet);
const gchar*    snippet_get_trigger_key                 (AnjutaSnippet* snippet);
const gchar*    snippet_get_language                    (AnjutaSnippet* snippet);
const gchar*    snippet_get_name                        (AnjutaSnippet* snippet);
GList*          snippet_get_keywords_list               (AnjutaSnippet* snippet);
GList*          snippet_get_variable_names_list         (AnjutaSnippet* snippet);
GList*          snippet_get_variable_defaults_list      (AnjutaSnippet* snippet);
GList*          snippet_get_variable_globals_list       (AnjutaSnippet* snippet);
const gchar*    snippet_get_content                     (AnjutaSnippet* snippet);
gchar*          snippet_get_default_content             (AnjutaSnippet* snippet);
void            snippet_set_editing_mode                (AnjutaSnippet* snippet);
GList*          snippet_get_variable_relative_positions (AnjutaSnippet* snippet,
                                                         const gchar* variable_name);
void            snippet_update_chars_inserted           (AnjutaSnippet* snippet,
                                                         gint32 inserted_chars);

G_END_DECLS

#endif /* __ANJUTA_SNIPPET_H__ */
