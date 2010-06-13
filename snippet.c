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

#include <string.h>
#include "snippet.h"
#include "snippets-db.h"
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

static void
double_allocated_memory (gchar **text, gint *allocated)
{
	gint i = 0;
	
	*allocated *= 2;
	*text = g_realloc (*text, *allocated);

	for (i = *allocated/2; i < *allocated; i ++)
		(*text)[i] = 0;
}

static gchar *
expand_global_and_default_variables (AnjutaSnippet *snippet,
                                     SnippetsDB *snippets_db)
{
	gchar *buffer = NULL, *snippet_text = NULL;
	gint allocated = 0, snippet_text_size = 0, i = 0, j = 0, k = 0;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      ANJUTA_IS_SNIPPET (snippet),
	                      NULL);
	snippet_text = snippet->priv->snippet_content;

	/* We initially allocate buffer the same amount of memory as snippet_text. Each
	   time more memory will be needed the allocated amount will be doubled */
	snippet_text_size = strlen (snippet_text);
	allocated = snippet_text_size;
	buffer = g_malloc0 (allocated * sizeof (gchar));

	/* We expand the variables to the default value or if they are global variables
	   we query the database for their value. If the database can't answer to our
	   request, we fill them also with their default values */
	for (i = 0; i < snippet_text_size; i ++)
	{
		/* If it's the start of a variable name, we look up the end, get the name
		   and evaluate it. */
		if (snippet_text[i] == '$' && snippet_text[i + 1] == '{')
		{
			gchar *cur_var_name = NULL, *cur_var_value = NULL;
			gint cur_var_size = 0, cur_var_value_size = 0;
			AnjutaSnippetVariable *cur_var = NULL;
			GList *iter = NULL;
			
			/* We search for the "}" */
			for (k = i + 2; k < snippet_text_size; k ++)
			{
				/* This isn't needed, but for the code to be clearer */
				cur_var_size ++;
				
				if (snippet_text[k + 1] == '}')
					break;
			}
			cur_var_name = g_malloc0 ((cur_var_size + 1) * sizeof (gchar));
			g_strlcpy (cur_var_name, snippet_text + i + 2, cur_var_size + 1);
			i = k + 1;
			
			/* Look up the variable */
			for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
			{
				cur_var = (AnjutaSnippetVariable *)iter->data;
				if (!g_strcmp0 (cur_var->variable_name, cur_var_name))
					break;
			}

			/* If iter is NULL, then we reached the end so the snippet is corrupt */
			if (!iter)
			{
				g_return_val_if_reached (NULL);
			}
			else
			{
				/* If it's a global variable, we query the database */
				if (cur_var->is_global)
					cur_var_value = snippets_db_get_global_variable (snippets_db, cur_var_name);

				/* If we didn't got an answer from the database or if the variable is not
				   global, we get the default value. */
				if (cur_var_value == NULL)
					cur_var_value = g_strdup (cur_var->default_value);
			}

			cur_var_value_size = strlen (cur_var_value);
			for (k = 0; k < cur_var_value_size; k ++, j ++)
			{
				buffer[j] = cur_var_value[k];
				
				if (allocated - 1 == j)
					double_allocated_memory (&buffer, &allocated);
			}

			g_free (cur_var_value);
			g_free (cur_var_name);
		}
		else
		{
			buffer[j] = snippet_text[i];
			j ++;

			if (allocated - 1 == j)
				double_allocated_memory (&buffer, &allocated);	
		}
	}

	return buffer;
}

static gchar *
get_text_with_indentation (const gchar *text,
                           const gchar *indent)
{
	gint newline_no = 0, indent_size = 0, text_size = 0, i = 0, j = 0, k = 0,
	     text_with_indentation_size = 0;
	gchar *text_with_indentation = NULL;
	
	/* Assertions */
	g_return_val_if_fail (text != NULL && indent != NULL,
	                      NULL);

	/* Calculate the string sizes */
	indent_size = strlen (indent);
	text_size = strlen (text);

	/* Calculate the number of newlines in text */
	for (i = 0; i < text_size; i ++)
		if (text[i] == '\n')
			newline_no ++;

	/* Since each newline will be replace with newline+indent, we calculate the
	   size of the new string */	
	text_with_indentation_size = (newline_no * indent_size + text_size + 1) * sizeof (gchar);

	/* Allocate memory for the new string */
	text_with_indentation = g_malloc0 (text_with_indentation_size);

	/* Compute the new string */
	for (i = 0, j = 0; i < text_size; i ++)
	{
		text_with_indentation[j] = text[i];
		j ++;
		
		if (text[i] == '\n')
		{
			for (k = 0; k < indent_size; k ++, j ++)
			{
				text_with_indentation[j] = indent[k];
			}
		}
	}
		
	return text_with_indentation;
}

/**
 * snippet_get_default_content:
 * @snippet: A #AnjutaSnippet object.
 * @snippets_db: A #SnippetsDB object. This is required for filling the global variables.
 *               This can be NULL if the snippet is independent of a #SnippetsDB or if
 *               it doesn't have global variables.
 * @indent: The indentation of the line where the snippet will be inserted.
 *
 * The content of the snippet filled with the default values of the variables.
 * Every '\n' character will be replaced with a string obtained by concatanating
 * "\n" with indent.
 *
 * Returns: The default content of the snippet or NULL if @snippet is invalid.
 **/
gchar*
snippet_get_default_content (AnjutaSnippet *snippet,
                             GObject *snippets_db_obj,
                             const gchar *indent)
{
	gchar* buffer = NULL, *temp = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippet != NULL && ANJUTA_IS_SNIPPET (snippet),
	                      NULL);

	temp = snippet->priv->snippet_content;

	/* If we should expand the global variables */
	if (snippets_db_obj && ANJUTA_IS_SNIPPETS_DB (snippets_db_obj))
	{
		/* Expand the global variables */
		temp = expand_global_and_default_variables (snippet, 
		                                            ANJUTA_SNIPPETS_DB (snippets_db_obj));
	}
	
	/* Get the text with indentation */
	if (temp)
	{
		buffer = get_text_with_indentation (temp, indent);
		g_free (temp);
	}
	else
	{
		buffer = get_text_with_indentation (snippet->priv->snippet_content, indent);
	}
	
	return buffer;
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
