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


#define ANJUTA_SNIPPET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPET, AnjutaSnippetPrivate))

struct _AnjutaSnippetPrivate
{
	gchar* trigger_key;
	gchar* snippet_language;
	gchar* snippet_name;
	
	gchar* snippet_content;
	
	GList* variables;
	GList* keywords;
	
	gint32 chars_inserted;
};


G_DEFINE_TYPE (AnjutaSnippet, snippet, G_TYPE_OBJECT);

static void
snippet_dispose (GObject* snippet)
{
	AnjutaSnippet* anjuta_snippet = ANJUTA_SNIPPET (snippet);
	GList* iter = NULL;
	gpointer p;
	AnjutaSnippetVariable* cur_snippet_var;

	/* Delete the trigger_key, snippet_language, snippet_name and snippet_content fields */
	g_free (anjuta_snippet->priv->trigger_key);
	anjuta_snippet->priv->trigger_key = NULL;
	g_free (anjuta_snippet->priv->snippet_language);
	anjuta_snippet->priv->snippet_language = NULL;
	g_free (anjuta_snippet->priv->snippet_name);
	anjuta_snippet->priv->snippet_name = NULL;
	g_free (anjuta_snippet->priv->snippet_content);
	anjuta_snippet->priv->snippet_content = NULL;
	
	/* Delete the keywords */
	for (iter = g_list_first (anjuta_snippet->priv->keywords); iter != NULL; iter = g_list_next (iter))
	{
		p = iter->data;
		g_free (p);
	}
	g_list_free (anjuta_snippet->priv->keywords);
	anjuta_snippet->priv->keywords = NULL;
	
	/* Delete the snippet variables */
	for (iter = g_list_first (anjuta_snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		
		g_free (cur_snippet_var->variable_name);
		g_free (cur_snippet_var->default_value);
		g_free (cur_snippet_var->relative_positions);
		
		g_free (cur_snippet_var);
	}
	g_list_free (anjuta_snippet->priv->variables);

	G_OBJECT_CLASS (snippet_parent_class)->dispose (snippet);
}

static void
snippet_finalize (GObject* snippet)
{
	G_OBJECT_CLASS (snippet_parent_class)->finalize (snippet);
}

static void
snippet_class_init (AnjutaSnippetClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippet_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippet_dispose;
	object_class->finalize = snippet_finalize;

	g_type_class_add_private (klass, sizeof (AnjutaSnippetPrivate));
}

static void
snippet_init (AnjutaSnippet* snippet)
{
	AnjutaSnippetPrivate* priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);
	
	snippet->priv = priv;
	snippet->parent_snippets_group = NULL;

	/* Initialize the private field */
	snippet->priv->trigger_key = NULL;
	snippet->priv->snippet_language = NULL;
	snippet->priv->snippet_name = NULL;
	snippet->priv->snippet_content = NULL;
	snippet->priv->variables = NULL;
	snippet->priv->keywords = NULL;
	snippet->priv->chars_inserted = 0;
}

/**
 * snippet_new:
 * @trigger_key: The trigger key of the snippet.
 * @snippet_language: The language for which the snippet is meant.
 * @snippet_name: A short, intuitive name of the snippet.
 * @snippet_content: The actual content of the snippet.
 * @variables_names: A #GList with the variable names.
 * @variables_default_values: A #GList with the default values of the variables.
 * @variable_globals: A #GList with gboolean's that state if the variable is global or not.
 * @keywords: A #GList with the keywords of the snippet.
 *
 * Creates a new snippet object. All the data given to this function will be copied.
 *
 * Returns: The new #AnjutaSnippet or NULL on failure.
 **/
AnjutaSnippet* 
snippet_new (const gchar* trigger_key,
             const gchar* snippet_language,
             const gchar* snippet_name,
             const gchar* snippet_content,
             GList* variable_names,
             GList* variable_default_values,
             GList* variable_globals,
             GList* keywords)
{
	AnjutaSnippet* snippet = NULL;
	GList *iter1 = NULL, *iter2 = NULL, *iter3 = NULL;
	gchar* temporary_string_holder = NULL;
	AnjutaSnippetVariable* cur_snippet_var = NULL;
	
	/* Assertions */
	g_return_val_if_fail (trigger_key != NULL &&\
	                      snippet_language != NULL &&\
	                      snippet_name != NULL &&\
	                      snippet_content != NULL,\
	                      NULL);
	g_return_val_if_fail (g_list_length (variable_names) == g_list_length (variable_default_values) &&\
	                      g_list_length (variable_names) == g_list_length (variable_globals),\
	                      NULL);
	
	/* Initialize the object */
	snippet = ANJUTA_SNIPPET (g_object_new (snippet_get_type (), NULL));
	
	/* Make a copy of the given strings */
	snippet->priv->trigger_key      = g_strdup (trigger_key);
	snippet->priv->snippet_language = g_strdup (snippet_language);
	snippet->priv->snippet_name     = g_strdup (snippet_name);
	snippet->priv->snippet_content  = g_strdup (snippet_content);
	
	/* Copy all the keywords to a new list */
	snippet->priv->keywords = NULL;
	for (iter1 = g_list_first (keywords); iter1 != NULL; iter1 = g_list_next (iter1))
	{
		temporary_string_holder = g_strdup ((gchar*)iter1->data);
		snippet->priv->keywords = g_list_append (snippet->priv->keywords, temporary_string_holder);
	}
	
	/* Make a list of variables */
	snippet->priv->variables = NULL;
	iter1 = g_list_first (variable_names);
	iter2 = g_list_first (variable_default_values);
	iter3 = g_list_first (variable_globals);
	while (iter1 != NULL && iter2 != NULL && iter3 != NULL)
	{
		cur_snippet_var = g_malloc (sizeof (AnjutaSnippetVariable));
		
		cur_snippet_var->variable_name = g_strdup ((gchar*)iter1->data);
		cur_snippet_var->default_value = g_strdup ((gchar*)iter2->data);
		cur_snippet_var->is_global = GPOINTER_TO_INT (iter3->data);
		cur_snippet_var->appearances = 0;            /* TODO */
		cur_snippet_var->relative_positions = NULL;  /* TODO The relative positions should be calculated in a single
		                                                     snippet_content parsing after this block. */
		
		snippet->priv->variables = g_list_append (snippet->priv->variables, cur_snippet_var);

		iter1 = g_list_next (iter1);
		iter2 = g_list_next (iter2);
		iter3 = g_list_next (iter3);
	}
	
	return snippet;
}
														 
/**
 * snippet_get_key:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the snippet-key of the snippet which is unique at runtime. Should be freed.
 *
 * Returns: The snippet-key or NULL if @snippet is invalid.
 **/
gchar*	
snippet_get_key (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);
	
	return g_strconcat(snippet->priv->trigger_key, ".", snippet->priv->snippet_language, NULL);
}

/**
 * snippet_get_trigger_key:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the trigger-key of the snippet.
 *
 * Returns: The snippet-key or NULL if @snippet is invalid.
 **/
const gchar*
snippet_get_trigger_key (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	return snippet->priv->trigger_key;
}

/**
 * snippet_get_language:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the language of the snippet.
 *
 * Returns: The snippet language or NULL if @snippet is invalid.
 **/
const gchar*	
snippet_get_language (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	return snippet->priv->snippet_language;
}

/**
 * snippet_get_name:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the name of the snippet.
 *
 * Returns: The snippet name or NULL if @snippet is invalid.
 **/
const gchar*
snippet_get_name (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	return snippet->priv->snippet_name;
}

/**
 * snippet_get_keywords_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the list with the keywords of the snippet. The GList* should be free'd, but not
 * the containing data.
 *
 * Returns: A #GList of #gchar* with the keywords of the snippet or NULL if 
 *          @snippet is invalid.
 **/
GList*
snippet_get_keywords_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *keywords_copy = NULL;
	gchar *cur_keyword;

	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	for (iter = g_list_first (snippet->priv->keywords); iter != NULL; iter = g_list_next (iter))
	{
		cur_keyword = (gchar *)iter->data;
		keywords_copy = g_list_append (keywords_copy, cur_keyword);
	}
	
	return keywords_copy;
}

/**
 * snippet_get_variable_names_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables names, in the order they should be edited. The GList*
 * returned should be freed, but not the containing data.
 *
 * Returns: The variable names list or NULL if the @snippet is invalid.
 **/
GList*
snippet_get_variable_names_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *variable_names = NULL;
	AnjutaSnippetVariable *cur_snippet_var = NULL;

	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		variable_names = g_list_append (variable_names, cur_snippet_var->variable_name);
	}
	
	return variable_names;
}

/**
 * snippet_get_variable_defaults_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables default values, in the order they should be edited.
 * The GList* returned should be freed, but not the containing data.
 *
 * Returns: The variables default values #GList or NULL if @snippet is invalid.
 **/
GList*
snippet_get_variable_defaults_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *variable_defaults = NULL;
	AnjutaSnippetVariable *cur_snippet_var = NULL;

	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		variable_defaults = g_list_append (variable_defaults, cur_snippet_var->default_value);
	}
	
	return variable_defaults;
}

/**
 * snippet_get_variable_globals_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables global truth value, in the order they should be edited.
 * The GList* returned should be freed, but not the containing data.
 *
 * Returns: The variables global truth values #GList or NULL if @snippet is invalid.
 **/
GList* 
snippet_get_variable_globals_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *variable_globals = NULL;
	gboolean temp_holder = FALSE;
	AnjutaSnippetVariable *cur_snippet_var = NULL;

	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		temp_holder = cur_snippet_var->is_global;
		variable_globals = g_list_append (variable_globals, GINT_TO_POINTER (temp_holder));
	}
	
	return variable_globals;
}

/**
 * snippet_get_content:
 * @snippet: A #AnjutaSnippet object.
 *
 * The content of the snippet without it being proccesed.
 *
 * Returns: The content of the snippet or NULL if @snippet is invalid.
 **/
const gchar*
snippet_get_content (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	return snippet->priv->snippet_content;
}

/**
 * snippet_get_default_content:
 * @snippet: A #AnjutaSnippet object.
 *
 * The content of the snippet filled with the default values of the variables.
 *
 * Returns: The default content of the snippet or NULL if @snippet is invalid.
 **/
gchar*
snippet_get_default_content (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

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
	/* Assertions */
	g_return_if_fail (snippet != NULL && ANJUTA_IS_SNIPPET (snippet));

	/* TODO Compute the relative positions (may be changed after a previous editing mode) */
	snippet->priv->chars_inserted = 0;
}

/**
 * snippet_get_variable_relative_positions:
 * @snippet: A #AnjutaSnippet object.
 * @variable_name: The name of the variable for which the relative positions are requested.
 *
 * Gets a list with the relative positions of all the variables from the start of the snippet text
 * at the time the function is called.
 *
 * Returns: A #GList with the positions or NULL on failure.
 **/
GList*	
snippet_get_variable_relative_positions	(AnjutaSnippet* snippet,
                                         const gchar* variable_name)
{
	/* Assertions */
	g_return_val_if_fail (snippet != NULL &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

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
	/* Assertions */
	g_return_if_fail (snippet != NULL && ANJUTA_IS_SNIPPET (snippet));

	snippet->priv->chars_inserted += inserted_chars;
}
