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

#define SNIPPET_TYPE            (snippet_get_type ())
#define SNIPPET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SNIPPET_TYPE, Snippet))
#define SNIPPET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SNIPPET_TYPE, SnippetClass))
#define IS_SNIPPET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNIPPET_TYPE))
#define IS_SNIPPET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SNIPPET_TYPE))

typedef struct _Snippet Snippet;
typedef struct _SnippetClass SnippetClass;

/* The snippet key is formed by concatenating the trigger_key, "." and snippet_language.
   Example: If the trigger key is "class" and the snippet language is "C++", the snippet key
   will be: "class.C++" */
struct _Snippet
{
	/* The trigger key of the snippet. Unique per language. */
	gchar* trigger_key;

	/* A list of languages for which the snippet is meant. */
	gchar* snippet_language;

	/* A short, intuitive name for the snippet. */
	gchar* snippet_name;
	
	/* The actual snippet content. */
	gchar* snippet_content;

	/* A list of the variables (SnippetVariable) in the snippet. */
	GList* variables;

	/* A list of keywords (gchar*) for searching purposes. */
	GList* keywords;
	
	/* Characters inserted since the user started to edit the snippet. */
	gint32 chars_inserted;
};

struct _SnippetClass
{
	GObjectClass klass;

};


GType			snippet_get_type						(void);
Snippet*		snippet_new								(const gchar* trigger_key,
														 const gchar* snippet_language,
														 const gchar* snippet_name,
														 const gchar* snippet_content,
														 GList* variable_names,
														 GList* variable_default_values,
														 GList* keywords);
gchar*			snippet_get_key							(Snippet* snippet);
const gchar*	snippet_get_trigger_key					(Snippet* snippet);
const gchar*	snippet_get_language					(Snippet* snippet);
const gchar*	snippet_get_name						(Snippet* snippet);
GList*			snippet_get_keywords_list				(Snippet* snippet);
GList*			snippet_get_variables_list				(Snippet* snippet);
gchar*			snippet_get_default_content				(Snippet* snippet);
void			snippet_set_editing_mode 				(Snippet* snippet);
gint32			snippet_get_next_variable_relative_pos	(Snippet* snippet);
void			snippet_update_chars_inserted			(Snippet* snippet,
														 gint32 inserted_chars);
void			snippet_clear_editing_mode 				(Snippet* snippet);



#endif /* __ANJUTA_SNIPPET_H__ */
