/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-db.c
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

#include "snippets-db.h"
#include "snippets-xml-parser.h"
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#define DEFAULT_SNIPPETS_FILE PACKAGE_DATA_DIR"/default-snippets.xml"

#define ANJUTA_SNIPPETS_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_DB, SnippetsDBPrivate))

struct _SnippetsDBPrivate
{
	/* A tree model to be used by the Snippet Browser. */
	GtkTreeStore* snippets_tree;
	
	/* A binary balanced tree with the trigger keys for searching purposes. */
	GTree* trigger_keys_tree;
	
	/* A binary balanced tree with the keywords for searching purposes */
	GTree* keywords_tree;
	
	/* A hashtable with the snippet-key's as keys and pointers to the aproppiate Snippet as value */
	GHashTable* snippet_keys_map;
	
	/* A list of Snippet Group's. Here the actual Snippet's are stored. */
	GList* snippet_groups;
	
	/* A hashtable with the variables names as keys and SnippetsGlobalVariable as value. */
	GHashTable* global_variables;
};

typedef struct _SnippetsGlobalVariable
{
	/* If this is TRUE, then the value of the variable will be a shell command. Only supported on Linux. */
	gboolean is_shell_command;
	
	/* The value of the variable. This can be an actual value or a command to parse. */
	gchar* value;
} SnippetsGlobalVariable;

G_DEFINE_TYPE (SnippetsDB, snippets_db, G_TYPE_OBJECT);

static void
snippets_db_dispose (GObject* snippets_db)
{
	DEBUG_PRINT ("%s", "Disposing SnippetsDB");
	/* TODO */
	G_OBJECT_CLASS (snippets_db_parent_class)->dispose (snippets_db);
}

static void
snippets_db_finalize (GObject* snippets_db)
{
	DEBUG_PRINT ("%s", "Finalizing SnippetsDB");
	/* TODO */
	G_OBJECT_CLASS (snippets_db_parent_class)->finalize (snippets_db);
}

static void
snippets_db_class_init (SnippetsDBClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippets_db_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippets_db_dispose;
	object_class->finalize = snippets_db_finalize;
	
	g_type_class_add_private (klass, sizeof (SnippetsDBPrivate));
}

static void
snippets_db_init (SnippetsDB * snippets_db)
{
	/* TODO */
	SnippetsDBPrivate *priv;
	
	snippets_db->priv = priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);
	
}

/**
 * snippets_db_new:
 *
 * A new #SnippetDB with snippets loaded from the default folder.
 *
 * Returns: A new #SnippetsDB object.
 **/
SnippetsDB*	
snippets_db_new ()
{
	/* TODO */
	return NULL;
}

/**
 * snippets_db_load_file:
 * @snippets_db: A #SnippetsDB object where the file should be loaded.
 * @snippet_packet_file_path: A string with the file path.
 * @overwrite: If on conflicting snippets, it should overwrite them.
 * @conflicting_snippets: A #GList with the conflicting snippets if overwrite is FALSE.
 * @format_type: A #FormatType with the type of the snippet-packet XML file.
 *
 * Loads the given file in the #SnippetsDB 
 *
 * Returns: The ID of the loaded file (to be saved later) or -1 on failure.
 **/
gint32	
snippets_db_load_file (SnippetsDB* snippets_db,
                       const gchar* snippet_packet_file_path,
                       gboolean overwrite,
                       GList* conflicting_snippets,
                       FormatType format_type)
{
	/* TODO */
	return -1;
}

/**
 * snippets_db_save_file:
 * @snippets_db: A #SnippetsDB object where the file is loaded.
 * @snippet_packet_loaded_id: The id returned by #snippets_db_load_file
 *
 * Saves the file loaded in the #SnippetsDB
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_save_file (SnippetsDB* snippets_db,
                       gint32 snippet_packet_loaded_id)
{

	/* TODO */
	return FALSE;
}

/**
 * snippets_db_save_all:
 * @snippets_db: A #SnippestDB object.
 * 
 * Saves all the files loaded in the #SnippetsDB.
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_save_all (SnippetsDB* snippets_db)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_search:
 * @snippets_db: A #SnippetsDB object
 * @search_string: A string with the searched content.
 * @snippet_language: The language for which the searching is requested. If NULL, it will search all languages.
 * @maximum_results: The maximum results that are to be returned.
 *
 * Searches the #SnippetsDB from the @search_string given. 
 *
 * Returns: A #GList with #Snippet entries sorted by relevance.
 **/
GList*	
snippets_db_search (SnippetsDB* snippets_db,
                    const gchar* search_string,
                    const gchar* snippet_language,
                    guint16 maximum_results)
{
	/* TODO */
	return NULL;
}

/**
 * snippets_db_add_snippet:
 * @snippets_db: A #SnippetsDB object
 * @added_snippet: A #Snippet object
 * @overwrite: If a snippet with the same key exists, it will overwrite it.
 * 
 * Adds the @added_snippet to the @snippets_db. With overwrite set to TRUE, this method can be used
 * for updating a snippet.
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_add_snippet (SnippetsDB* snippets_db,
                         AnjutaSnippet* added_snippet,
                         gboolean overwrite)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_get_snippet:
 * @snippets_db: A #SnippetsDB object
 * @snippets_key: The snippet-key of the requested snippet
 *
 * Gets a snippet from the database for the given key.
 *
 * Returns: The requested snippet (not a copy, should not be freed) or NULL if not found.
 **/
AnjutaSnippet*	
snippets_db_get_snippet (SnippetsDB* snippets_db,
                         const gchar* snippet_key)
{
	/* TODO */
	return NULL;
}

/**
 * snippets_db_remove_snippet:
 * @snippets_db: A #SnippetsDB object.
 * @snippet_key: The snippet-key of the snippet for which a removal is requested.
 *
 * Removes a snippet from the #SnippetDB.
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_remove_snippet (SnippetsDB* snippets_db,
                            const gchar* snippet_key)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_add_snippet_group:
 * @snippets_db: A #SnippetsDB object
 * @group_name: The name of the added group.
 * @group_description: A short description of the added group.
 *
 * Adds an empty Snippet Group to the #SnippetsDB
 *
 * Returns: TRUE on success
 **/
gboolean	
snippets_db_add_snippet_group (SnippetsDB* snippets_db,
                               const gchar* group_name,
                               const gchar* group_description)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_remove_snippet_group:
 * @snippets_db: A #SnippetsDB object.
 * @group_name: The name of the group to be removed.
 * 
 * Removes a snippet group (and it's containing snippets) from the #SnippetsDB
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_remove_snippet_group (SnippetsDB* snippets_db,
                                  const gchar* group_name)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_get_tree_model:
 * @snippets_db: A #SnippetsDB object.
 * @snippets_language: The language for which the GtkTreeModel is requested or NULL for all languages.
 *
 * A #GtkTreeModel to be used for the Snippet Browser.
 *
 * Returns: The requested #GtkTreeModel or NULL on failure.
 **/
GtkTreeModel*	
snippets_db_get_tree_model (SnippetsDB* snippets_db,
                            const gchar* snippets_language)
{
	/* TODO */
	return NULL;
}

/**
 * snippets_db_global_variable_get:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: The name of the global variable.
 *
 * Gets the value of a global variable. A global variable value can be static, or the output of a 
 * command.
 *
 * Returns: The value of the global variable, or NULL if the variable wasn't found.
 */
gchar* snippets_db_global_variable_get (SnippetsDB* snippets_db,
                                        const gchar* variable_name)
{
	/* TODO */
	return NULL;
}
