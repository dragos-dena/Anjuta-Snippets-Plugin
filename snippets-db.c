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
#include <gtk/gtk.h>

#define DEFAULT_GLOBAL_VARS_FILE           "snippets-global-variables.xml"
#define DEFAULT_SNIPPETS_FILE              "default-snippets.xml"
#define DEFAULT_SNIPPETS_FILE_INSTALL_PATH PACKAGE_DATA_DIR"/"DEFAULT_SNIPPETS_FILE
#define DEFAULT_GLOBAL_VARS_INSTALL_PATH   PACKAGE_DATA_DIR"/"DEFAULT_GLOBAL_VARS_FILE
#define USER_SNIPPETS_DB_DIR               "snippets-database"
#define USER_SNIPPETS_DIR                  "snippets-database/snippet-packages"

#define ANJUTA_SNIPPETS_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_DB, SnippetsDBPrivate))


/**
 * SnippetsDBPrivate:
 * @snippets_groups: A #GList where the #AnjutaSnippetsGroup objects are loaded.
 * @unsaved_snippets_group: A #GList where the unsaved #AnjutaSnippetsGroup objects are tracked.
 *                          Important: One should not try to destroy the objects in this group,
 *                          as they are saved in @snippets_groups. This is just for tracking 
 *                          purposes.
 * @trigger_keys_tree: A #GTree with the keys being a string representing the trigger-key and
 *                     the values being a #GList with #AnjutaSnippet as entries for the list.
 *                     Important: One should not try to delete anything. The #GTree was constructed
 *                     with destroy functions passed to the #GTree that will free the memory.
 * @keywords_tree: A #GTree with the keys being a string representing the keyword and the values
 *                 being a #GList with #AnjutaSnippet as entries for the list.
 *                 Important: One should not try to delete anything. The #GTree was constructed
 *                 with destroy functions passed to the #GTree that will free the memory.
 * @snippets_group_format_map: A #GHashTable with strings representing the group name as keys and
 *                             #FormatType entries as values.
 *                             Important: One should not try to delete anything. The #GHashTable was 
 *                             constructed with destroy functions passed to the #GHashTable that will 
 *                             free the memory.
 * @snippet_keys_map: A #GHashTable with strings representing the snippet-key as keys and pointers
 *                    to #AnjutaSnippet objects as values.
 *                    Important: One should not try to delete anything. The #GHashTable was 
 *                    constructed with destroy functions passed to the #GHashTable that will 
 *                    free the memory.
 * @global_variables: A #GtkListStore where the static and command-based global variables are stored.
 *                    See snippets-db.h for details about columns.
 *                    Important: Only static and command-based global variables are stored here!
 *                    The internal global variables are computed when #snippets_db_get_global_variable
 *                    is called.
 *
 * The private field for the SnippetsDB object.
 */
struct _SnippetsDBPrivate
{	
	GList* snippets_groups;
	GList* unsaved_snippets_groups;
	
	GTree* trigger_keys_tree;
	GTree* keywords_tree;

	GHashTable* snippets_group_format_map;
	GHashTable* snippet_keys_map;
	
	GtkListStore* global_variables;
};


/* GObject methods declaration */
static void              snippets_db_dispose         (GObject* snippets_db);

static void              snippets_db_finalize        (GObject* snippets_db);

static void              snippets_db_class_init      (SnippetsDBClass* klass);

static void              snippets_db_init            (SnippetsDB *snippets_db);


/* GtkTreeModel methods declaration */
static void              snippets_db_tree_model_init (GtkTreeModelIface *iface);

static GtkTreeModelFlags snippets_db_get_flags       (GtkTreeModel *tree_model);

static gint              snippets_db_get_n_columns   (GtkTreeModel *tree_model);

static GType             snippets_db_get_column_type (GtkTreeModel *tree_model,
                                                      gint index);

static gboolean          snippets_db_get_iter        (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreePath *path);

static GtkTreePath*      snippets_db_get_path        (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter);

static void              snippets_db_get_value       (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      gint column,
                                                      GValue *value);

static gboolean          snippets_db_iter_next       (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter);

static gboolean          snippets_db_iter_children   (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreeIter *parent);

static gboolean          snippets_db_iter_has_child  (GtkTreeModel *tree_model,
                                                      GtkTreeIter  *iter);

static gint              snippets_db_iter_n_children (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter);

static gboolean          snippets_db_iter_nth_child  (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreeIter *parent,
                                                      gint n);

static gboolean          snippets_db_iter_parent     (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreeIter *child);


static GObjectClass *snippets_db_parent_class = NULL;

GType
snippets_db_get_type (void)
{
  static GType snippets_db_type = 0;

	if (snippets_db_type == 0)
	{
		static const GTypeInfo snippets_db_info =
		{
			sizeof (SnippetsDBClass),
			NULL,                                         /* base_init */
			NULL,                                         /* base_finalize */
			(GClassInitFunc) snippets_db_class_init,
			NULL,                                         /* class finalize */
			NULL,                                         /* class_data */
			sizeof (SnippetsDB),
			0,                                            /* n_preallocs */
			(GInstanceInitFunc) snippets_db_init
		};
		static const GInterfaceInfo tree_model_info =
		{
			(GInterfaceInitFunc) snippets_db_tree_model_init,
			NULL,
			NULL
		};

		snippets_db_type = g_type_register_static (G_TYPE_OBJECT, "SnippetsDB",
		                                           &snippets_db_info, (GTypeFlags)0);

		g_type_add_interface_static (snippets_db_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
	}

	return snippets_db_type;
}

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

	g_type_class_add_private (klass, sizeof (SnippetsDBPrivate));
}

static void
snippets_db_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags       = snippets_db_get_flags;
	iface->get_n_columns   = snippets_db_get_n_columns;
	iface->get_column_type = snippets_db_get_column_type;
	iface->get_iter        = snippets_db_get_iter;
	iface->get_path        = snippets_db_get_path;
	iface->get_value       = snippets_db_get_value;
	iface->iter_next       = snippets_db_iter_next;
	iface->iter_children   = snippets_db_iter_children;
	iface->iter_has_child  = snippets_db_iter_has_child;
	iface->iter_n_children = snippets_db_iter_n_children;
	iface->iter_nth_child  = snippets_db_iter_nth_child;
	iface->iter_parent     = snippets_db_iter_parent;
}

static gint
trigger_keys_tree_compare_func (gconstpointer a,
                                gconstpointer b,
                                gpointer user_data)
{
	gchar* trigger_key1 = (gchar *)a;
	gchar* trigger_key2 = (gchar *)b;

	return g_ascii_strcasecmp (trigger_key1, trigger_key2);
}

static gint
keywords_tree_compare_func (gconstpointer a,
                            gconstpointer b,
                            gpointer user_data)
{
	gchar* keyword1 = (gchar *)a;
	gchar* keyword2 = (gchar *)b;

	return g_ascii_strcasecmp (keyword1, keyword2);
}

static void
snippets_db_init (SnippetsDB *snippets_db)
{
	SnippetsDBPrivate* priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);
	
	snippets_db->priv = priv;
	snippets_db->anjuta_shell = NULL;
	snippets_db->stamp = g_random_int ();

	/* Initialize the private fields */
	snippets_db->priv->snippets_groups = NULL;
	snippets_db->priv->unsaved_snippets_groups = NULL;
	snippets_db->priv->trigger_keys_tree = g_tree_new_full (trigger_keys_tree_compare_func,
	                                                        NULL,
	                                                        g_free,
	                                                        (GDestroyNotify)g_list_free);
	snippets_db->priv->keywords_tree = g_tree_new_full (keywords_tree_compare_func,
	                                                    NULL,
	                                                    g_free,
	                                                    (GDestroyNotify)g_list_free);
	snippets_db->priv->snippets_group_format_map = g_hash_table_new_full (g_str_hash, 
	                                                                      g_str_equal, 
	                                                                      g_free, 
	                                                                      NULL);
	snippets_db->priv->snippet_keys_map = g_hash_table_new_full (g_str_hash, 
	                                                             g_str_equal, 
	                                                             g_free, 
	                                                             NULL);
	snippets_db->priv->global_variables = gtk_list_store_new (GLOBAL_VARS_MODEL_COL_N,
	                                                          G_TYPE_STRING,
	                                                          G_TYPE_STRING,
	                                                          G_TYPE_BOOLEAN);
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
		snippets_db_load_file (snippets_db, cur_file_path, TRUE, TRUE, NATIVE_FORMAT);

		g_free (cur_file_path);
		cur_file_name = g_dir_read_name (user_snippets_dir);
	}
	g_dir_close (user_snippets_dir);
	g_free (user_snippets_dir_path);

	return snippets_db;
}

/**
 * snippets_db_load_file:
 * @snippets_db: A #SnippetsDB object where the file should be loaded.
 * @snippet_packet_file_path: A string with the file path.
 * @overwrite_group: If a #AnjutaSnippetsGroup with the same name exists, it will be overwriten.
 * @overwrite_snippets: If conflicting snippets are found, the old ones will be overwriten.
 * @format_type: A #FormatType with the type of the snippet-packet XML file.
 *
 * Loads the given file in the #SnippetsDB 
 *
 * Returns: TRUE on success.
 **/
gboolean
snippets_db_load_file (SnippetsDB* snippets_db,
                       const gchar* snippet_packet_file_path,
                       gboolean overwrite_group,
                       gboolean overwrite_snippets,
                       FormatType format_type)
{
	AnjutaSnippetsGroup *snippets_group = NULL;
	gboolean success = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL &&
	                      ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippet_packet_file_path != NULL,
	                      FALSE);

	/* Get the AnjutaSnippetsGroup object from the file */
	snippets_group = snippets_manager_parse_snippets_xml_file (snippet_packet_file_path,
	                                                           format_type);
	g_return_val_if_fail (snippets_group != NULL && 
	                      ANJUTA_IS_SNIPPETS_GROUP (snippets_group), 
	                      FALSE);

	/* Add the new AnjutaSnippetsGroup to the database */
	success = snippets_db_add_snippets_group (snippets_db, 
	                                          snippets_group,
	                                          overwrite_group,
	                                          overwrite_snippets);
	
	return success;
}

/**
 * snippets_db_save_file:
 * @snippets_db: A #SnippetsDB object where the file is loaded.
 * @snippet_group_name: The #AnjutaSnippetsGroup name for the group that should be saved.
 * @snippet_packet_file_path: The name of the file where the #AnjutaSnippetsGroup should
 *                            be saved. Can be NULL (read bellow).
 *
 * Saves the #AnjutaSnippetsGroup in a snippet-packet XML file. If snippet_packet_file_path
 * is not NULL, then it will be saved in the user directory where all snippet-packet files
 * are saved. If the #AnjutaSnippetsGroup was created on runtime and doesn't have a file name
 * the file name will be computed based on the #AnjutaSnippetsGroup first 50 characters. 
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_save_file (SnippetsDB* snippets_db,
                       const gchar* snippets_group_name,
                       const gchar* snippet_packet_file_path)
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
 * @group_name: The name of the group where the snippet should be added.
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
                         const gchar* group_name,
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
const AnjutaSnippet*	
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
 * snippets_db_add_snippets_group:
 * @snippets_db: A #SnippetsDB object
 * @snippets_group: A #AnjutaSnippetsGroup object
 * @overwrite_group: If a #AnjutaSnippetsGroup with the same name exists it will 
 *                   be overwriten.
 * @overwrite_snippets: If there will be conflicting snippets, they will be overwriten.
 *
 * Adds an #AnjutaSnippetsGroup to the database, checking for conflicts.
 *
 * Returns: TRUE on success
 **/
gboolean	
snippets_db_add_snippets_group (SnippetsDB* snippets_db,
                                AnjutaSnippetsGroup* snippets_group,
                                gboolean overwrite_group,
                                gboolean overwrite_snippets)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_remove_snippets_group:
 * @snippets_db: A #SnippetsDB object.
 * @group_name: The name of the group to be removed.
 * 
 * Removes a snippet group (and it's containing snippets) from the #SnippetsDB
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_remove_snippets_group (SnippetsDB* snippets_db,
                                   const gchar* group_name)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_get_snippets_group:
 * @snippets_db: A #SnippetsDB object.
 * @group_name: The name of the #AnjutaSnippetsGroup requested object.
 *
 * Returns: The requested #AnjutaSnippetsGroup object or NULL on failure.
 */
const AnjutaSnippetsGroup*
snippets_db_get_snippets_group (SnippetsDB* snippets_db,
                                const gchar* group_name)
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
	/* Debugging TODO - delete this when stable */
	/*printf ("\n+++ Debugging Global Variable +++\n");
	printf ("Global Variable Name: %s\n", variable_name);
	printf ("Global Variable Is Command (1 for true): %d\n", variable_is_shell_command);
	printf ("Global Variable Value: %s\n", variable_value);
	*//* Debugging end */
	
	return FALSE;
}

/**
 * snippets_db_remove_global_variable:
 * @snippets_db: A #SnippetsDB object
 * @group_name: The name of the global variable to be removed.
 *
 * Removes a global variable from the database. Only works for static or command
 * based variables.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_remove_global_variable (SnippetsDB* snippets_db, 
                                    const gchar* variable_name)
{
	/* TODO */
	return FALSE;
}

/**
 * snippets_db_get_global_vars_model:
 * snippets_db: A #SnippetsDB object.
 *
 * The returned GtkTreeModel is basically a list with the global variables that
 * should be used for displaying the variables. At the moment, it's used for
 * displaying the variables in the preferences.
 *
 * Returns: The #GtkTreeModel of the global variables list.
 */
GtkTreeModel*
snippets_db_get_global_vars_model (SnippetsDB* snippets_db);

/* GtkTreeModel methods definition */
static GtkTreeModelFlags 
snippets_db_get_flags (GtkTreeModel *tree_model)
{
	return 0;
}

static gint
snippets_db_get_n_columns (GtkTreeModel *tree_model)
{
	return -1;
}

static GType
snippets_db_get_column_type (GtkTreeModel *tree_model,
                             gint index)
{
	return G_TYPE_NONE;
}

static gboolean
snippets_db_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter *iter,
                      GtkTreePath *path)
{
	return FALSE;
}

static GtkTreePath*
snippets_db_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter *iter)
{
	return NULL;
}

static void
snippets_db_get_value (GtkTreeModel *tree_model,
                       GtkTreeIter *iter,
                       gint column,
                       GValue *value)
{

}

static gboolean
snippets_db_iter_next (GtkTreeModel *tree_model,
                       GtkTreeIter *iter)
{
	return FALSE;
}

static gboolean
snippets_db_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter *iter,
                           GtkTreeIter *parent)
{
	return FALSE;
}

static gboolean
snippets_db_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
	return FALSE;
}

static gint
snippets_db_iter_n_children (GtkTreeModel *tree_model,
                             GtkTreeIter *iter)
{
	return -1;
}

static gboolean
snippets_db_iter_nth_child (GtkTreeModel *tree_model,
                            GtkTreeIter *iter,
                            GtkTreeIter *parent,
                            gint n)
{
	return FALSE;
}

static gboolean
snippets_db_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter *iter,
                         GtkTreeIter *child)
{
	return FALSE;
}
