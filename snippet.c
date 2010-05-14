/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet.c
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

#include "snippet.h"
#include <gio/gio.h>

/**
 * SnippetVariable:
 * @variable_name: the name of the variable.
 * @default_value: the default value as it will be inserted in the code.
 * @is_global: if the variable is global accross the SnippetDB. Eg: username or email.
 * @appearances: the number of appearances the variable has in the snippet.
 * @relative_positions: the relative positions from the start of the snippet code each instance of
 *                      this variable has.
 *
 * The snippet variable structure.
 *
 **/
typedef struct _AnjutaSnippetVariable
{
	gchar* variable_name;
	gchar* default_value;
	gboolean is_global;
	gint16 appearances;
	guint32* relative_positions;
} AnjutaSnippetVariable;


G_DEFINE_TYPE (AnjutaSnippet, snippet, G_TYPE_OBJECT);

static void
snippet_dispose (GObject* snippet)
{
	/* TODO */
	G_OBJECT_CLASS (snippet_parent_class)->dispose (snippet);
}

static void
snippet_finalize (GObject* snippet)
{
	/* TODO */
	G_OBJECT_CLASS (snippet_parent_class)->finalize (snippet);
}

static void
snippet_class_init (AnjutaSnippetClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippet_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippet_dispose;
	object_class->finalize = snippet_finalize;
}

static void
snippet_init (AnjutaSnippet* snippet)
{
	/* TODO */
}

/**
 * snippet_new:
 * @trigger_key: The trigger key of the snippet.
 * @snippet_language: The language for which the snippet is meant.
 * @snippet_name: A short, intuitive name of the snippet.
 * @snippet_content: The actual content of the snippet.
 * @variables_names: A #GList with the variable names.
 * @variables_default_values: A #GList with the default values of the variables.
 * @keywords: A #GList with the keywords of the snippet.
 *
 * Creates a new snippet object.
 *
 * Returns: The new #AnjutaSnippet or NULL on failure.
 **/
AnjutaSnippet* 
snippet_new	(const gchar* trigger_key,
             const gchar* snippet_language,
             const gchar* snippet_name,
             const gchar* snippet_content,
             GList* variable_names,
             GList* variable_default_values,
             GList* keywords)
{
	/* TODO */
	return NULL;
}
														 
/**
 * snippet_get_key:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the snippet-key of the snippet which is unique at runtime.
 *
 * Returns: The snippet-key. Should be freed.
 **/
gchar*	
snippet_get_key (AnjutaSnippet* snippet)
{
	return g_strconcat(snippet->trigger_key, ".", snippet->snippet_language, NULL);
}

/**
 * snippet_get_trigger_key:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the trigger-key of the snippet.
 *
 * Returns: The snippet-key.
 **/
const gchar*
snippet_get_trigger_key (AnjutaSnippet* snippet)
{
	return snippet->trigger_key;
}

/**
 * snippet_get_language:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the language of the snippet.
 *
 * Returns: The snippet language.
 **/
const gchar*	
snippet_get_language (AnjutaSnippet* snippet)
{
	return snippet->snippet_language;
}

/**
 * snippet_get_name:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the name of the snippet.
 *
 * Returns: The snippet name.
 **/
const gchar*
snippet_get_name (AnjutaSnippet* snippet)
{
	return snippet->snippet_name;
}

/**
 * snippet_get_keywords_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the list with the keywords of the snippet.
 *
 * Returns: A #GList of #gchar* with the keywords of the snippet. Shouldn't be freed.
 **/
GList*
snippet_get_keywords_list (AnjutaSnippet* snippet)
{
	return snippet->keywords;
}

/**
 * snippet_get_variable_names_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables names, in the order they should be edited.
 *
 * Returns: The variable names list. Should be freed.
 **/
GList*
snippet_get_variable_names_list (AnjutaSnippet* snippet)
{
	/* TODO */
	return NULL;
}

/**
 * snippet_get_variable_defaults_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables default values, in the order they should be edited.
 *
 * Returns: The variables default values #GList. Should be freed.
 **/
GList*
snippet_get_variable_defaults_list (AnjutaSnippet* snippet)
{
	/* TODO */
	return NULL;
}

/**
 * snippet_get_default_content:
 * @snippet: A #AnjutaSnippet object.
 *
 * The content of the snippet filled with the default values of the variables.
 *
 * Returns: The default content of the snippet.
 **/
gchar*
snippet_get_default_content (AnjutaSnippet* snippet)
{
	/* TODO */
	return NULL;
}

/**
 * snippet_set_editing_mode:
 * @snippet: A #AnjutaSnippet object.
 *
 * Should be called after the snippet is inserted to reset it's internal counter.
 *
 **/
void
snippet_set_editing_mode (AnjutaSnippet* snippet)
{
	/* TODO */
	snippet->chars_inserted = 0;
}

/**
 * snippet_get_variable_relative_positions:
 * @snippet: A #AnjutaSnippet object.
 * @variable_name: The name of the variable for which the relative positions are requested.
 *
 * Gets a list with the relative positions of all the variables from the start of the snippet text
 * at the time the function is called.
 *
 * Returns: A #GList with the positions.
 **/
GList*	
snippet_get_variable_relative_positions	(AnjutaSnippet* snippet,
                                         const gchar* variable_name)
{
	/* TODO */
	return NULL;
}

/**
 * snippet_update_chars_inserted:
 * @snippet: A #AnjutaSnippet object.
 * @inserted_chars: The number of characters were inserted/deleted after the last update.
 *
 * Updates the internal counter of the inserted characters so a next call of
 * #AnjutaSnippet_get_variable_relative_positions will be relevant.
 *
 **/
void
snippet_update_chars_inserted (AnjutaSnippet* snippet,
                               gint32 inserted_chars)
{
	/* TODO */
	snippet->chars_inserted += inserted_chars;
}
