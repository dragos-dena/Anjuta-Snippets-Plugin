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
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#define DEFAULT_GLOBAL_VARS_FILE           "snippets-global-variables.xml"
#define DEFAULT_SNIPPETS_FILE              "default-snippets.xml"
#define DEFAULT_SNIPPETS_FILE_INSTALL_PATH PACKAGE_DATA_DIR"/"DEFAULT_SNIPPETS_FILE
#define DEFAULT_GLOBAL_VARS_INSTALL_PATH   PACKAGE_DATA_DIR"/"DEFAULT_GLOBAL_VARS_FILE
#define USER_SNIPPETS_DB_DIR               "snippets-database"
#define USER_SNIPPETS_DIR                  "snippets-database/snippet-packages"

/* Internal global variables */
#define GLOBAL_VAR_FILE_NAME       "filename"
#define GLOBAL_VAR_USER_NAME       "username"
#define GLOBAL_VAR_USER_FULL_NAME  "userfullname"
#define GLOBAL_VAR_HOST_NAME       "hostname"


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


G_DEFINE_TYPE_WITH_CODE (SnippetsDB, snippets_db, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                snippets_db_tree_model_init));

/* SnippetsDB private methods */

static void
snippets_db_load_internal_global_variables (SnippetsDB *snippets_db)
{
	GtkTreeIter iter_added;
	GtkListStore *global_vars_store = NULL;

	/* Assertions */
	g_return_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                  snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                  GTK_IS_LIST_STORE (snippets_db->priv->global_variables));
	global_vars_store = snippets_db->priv->global_variables;

	/* Add the filename global variable */
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_FILE_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);

	/* Add the username global variable */
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_USER_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);

	/* Add the userfullname global variable */
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_USER_FULL_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);

	/* Add the hostname global variable*/
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_HOST_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);
}

static gchar*
get_internal_global_variable_value (AnjutaShell *shell,
                                    const gchar* variable_name)
{
	gchar* value = NULL;

	/* Assertions */
	g_return_val_if_fail (variable_name != NULL, NULL);

	/* Check manually what variable is requested */
	if (!g_strcmp0 (variable_name, GLOBAL_VAR_FILE_NAME))
	{
		IAnjutaDocumentManager *anjuta_docman = NULL;
		IAnjutaDocument *anjuta_cur_doc = NULL;

		anjuta_docman = anjuta_shell_get_interface (shell,
		                                            IAnjutaDocumentManager,
		                                            NULL);
		if (anjuta_docman)
		{
			anjuta_cur_doc = ianjuta_document_manager_get_current_document (anjuta_docman, NULL);
			if (!anjuta_cur_doc)
				return NULL;
			value = g_strdup (ianjuta_document_get_filename (anjuta_cur_doc, NULL));
			return value;
		}
		else
			return NULL;
	}
	if (!g_strcmp0 (variable_name, GLOBAL_VAR_USER_NAME))
	{
		value = g_strdup (g_get_user_name ());
		return value;
	}
	if (!g_strcmp0 (variable_name, GLOBAL_VAR_USER_FULL_NAME))
	{
		value = g_strdup (g_get_real_name ());
		return value;
	}
	if (!g_strcmp0 (variable_name, GLOBAL_VAR_HOST_NAME))
	{
		value = g_strdup (g_get_host_name ());
		return value;
	}
	
	return NULL;
}

static GtkTreeIter*
get_iter_at_global_variable_name (GtkListStore *global_vars_store,
                                  const gchar *variable_name)
{
	GtkTreeIter iter;
	gchar *stored_name = NULL;
	gboolean iter_is_set = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (GTK_IS_LIST_STORE (global_vars_store),
	                      NULL);

	iter_is_set = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (global_vars_store), &iter);
	while (iter_is_set) 
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), &iter,
		                    GLOBAL_VARS_MODEL_COL_NAME, &stored_name, 
		                    -1);

		/* If we found the name in the database */
		if (!g_strcmp0 (stored_name, variable_name) )
		{
			g_free (stored_name);
			return gtk_tree_iter_copy (&iter);
		}

		iter_is_set = gtk_tree_model_iter_next (GTK_TREE_MODEL (global_vars_store), &iter);
		g_free (stored_name);
	}
	
	return NULL;
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

static gint
compare_snippets_groups_by_name (gconstpointer a,
                                 gconstpointer b)
{
	AnjutaSnippetsGroup *group1 = (AnjutaSnippetsGroup *)a;
	AnjutaSnippetsGroup *group2 = (AnjutaSnippetsGroup *)b;

	/* Assertions */
	g_return_val_if_fail (group1 != NULL && ANJUTA_IS_SNIPPETS_GROUP (group1) &&
	                      group2 != NULL && ANJUTA_IS_SNIPPETS_GROUP (group2),
	                      0);

	return g_strcmp0 (snippets_group_get_name (group1),
	                  snippets_group_get_name (group2));
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
	snippets_db->priv->snippet_keys_map = g_hash_table_new_full (g_str_hash, 
	                                                             g_str_equal, 
	                                                             g_free, 
	                                                             NULL);
	snippets_db->priv->global_variables = gtk_list_store_new (GLOBAL_VARS_MODEL_COL_N,
	                                                          G_TYPE_STRING,
	                                                          G_TYPE_STRING,
	                                                          G_TYPE_BOOLEAN,
	                                                          G_TYPE_BOOLEAN);
}

/* SnippetsDB public methods */

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

	/* Load the internal global variables */
	snippets_db_load_internal_global_variables (snippets_db);
	
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
 * Loads the given file in the #SnippetsDB. 
 * 
 * If the snippet_packet_file_path doesn't point to a file in the user folder then it will be 
 * considered as unsaved and the file_path property of the #AnjutaSnippetsGroup will be set to the given
 * @snippet_packet_file_path. On calling snippets_db_save_file () with snippet_packet_file_path
 * set to NULL for the resulting group it will first try to give the same file name as the 
 * one indicated in the @snippet_packet_file_path and if a conflict occurs, it will add a 
 * sufix to it.
 *
 * If the @format_type is not #NATIVE_FORMAT, the #AnjutaSnippetsGroup will be marked
 * as unsaved, and it will follow the above procedure.
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
	gchar *snippet_packet_file_path_dir = NULL, *user_snippets_folder_dir = NULL;
	gboolean file_path_in_user_folder = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL &&
	                      ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippet_packet_file_path != NULL,
	                      FALSE);

	/* Get the AnjutaSnippetsGroup object from the file */
	snippets_group = snippets_manager_parse_snippets_xml_file (snippet_packet_file_path,
	                                                           format_type);
	g_return_val_if_fail (snippets_group != NULL && ANJUTA_IS_SNIPPETS_GROUP (snippets_group), 
	                      FALSE);

	/* Add the new AnjutaSnippetsGroup to the database */
	success = snippets_db_add_snippets_group (snippets_db, 
	                                          snippets_group,
	                                          overwrite_group,
	                                          overwrite_snippets);
	g_object_unref (snippets_group);

	/* If success, we check to see if the new AnjutaSnippetsGroup should be marked as unsaved */
	snippet_packet_file_path_dir = g_path_get_dirname (snippet_packet_file_path);
	user_snippets_folder_dir = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DIR, "/", NULL);
	file_path_in_user_folder = !g_ascii_strncasecmp (snippet_packet_file_path_dir, 
	                                                 user_snippets_folder_dir, 
	                                                 sizeof (snippet_packet_file_path_dir));
	if (success)
	{
		/* If it's not the native format or it's not in the user snippets folder we will
		   consider it unsaved. */
		if (format_type != NATIVE_FORMAT || !file_path_in_user_folder)
			snippets_db->priv->unsaved_snippets_groups = g_list_prepend (snippets_db->priv->unsaved_snippets_groups, 
			                                                             snippets_group);

		snippets_group->file_path = g_strdup (snippet_packet_file_path);
	}
	g_free (snippet_packet_file_path_dir);
	g_free (user_snippets_folder_dir);

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
 * The last case may also appear if the user loaded a snippet packet from other format
 * than the native one.
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
 * @snippet_language: The language for which the searching is requested. If NULL, 
 *                    it will search all languages.
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

static void
add_snippet_to_searching_trees (SnippetsDB *snippets_db,
                                AnjutaSnippet *snippet)
{
	/* TODO */
}

static void
remove_snippet_from_searching_trees (SnippetsDB *snippets_db,
                                     AnjutaSnippet *snippet)
{
	/* TODO */
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
	GList *iter = NULL;
	AnjutaSnippetsGroup *cur_snippets_group = NULL;
	const gchar *cur_snippets_group_name = NULL;
	gboolean added_to_group = FALSE;
		
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      added_snippet != NULL && ANJUTA_IS_SNIPPET (added_snippet),
	                      FALSE);

	/* Lookup the AnjutaSnippetsGroup with the given group_name */
	for (iter = g_list_first (snippets_db->priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippets_group = (AnjutaSnippetsGroup *)iter->data;
		cur_snippets_group_name = snippets_group_get_name (cur_snippets_group);

		/* We found the group */
		if (!g_strcmp0 (cur_snippets_group_name, group_name))
		{
			/* Try to add the snippet to the group */
			added_to_group = snippets_group_add_snippet (cur_snippets_group, added_snippet, overwrite);

			/* If it was succesfully added, update the internal structures */
			if (added_to_group)
			{
				/* Add to the two GTree's */
				add_snippet_to_searching_trees (snippets_db, added_snippet);

				/* Add to the Hashtable */
				g_hash_table_insert (snippets_db->priv->snippet_keys_map,
				                     snippet_get_key (added_snippet),
				                     added_snippet);				
			}

			return added_to_group;
		}
	}
	
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
	AnjutaSnippet *snippet = NULL;

	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db),
	                      NULL);

	/* Look up the the snippet in the hashtable */
	snippet = g_hash_table_lookup (snippets_db->priv->snippet_keys_map, snippet_key);

	return snippet;
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
	AnjutaSnippet *deleted_snippet = NULL;
	AnjutaSnippetsGroup *deleted_snippet_group = NULL;
		
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db),
	                      FALSE);

	/* Get the snippet to be deleted */
	deleted_snippet = g_hash_table_lookup (snippets_db->priv->snippet_keys_map,
	                                       snippet_key);
	g_return_val_if_fail (deleted_snippet != NULL,
	                      FALSE);

	/* Remove it from the hashtable */
	g_hash_table_remove (snippets_db->priv->snippet_keys_map,
	                     snippet_key);

	/* Remove it from the searching tree's */
	remove_snippet_from_searching_trees (snippets_db, deleted_snippet);

	/* Remove it from the snippets group */
	deleted_snippet_group = ANJUTA_SNIPPETS_GROUP (deleted_snippet->parent_snippets_group);
	g_return_val_if_fail (deleted_snippet_group != NULL &&
	                      ANJUTA_IS_SNIPPETS_GROUP (deleted_snippet_group),
	                      FALSE);
	snippets_group_remove_snippet (deleted_snippet_group, snippet_key);

	return TRUE;
}

/**
 * snippets_db_add_snippets_group:
 * @snippets_db: A #SnippetsDB object
 * @snippets_group: A #AnjutaSnippetsGroup object
 * @overwrite_group: If a #AnjutaSnippetsGroup with the same name exists it will 
 *                   be overwriten.
 * @overwrite_snippets: If there will be conflicting snippets, they will be overwriten.
 *
 * Adds an #AnjutaSnippetsGroup to the database, checking for conflicts. The @snippets_group
 * passed as argument will have it's reference increased by one. If it isn't needed anymore
 * it should be unrefed after calling this function.
 *
 * Returns: TRUE on success
 **/
gboolean	
snippets_db_add_snippets_group (SnippetsDB* snippets_db,
                                AnjutaSnippetsGroup* snippets_group,
                                gboolean overwrite_group,
                                gboolean overwrite_snippets)
{
	AnjutaSnippetsGroup *cur_group = NULL;
	AnjutaSnippet *cur_snippet = NULL, *conflicting_snippet = NULL;
	GList *iter = NULL, *replaced_node = NULL;
	const GList *added_snippets = NULL, *c_iter = NULL;
	const gchar *cur_group_name = NULL;
	AnjutaSnippetsGroup *replaced_group = NULL;
	SnippetsDBPrivate *priv = NULL;
		
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_group != NULL && ANJUTA_IS_SNIPPETS_GROUP (snippets_group),
	                      FALSE);

	/* So lines won't get too big :-) */
	priv = snippets_db->priv;

	/* Look if there is a group with the same name */
	cur_group_name = snippets_group_get_name (snippets_group);
	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		cur_group = (AnjutaSnippetsGroup *)iter->data;
		cur_group_name = snippets_group_get_name (cur_group);
		if (!g_strcmp0 (cur_group_name, snippets_group_get_name (snippets_group)))
		{
			replaced_group = cur_group;
			replaced_node = iter;
			break;
		} 
	}

	/* If we found a group with the same name */
	if (replaced_group)
	{
		/* If we should replace the old group */
		if (overwrite_group)
		{
			/* Delete the old group */
			g_object_unref (cur_group);

			/* Make the node point to the new group */
			replaced_node->data = snippets_group;
		}
		else
		{
			g_return_val_if_reached (FALSE);
		}
	}
	/* If we didn't found a group with the same name */
	else
	{
		/* Add the snippets_group to the database keeping sorted the list by the
		   group name. */
		   priv->snippets_groups = g_list_insert_sorted (priv->snippets_groups,
		                                                 snippets_group,
		                                                 compare_snippets_groups_by_name);
	}

	/* Check for conflicting snippets */
	added_snippets = snippets_group_get_snippets_list (snippets_group);
	for (c_iter = g_list_first ((GList *)added_snippets); c_iter != NULL; c_iter = g_list_next (c_iter))
	{
		conflicting_snippet = NULL;
		cur_snippet = (AnjutaSnippet *)c_iter->data;

		/* Look to see if there is a snippet with the same key in the database */
		conflicting_snippet = g_hash_table_lookup (priv->snippet_keys_map,
		                                           snippet_get_key (cur_snippet));

		/* If we found a conflicting snippet */
		if (conflicting_snippet)
		{
			/* If we should replace the old snippet then we remove it from the database */
			if (overwrite_snippets)
			{
				snippets_db_remove_snippet (snippets_db, 
				                            snippet_get_key (conflicting_snippet));
			}
			/* If we should keep the old snippet we remove the current one from the group */
			else
			{
				snippets_group_remove_snippet (snippets_group,
				                               snippet_get_key (cur_snippet));
			}
		}

		/* Add it to the hash-table */
		g_hash_table_insert (priv->snippet_keys_map, 
		                     snippet_get_key (cur_snippet),
		                     cur_snippet);
	}

	g_object_ref (snippets_group);
	
	return TRUE;
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
	AnjutaSnippetsGroup *snippets_group = NULL;
	SnippetsDBPrivate *priv = NULL;
	GList *iter = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db),
	                      NULL);

	/* Look up the AnjutaSnippetsGroup object with the name being group_name */
	priv = snippets_db->priv;
	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		snippets_group = (AnjutaSnippetsGroup *)iter->data;
		if (!g_strcmp0 (snippets_group_get_name (snippets_group), group_name))
		{
			return snippets_group;
		}
	}
	
	return NULL;
}

/**
 * snippets_db_get_global_variable_text:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: The variable name.
 *
 * Gets the actual text of the variable. If it's a command it will return the command,
 * not the output. If it's static it will have the same result as #snippets_db_get_global_variable.
 * If it's internal it will return an empty string.
 *
 * Returns: The actual text of the global variable.
 */
gchar*
snippets_db_get_global_variable_text (SnippetsDB* snippets_db,
                                      const gchar* variable_name)
{
	GtkTreeIter *iter = NULL;
	GtkListStore *global_vars_store = NULL;
	gboolean is_internal = FALSE;
	gchar *value = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
                          GTK_IS_LIST_STORE (snippets_db->priv->global_variables),
	                      NULL);
	global_vars_store = snippets_db->priv->global_variables;

	/* Search for the variable */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		/* Check if it's internal or not */
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal, 
		                    -1);

		/* If it's internal we return an empty string */
		if (is_internal)
		{
			return g_strdup("");
		}
		else
		{
			/* If it's a command we launch that command and return the output */
			gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
				                GLOBAL_VARS_MODEL_COL_VALUE, &value, 
				                -1);
			return value;
		}
	}

	return NULL;

}

/**
 * snippets_db_get_global_variable:
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
	GtkTreeIter *iter = NULL;
	GtkListStore *global_vars_store = NULL;
	gboolean is_command = FALSE, is_internal = FALSE, command_success = FALSE;
	gchar *value = NULL, *command_line = NULL, *command_output = NULL, *command_error = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
                          GTK_IS_LIST_STORE (snippets_db->priv->global_variables),
	                      NULL);
	global_vars_store = snippets_db->priv->global_variables;

	/* Search for the variable */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		/* Check if it's a command/internal or not */
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, &is_command, 
		                    -1);
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal, 
		                    -1);

		/* If it's internal we call a function defined above to compute the value */
		if (is_internal)
		{
			return get_internal_global_variable_value (snippets_db->anjuta_shell,
			                                           variable_name);
		}
		/* If it's a command we launch that command and return the output */
		else if (is_command)
		{
			gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
			                    GLOBAL_VARS_MODEL_COL_VALUE, &command_line, 
			                    -1);
			command_success = g_spawn_command_line_sync (command_line,
			                                             &command_output,
			                                             &command_error,
			                                             NULL,
			                                             NULL);
			g_free (command_line);
			g_free (command_error);
			if (command_success)
			{
				/* If the last character is a newline we eliminate it */
				gint command_output_size = 0;
				while (command_output[command_output_size] != 0)
					command_output_size ++;
				if (command_output[command_output_size - 1] == '\n')
					command_output[command_output_size - 1] = 0;
					
				return command_output;
			}
			g_return_val_if_reached (NULL);
		}
		/* If it's static just return the value stored */
		else
		{
			gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
			                    GLOBAL_VARS_MODEL_COL_VALUE, &value, 
			                    -1);
			return value;
		
		}		
	}

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
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean found = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                      GTK_IS_LIST_STORE (snippets_db->priv->global_variables) &&
	                      variable_name != NULL,
	                      FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	
	/* Locate the variable in the GtkListStore */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	found = (iter != NULL);
	if (iter)
		gtk_tree_iter_free (iter);

	return found;
}

/**
 * snippets_db_add_global_variable:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: A variable name.
 * @variable_value: The global variable value.
 * @variable_is_command: If the variable is the output of a command.
 * @overwrite: If a global variable with the same name exists, it should be overwriten.
 *
 * Adds a global variable to the Snippets Database.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_add_global_variable (SnippetsDB* snippets_db,
                                 const gchar* variable_name,
                                 const gchar* variable_value,
                                 gboolean variable_is_command,
                                 gboolean overwrite)
{
	GtkTreeIter *iter = NULL, iter_to_add;
	GtkListStore *global_vars_store = NULL;
	gboolean is_internal = FALSE;

	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                      GTK_IS_LIST_STORE (snippets_db->priv->global_variables) &&
	                      variable_name != NULL && variable_value != NULL,
	                      FALSE);
	global_vars_store = snippets_db->priv->global_variables;

	/* Check to see if there is a global variable with the same name in the database */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);

		/* If it's internal it can't be overwriten */
		if (overwrite && !is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_NAME, variable_name,
			                    GLOBAL_VARS_MODEL_COL_VALUE, variable_value,
			                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, variable_is_command,
			                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, FALSE,
			                    -1);
			gtk_tree_iter_free (iter);
			return TRUE;	
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}
	else
	{
		/* Add the the global_vars_store */
		gtk_list_store_append (global_vars_store, &iter_to_add);
		gtk_list_store_set (global_vars_store, &iter_to_add,
		                    GLOBAL_VARS_MODEL_COL_NAME, variable_name,
		                    GLOBAL_VARS_MODEL_COL_VALUE, variable_value,
		                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, variable_is_command,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, FALSE,
		                    -1);
	}
	return TRUE;
}

/**
 * snippets_db_set_global_variable_name:
 * @snippets_db: A #SnippetsDB object.
 * @variable_old_name: The old name of the variable
 * @variable_new_name: The name with which it should be changed.
 *
 * Changed the Global Variable name.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_set_global_variable_name (SnippetsDB* snippets_db,
                                      const gchar* variable_old_name,
                                      const gchar* variable_new_name)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                      GTK_IS_LIST_STORE (snippets_db->priv->global_variables) &&
	                      variable_old_name != NULL,
	                      FALSE);
	global_vars_store = snippets_db->priv->global_variables;

	/* Test if the variable_new_name is already in the database */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_new_name);
	if (iter)
	{
		gtk_tree_iter_free (iter);
		return FALSE;
	}

	/* Get a GtkTreeIter pointing at the global variable to be updated */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_old_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);
		                    
		if (!is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_NAME, variable_new_name,
			                    -1);
			gtk_tree_iter_free (iter);
			return TRUE;
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}

	return FALSE;	
}

/**
 * snippets_db_set_global_variable_value:
 * @snippets_db: A #SnippetsDB value.
 * @variable_name: The name of the global variable to be updated.
 * @variable_new_value: The new value to be set to the variable.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_set_global_variable_value (SnippetsDB* snippets_db,
                                       const gchar* variable_name,
                                       const gchar* variable_new_value)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;
	gchar *stored_value = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                      GTK_IS_LIST_STORE (snippets_db->priv->global_variables) &&
	                      variable_name != NULL,
	                      FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	/* Get a GtkTreeIter pointing at the global variable to be updated */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_VALUE, &stored_value,
		                    -1);
		                    
		if (!is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_VALUE, variable_new_value,
			                    -1);
			                    
			g_free (stored_value);
			gtk_tree_iter_free (iter);

			return TRUE;
		}
		else
		{
			g_free (stored_value);
			gtk_tree_iter_free (iter);
			
			return FALSE;
		}
	}

	return FALSE;
}


/**
 * snippets_db_set_global_variable_type:
 * @snippets_db: A #SnippetsDB value.
 * @variable_name: The name of the global variable to be updated.
 * @is_command: TRUE if after the update the global variable should be considered a command.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_set_global_variable_type (SnippetsDB *snippets_db,
                                      const gchar* variable_name,
                                      gboolean is_command)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;

	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                      GTK_IS_LIST_STORE (snippets_db->priv->global_variables) &&
	                      variable_name != NULL,
	                      FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	
	/* Get a GtkTreeIter pointing at the global variable to be updated */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);

		if (!is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, is_command,
			                    -1);
			gtk_tree_iter_free (iter);
			return TRUE;
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}

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
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                      GTK_IS_LIST_STORE (snippets_db->priv->global_variables) &&
	                      variable_name != NULL,
	                      FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	
	/* Get a GtkTreeIter pointing at the global variable to be removed */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal, 
		                    -1);

		if (!is_internal)
		{
			gtk_list_store_remove (global_vars_store, iter);
			gtk_tree_iter_free (iter);
			return TRUE;
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}
		
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
snippets_db_get_global_vars_model (SnippetsDB* snippets_db)
{
	/* Assertions */
	g_return_val_if_fail (snippets_db != NULL && ANJUTA_IS_SNIPPETS_DB (snippets_db) &&
	                      snippets_db->priv != NULL && snippets_db->priv->global_variables != NULL &&
	                      GTK_IS_LIST_STORE (snippets_db->priv->global_variables),
	                      NULL);
	
	return GTK_TREE_MODEL (snippets_db->priv->global_variables);
}

/* GtkTreeModel methods definition */
static GtkTreeModelFlags 
snippets_db_get_flags (GtkTreeModel *tree_model)
{
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model),
	                      (GtkTreeModelFlags)0);

	return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
snippets_db_get_n_columns (GtkTreeModel *tree_model)
{
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model),
	                      0);

	return SNIPPETS_DB_MODEL_COL_N;
}

static GType
snippets_db_get_column_type (GtkTreeModel *tree_model,
                             gint index)
{
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model) &&
	                      index >= 0 && index < SNIPPETS_DB_MODEL_COL_N,
	                      G_TYPE_INVALID);

	if (index == 1)
		return G_TYPE_BOOLEAN;
	else
		return G_TYPE_STRING;
}

static gboolean
snippets_db_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter *iter,
                      GtkTreePath *path)
{
	SnippetsDB *snippets_db = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	GList *snippets_group_node = NULL, *snippet_node = NULL;
	gint *indices = NULL, depth = 0, db_count = 0, group_count = 0;
	gpointer parent_node = NULL, own_node = NULL;
	
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model) &&
	                      path != NULL,
	                      FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (snippets_db);
	
	/* Get the indices and depth of the path and assert the depth */
	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);
	g_return_val_if_fail (depth > 2, FALSE);
	db_count = indices[0];
	if (depth == 2)
		group_count = indices[1];

	/* Get the snippets_group node for the given db_count */
	snippets_group_node = g_list_nth (snippets_db->priv->snippets_groups, db_count);
	g_return_val_if_fail (snippets_group_node != NULL,
	                      FALSE);

	/* If the path points to the SnippetsGroups level */
	if (depth == 1)
	{
		parent_node = NULL;
		own_node = snippets_group_node;
	}
	/* If the path points to the Snippets level */
	else
	{
		/* Get the Snippet node */
		snippets_group = (AnjutaSnippetsGroup *)snippets_group_node->data;
		g_return_val_if_fail (snippets_group != NULL && ANJUTA_IS_SNIPPETS_GROUP (snippets_group),
		                      FALSE);
		snippet_node = g_list_nth ((GList *)snippets_group_get_snippets_list (snippets_group),
		                           group_count);
		g_return_val_if_fail (snippet_node != NULL,
		                      FALSE);

		parent_node = snippets_group_node;
		own_node = snippet_node;
	}
	
	/* Finally complete the iter fields */
	iter->user_data  = parent_node;
	iter->user_data2 = own_node;
	iter->user_data3 = NULL;
	iter->stamp = snippets_db->stamp;
	
	return TRUE;
}

static GtkTreePath*
snippets_db_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter *iter)
{
	GtkTreePath *path = NULL;
	SnippetsDB *snippets_db = NULL;
	gint count = 0;
	GList *l_iter = NULL;
	
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model) &&
	                      iter != NULL && iter->user_data2 != NULL,
	                      NULL);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Make a new GtkTreePath object */
	path = gtk_tree_path_new ();

	/* Get the first index */
	l_iter = iter->user_data2;
	while (l_iter != NULL)
	{
		count ++;
		l_iter = g_list_previous (l_iter);
	}
	gtk_tree_path_append_index (path, count);
	
	/* If it's a snippet node, then it has a parent */
	if (iter->user_data != NULL)
	{
		l_iter = iter->user_data;

		while (l_iter != NULL)
		{
			count ++;
			l_iter = g_list_previous (l_iter);
		}
		gtk_tree_path_append_index (path, count);
	}

	return path;
}

static void
snippets_db_get_value (GtkTreeModel *tree_model,
                       GtkTreeIter *iter,
                       gint column,
                       GValue *value)
{
	SnippetsDB *snippets_db = NULL;
	AnjutaSnippet *snippet = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	GList *node = NULL;
	
	/* Assertions */
	g_return_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model) &&
	                  iter != NULL && iter->user_data2 != NULL &&
	                  column >= 0 && column < SNIPPETS_DB_MODEL_COL_N);

	/* Initializations */
	g_value_init (value, snippets_db_get_column_type (tree_model, column));
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Get the data in the node */
	node = (GList *)iter->user_data2;
	if (iter->user_data == NULL)
		snippet = (AnjutaSnippet *)node->data;
	else
		snippets_group = (AnjutaSnippetsGroup *)node->data;

	/* Fill value based on column */
	switch (column)
	{
		case SNIPPETS_DB_MODEL_COL_NAME:
			if (snippet)
				g_value_set_static_string (value, snippet_get_name (snippet));
			else
				g_value_set_static_string (value, snippets_group_get_name (snippets_group));
			break;

		case SNIPPETS_DB_MODEL_COL_IS_SNIPPET:
			if (snippet)
				g_value_set_boolean (value, TRUE);
			else
				g_value_set_boolean (value, FALSE);
			break;

		case SNIPPETS_DB_MODEL_COL_DEFAULT_CONTENT:
			if (snippet)
				g_value_set_string (value, 
				                    (gpointer)snippet_get_default_content (snippet,
				                                                           G_OBJECT (snippets_db),
				                                                           ""));
			else
				g_value_set_string (value, NULL);
			break;

		case SNIPPETS_DB_MODEL_COL_LANG:
			if (snippet)
				g_value_set_static_string (value, snippet_get_language (snippet));
			else
				g_value_set_static_string (value, NULL);
			break;

		case SNIPPETS_DB_MODEL_COL_TRIGGER:
			if (snippet)
				g_value_set_static_string (value, snippet_get_trigger_key (snippet));
			else
				g_value_set_static_string (value, NULL);
			break;

		case SNIPPETS_DB_MODEL_COL_SNIPPET_KEY:
			if (snippet)
				g_value_set_string (value, snippet_get_key (snippet));
			else
				g_value_set_string (value, NULL);
			break;
	}
	
}

static gboolean
snippets_db_iter_next (GtkTreeModel *tree_model,
                       GtkTreeIter *iter)
{
	GList *l_iter = NULL;
	SnippetsDB* snippets_db = NULL;
	
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model) &&
	                      iter != NULL && iter->user_data2 != NULL,
	                      FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Check if the stamp is correct */
	g_return_val_if_fail (snippets_db->stamp == iter->stamp,
	                      FALSE);

	/* Update the iter */
	l_iter = (GList *)iter->user_data2;
	l_iter = g_list_next (l_iter);
	iter->user_data2 = l_iter;

	return TRUE;
}

static gboolean
snippets_db_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter *iter,
                           GtkTreeIter *parent)
{
	GList *parent_node = NULL, *own_node = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	SnippetsDB *snippets_db = NULL;
	
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model),
	                      FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* Get the first node of the SnippetsGroup list */
	if (parent == NULL)
	{
		parent_node = NULL;
		own_node = snippets_db->priv->snippets_groups;
	}

	/* If it's a Snippets Group node */
	if (parent->user_data == NULL)
	{
		/* Get the snippets_group object and assert it */
		snippets_group = (AnjutaSnippetsGroup *)parent->user_data2;
		g_return_val_if_fail (snippets_group != NULL &&
		                      ANJUTA_IS_SNIPPETS_GROUP (snippets_group) &&
		                      snippets_db->stamp == parent->stamp,
		                      FALSE);

		/* Get the first snippet of the snippets_group object */
		own_node = (GList *)snippets_group_get_snippets_list (snippets_group);		
		parent_node = parent->user_data2;

	}

	/* If it's a Snippet node */
	if (parent->user_data != NULL)
		return FALSE;

	/* Fill the iter structure */
	iter->user_data  = parent_node;
	iter->user_data2 = own_node;
	iter->user_data3 = NULL;
	iter->stamp      = snippets_db->stamp;

	return TRUE;
}

static gboolean
snippets_db_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model) &&
	                      iter != NULL && iter->user_data2 != NULL,
	                      FALSE);

	/* If the parent field is NULL then it's the 1st level so it has children */
	return iter->user_data == NULL;
}

static gint
snippets_db_iter_n_children (GtkTreeModel *tree_model,
                             GtkTreeIter *iter)
{
	const GList* snippets_list = NULL;
	SnippetsDB *snippets_db = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model),
	                      -1);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* If a top-level count is requested */
	if (iter == NULL)
	{
		return (gint)g_list_length (snippets_db->priv->snippets_groups);
	}

	/* If iter points to a SnippetsGroup node */
	if (iter->user_data == NULL)
	{
		/* Get the Snippets Group object and assert it */
		snippets_group = ANJUTA_SNIPPETS_GROUP (iter->user_data2);
		g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group),
		                      -1);

		snippets_list = snippets_group_get_snippets_list (snippets_group);
		return (gint)g_list_length ((GList *)snippets_list);
	}

	/* If iter points to a Snippet node than it has no children. */
	return 0;
}

static gboolean
snippets_db_iter_nth_child (GtkTreeModel *tree_model,
                            GtkTreeIter *iter,
                            GtkTreeIter *parent,
                            gint n)
{
	SnippetsDB *snippets_db = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	GList *parent_node = NULL, *own_node = NULL;
	const GList *snippets_list = NULL;

	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model),
	                      FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (snippets_db);

	/* If it's a top level request */
	if (parent == NULL)
	{
		parent_node = NULL;
		own_node = g_list_nth (snippets_db->priv->snippets_groups, n);
		if (own_node == NULL)
			return FALSE;
	}

	g_return_val_if_fail (parent != NULL && parent->user_data2 != NULL,
	                      FALSE);
	/* If it's a SnippetsGroup Node */
	if (parent->user_data == NULL)
	{
		parent_node = (GList *)parent->user_data2;
		snippets_group = (AnjutaSnippetsGroup *)parent_node->data;
		g_return_val_if_fail (snippets_group != NULL && 
		                      ANJUTA_IS_SNIPPETS_DB (snippets_group),
		                      FALSE);
		
		snippets_list = snippets_group_get_snippets_list (snippets_group);
		own_node = g_list_nth ((GList *)snippets_list, n);
	}

	/* If it's a Snippet node. No children. We could of skipped this check. */
	if (parent->user_data != NULL)
	{
		return FALSE;
	}

	/* Fill the iter fields */
	iter->user_data  = parent_node;
	iter->user_data2 = own_node;
	iter->user_data3 = NULL;
	iter->stamp      = snippets_db->stamp;
	
	return FALSE;
}

static gboolean
snippets_db_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter *iter,
                         GtkTreeIter *child)
{
	SnippetsDB *snippets_db = NULL;
	
	/* Assertions */
	g_return_val_if_fail (tree_model != NULL && ANJUTA_IS_SNIPPETS_DB (tree_model) &&
	                      child != NULL && child->user_data != NULL,
	                      FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* Fill the iter fields */
	iter->user_data  = NULL;
	iter->user_data2 = child->user_data;
	iter->user_data3 = NULL;
	iter->stamp      = snippets_db->stamp;

	return TRUE;
}
