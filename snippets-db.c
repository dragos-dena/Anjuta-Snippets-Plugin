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
#include <glib.h>
#include <glib/gstdio.h>

#define DEFAULT_GLOBAL_VARS_FILE           "default-snippets-global-variables.xml"
#define DEFAULT_SNIPPETS_FILE              "default-snippets.xml"
#define DEFAULT_SNIPPETS_FILE_INSTALL_PATH PACKAGE_DATA_DIR"/"DEFAULT_SNIPPETS_FILE
#define DEFAULT_GLOBAL_VARS_INSTALL_PATH   PACKAGE_DATA_DIR"/"DEFAULT_GLOBAL_VARS_FILE
#define USER_SNIPPETS_DB_DIR               "snippets-database"
#define USER_SNIPPETS_DIR                  "snippets-database/snippet-packages"

#define ANJUTA_SNIPPETS_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_DB, SnippetsDBPrivate))

struct _SnippetsDBPrivate
{
	/* A tree model to be used by the Snippet Browser. */
	GtkTreeStore* snippets_tree;
	
	/* A binary balanced tree with the trigger keys for searching purposes. */
	GTree* trigger_keys_tree;
	
	/* A binary balanced tree with the keywords for searching purposes */
	GTree* keywords_tree;

	/* A hashtable with group names as keys and FormatType's as values */
	GHashTable* snippets_group_format_map;
	
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

	/* A function that computes the value for this variable. If this is not NULL,
	   it will be the preffered way to get the output. These global variables will
	   be loaded internally and can't be edited. */
	gchar* (*output_function) (SnippetsDB* snippets_db);
} SnippetsGlobalVariable;

G_DEFINE_TYPE (SnippetsDB, snippets_db, G_TYPE_OBJECT);

static void
snippets_db_dispose (GObject* snippets_db)
{
	DEBUG_PRINT ("%s", "Disposing SnippetsDB");
	/* TODO Save the snippets */
	G_OBJECT_CLASS (snippets_db_parent_class)->dispose (snippets_db);
}

static void
snippets_db_finalize (GObject* snippets_db)
{
	DEBUG_PRINT ("%s", "Finalizing SnippetsDB");
	
	G_OBJECT_CLASS (snippets_db_parent_class)->finalize (snippets_db);
}

static void
snippets_db_class_init (SnippetsDBClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippets_db_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippets_db_dispose;
	object_class->finalize = snippets_db_finalize;
}

static void
snippets_db_init (SnippetsDB * snippets_db)
{
	SnippetsDBPrivate *priv = g_new0 (SnippetsDBPrivate, 1);
	
	snippets_db->priv = priv;
	
}

/**
 * snippets_db_new:
 * @anjuta_shell: A #AnjutaShell object.
 *
 * A new #SnippetDB with snippets loaded from the default folder.
 *
 * Returns: A new #SnippetsDB object.
 **/
SnippetsDB*	
snippets_db_new (AnjutaShell* anjuta_shell)
{
	SnippetsDB* snippets_db = ANJUTA_SNIPPETS_DB (g_object_new (snippets_db_get_type (), NULL));
	AnjutaSnippetsGroup* cur_snippet_group = NULL;
	gboolean user_defaults_file_exists = FALSE, user_global_vars_exists = FALSE;
	gchar *user_snippets_dir_path = NULL, *user_snippets_default = NULL, 
	      *user_snippets_db_dir_path = NULL, *user_global_vars = NULL,
	      *cur_file_path = NULL;
	const gchar *cur_file_name = NULL;
	GFile *installation_defaults_file = NULL, *user_defaults_file = NULL,
	      *installation_global_vars_file = NULL, *user_global_vars_file = NULL;
	GDir *user_snippets_dir = NULL;

	/* Check if there is a "snippets-database/snippets" directory */
	user_snippets_dir_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DIR, "/", NULL);
	g_mkdir_with_parents (user_snippets_dir_path, 0755);

	/* Check if the default snippets file is in the user directory */
	user_snippets_default = g_strconcat (user_snippets_dir_path, "/", DEFAULT_SNIPPETS_FILE, NULL);
	user_defaults_file_exists = g_file_test (user_snippets_default, G_FILE_TEST_EXISTS);

	/* If it's not in the user directory, copy from the installation path the default snippets */
	if (!user_defaults_file_exists)
	{
		installation_defaults_file = g_file_new_for_path (DEFAULT_SNIPPETS_FILE_INSTALL_PATH);
		user_defaults_file = g_file_new_for_path (user_snippets_default);

		g_file_copy (installation_defaults_file,
		             user_defaults_file,
		             G_FILE_COPY_NONE,
		             NULL, NULL, NULL, NULL);
	}
	g_free (user_snippets_default);
	
	/* Check if there is the default-snippets-global-variables.xml file in the user directory */
	user_snippets_db_dir_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", NULL);
	user_global_vars = g_strconcat (user_snippets_db_dir_path, "/", DEFAULT_GLOBAL_VARS_FILE, NULL);
	user_global_vars_exists = g_file_test (user_global_vars, G_FILE_TEST_EXISTS);

	/* If it's not in the user directory, copy from the installation path the default global 
	   variables file */
	if (!user_global_vars_exists)
	{
		installation_global_vars_file = g_file_new_for_path (DEFAULT_GLOBAL_VARS_INSTALL_PATH);
		user_global_vars_file = g_file_new_for_path (user_global_vars);

		g_file_copy (installation_global_vars_file,
		             user_global_vars_file,
		             G_FILE_COPY_NONE,
		             NULL, NULL, NULL, NULL);
	}

	/* Parse the global variables file */
	snippets_manager_parse_variables_xml_file (user_global_vars, snippets_db);
	g_free (user_global_vars);
	g_free (user_snippets_db_dir_path);

	/* Parse all the files in the user snippet-packages directory*/
	user_snippets_dir = g_dir_open (user_snippets_dir_path, 0, NULL);
	cur_file_name = g_dir_read_name (user_snippets_dir);
	while (cur_file_name)
	{
		cur_file_path = g_strconcat (user_snippets_dir_path, "/", cur_file_name, NULL);

		/* Parse the current file and make a new SnippetGroup object */
		cur_snippet_group = snippets_manager_parse_snippets_xml_file (cur_file_path,
		                                                              NATIVE_FORMAT);

		/* TODO - actually add the snippet group to the database */
		
		g_free (cur_file_path);
		cur_file_name = g_dir_read_name (user_snippets_dir);
	}
	g_dir_close (user_snippets_dir);
	g_free (user_snippets_dir_path);

	
	/* TODO */
	
	return snippets_db;
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
gchar* 
snippets_db_get_global_variable (SnippetsDB* snippets_db,
                                 const gchar* variable_name)
{
	/* TODO Here I should compute instant variables like the filename */
	return NULL;
}

/**
 * snippets_db_has_global_variable:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: A variable name.
 *
 * Checks if the Snippet Database has an entry with a variable name as requested.
 *
 * Returns: TRUE if the global variable exists.
 */
gboolean 
snippets_db_has_global_variable (SnippetsDB* snippets_db,
                                 const gchar* variable_name)
{

	return FALSE;
}

/**
 * snippets_db_has_global_variable:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: A variable name.
 * @variable_value: The global variable value.
 * @variable_is_shell_command: If the variable is the output of a shell command.
 *
 * Adds a global variable to the Snippets Database.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_add_global_variable (SnippetsDB* snippets_db,
                                 const gchar* variable_name,
                                 const gchar* variable_value,
                                 gboolean variable_is_shell_command)
{

	return FALSE;
}
