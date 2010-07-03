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
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#define DEFAULT_GLOBAL_VARS_FILE            "snippets-global-variables.xml"
#define COMMENTS_LICENSES_INSTALL_PATH      PACKAGE_DATA_DIR"/"COMMENTS_LICENSES_SNIPPETS_FILE
#define GENERAL_PURPOSE_INSTALL_PATH        PACKAGE_DATA_DIR"/"GENERAL_PURPOSE_SNIPPETS_FILE
#define DEFAULT_GLOBAL_VARS_INSTALL_PATH    PACKAGE_DATA_DIR"/"DEFAULT_GLOBAL_VARS_FILE
#define USER_SNIPPETS_DB_DIR                "snippets-database"
#define USER_SNIPPETS_DIR                   "snippets-database/snippet-packages"

#define SNIPPETS_DB_MODEL_DEPTH             2

/* Internal global variables */
#define GLOBAL_VAR_FILE_NAME       "filename"
#define GLOBAL_VAR_USER_NAME       "username"
#define GLOBAL_VAR_USER_FULL_NAME  "userfullname"
#define GLOBAL_VAR_HOST_NAME       "hostname"

#define ANJUTA_SNIPPETS_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_DB, SnippetsDBPrivate))

static gchar* default_snippet_files[] = {
	"general-purpose-snippets.xml",
	"comments-and-licenses-snippets.xml"
};

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
static void              snippets_db_dispose         (GObject* obj);

static void              snippets_db_finalize        (GObject* obj);

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

static GtkTreePath *
get_tree_path_for_snippets_group (SnippetsDB *snippets_db,
                                  AnjutaSnippetsGroup *snippets_group);
static GtkTreePath *
get_tree_path_for_snippet (SnippetsDB *snippets_db,
                           AnjutaSnippet *snippet);

static gchar *
get_snippet_key_from_trigger_and_language (const gchar *trigger_key,
                                           const gchar *language)
{
	gchar *lower_language = NULL;
	gchar *snippet_key = NULL;
	
	/* Assertions */
	g_return_val_if_fail (trigger_key != NULL, NULL);
	g_return_val_if_fail (language != NULL, NULL);

	/* All languages are with lower letters */
	lower_language = g_utf8_strdown (language, -1);

	snippet_key = g_strconcat (trigger_key, ".", lower_language, NULL);
	g_free (lower_language);

	return snippet_key;
}

static void
add_snippet_to_searching_trees (SnippetsDB *snippets_db,
                                AnjutaSnippet *snippet)
{
	/* TODO */
}

static void
add_snippet_to_hash_table (SnippetsDB *snippets_db,
                           AnjutaSnippet *snippet)
{
	GList *iter = NULL, *languages = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	languages = (GList *)snippet_get_languages (snippet);

	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		gchar *snippet_key = NULL;
		snippet_key = get_snippet_key_from_trigger_and_language (snippet_get_trigger_key (snippet),
		                                                         (const gchar *)iter->data);
		g_hash_table_insert (snippets_db->priv->snippet_keys_map,
		                     snippet_key,
		                     snippet);
	}
}

static void
remove_snippet_from_searching_trees (SnippetsDB *snippets_db,
                                     AnjutaSnippet *snippet)
{
	/* TODO */
}

static void
remove_snippet_from_hash_table (SnippetsDB *snippets_db,
                                AnjutaSnippet *snippet)
{
	GList *languages = NULL, *iter = NULL;
	gchar *cur_language = NULL, *cur_snippet_key = NULL, *trigger_key = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	
	languages = (GList *)snippet_get_languages (snippet);
	trigger_key = (gchar *)snippet_get_trigger_key (snippet);
	
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		cur_language = (gchar *)iter->data;
		cur_snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, cur_language);

		if (cur_snippet_key == NULL)
			continue;

		g_hash_table_remove (snippets_db->priv->snippet_keys_map, cur_snippet_key);
	}
}

static void
remove_snippets_group_from_hash_table (SnippetsDB *snippets_db,
                                       AnjutaSnippetsGroup *snippets_group)
{
	GList *snippets = NULL, *iter = NULL;
	AnjutaSnippet *cur_snippet = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group));

	snippets = (GList *)snippets_group_get_snippets_list (snippets_group);

	/* Iterate over all the snippets in the group, and remove all the snippet keys
	   a snippet has stored. */
	for (iter = g_list_first (snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		g_return_if_fail (ANJUTA_IS_SNIPPET (cur_snippet));

		remove_snippet_from_hash_table (snippets_db, cur_snippet);
	}
}

static void
remove_snippets_group_from_searching_trees (SnippetsDB *snippets_db,
                                            AnjutaSnippetsGroup *snippets_group)
{
	/* TODO */
}

static AnjutaSnippet*
get_conflicting_snippet (SnippetsDB *snippets_db,
                         AnjutaSnippet *new_snippet)
{
	GList *languages = NULL, *iter = NULL;
	const gchar *language = NULL, *trigger_key = NULL;
	gchar *snippet_key = NULL;
	AnjutaSnippet *conflicting_snippet = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (snippets_db->priv != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (new_snippet), NULL);

	trigger_key = snippet_get_trigger_key (new_snippet);
	languages = (GList *)snippet_get_languages (new_snippet);
	
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		language = (gchar *)iter->data;
		snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, language);

		conflicting_snippet = g_hash_table_lookup (snippets_db->priv->snippet_keys_map, 
		                                           snippet_key);
		if (conflicting_snippet)
			return conflicting_snippet;
	}

	return NULL;
}

static void
copy_default_files_to_user_folder (SnippetsDB *snippets_db)
{
	/* In this function we should copy the default snippet files and the global variables
	   files in the user folder if there aren't already files with the same name in that
	   folder */
	gchar *user_snippets_path = NULL, *user_snippets_db_path = NULL, *cur_file_user_path = NULL,
	      *cur_file_installation_path = NULL, *global_vars_user_path = NULL,
	      *global_vars_installation_path = NULL;
	gint i = 0;
	gboolean cur_file_exists = FALSE, copy_success = FALSE;
	GFile *cur_installation_file = NULL, *cur_user_file = NULL,
	      *global_vars_installation_file = NULL, *global_vars_user_file = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	/* Compute the user_snippets and user_snippets_db file paths */
	user_snippets_path    = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DIR, "/", NULL);
	user_snippets_db_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", NULL);

	/* Copy the snippet-package files if they don't exist in the user_folder */
	for (i = 0; i < G_N_ELEMENTS (default_snippet_files); i ++)
	{
		cur_file_user_path         = g_strconcat (user_snippets_path, "/", default_snippet_files[i], NULL);
		cur_file_installation_path = g_strconcat (PACKAGE_DATA_DIR, "/", default_snippet_files[i], NULL);

		/* We test if the current file is in the user folder. If it isn't we copy it from the installation
		   folder. */
		cur_file_exists = g_file_test (cur_file_user_path, G_FILE_TEST_EXISTS);
		if (!cur_file_exists)
		{
			cur_installation_file = g_file_new_for_path (cur_file_installation_path);
			cur_user_file         = g_file_new_for_path (cur_file_user_path);

			copy_success = g_file_copy (cur_installation_file,
			                            cur_user_file,
			                            G_FILE_COPY_NONE,
			                            NULL, NULL, NULL, NULL);

			if (!copy_success)
				DEBUG_PRINT ("Copying of %s failed", default_snippet_files[i]);
		}

		g_free (cur_file_user_path);
		g_free (cur_file_installation_path);
	}

	/* Copy the global variables file */
	global_vars_user_path  = g_strconcat (user_snippets_db_path, "/", DEFAULT_GLOBAL_VARS_FILE, NULL);
	global_vars_installation_path = g_strconcat (PACKAGE_DATA_DIR, "/", DEFAULT_GLOBAL_VARS_FILE, NULL);

	cur_file_exists = g_file_test (global_vars_user_path, G_FILE_TEST_EXISTS);
	if (!cur_file_exists)
	{
		global_vars_installation_file = g_file_new_for_path (global_vars_installation_path);
		global_vars_user_file         = g_file_new_for_path (global_vars_user_path);

		copy_success = g_file_copy (global_vars_installation_file,
		                            global_vars_user_file,
		                            G_FILE_COPY_NONE,
		                            NULL, NULL, NULL, NULL);

		if (!copy_success)
			DEBUG_PRINT ("Copying of %s failed", DEFAULT_GLOBAL_VARS_FILE);
	}
	g_free (global_vars_user_path);
	g_free (global_vars_installation_path);

	g_free (user_snippets_path);
	g_free (user_snippets_db_path);
}

static void
load_internal_global_variables (SnippetsDB *snippets_db)
{
	GtkTreeIter iter_added;
	GtkListStore *global_vars_store = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (snippets_db->priv != NULL);
	g_return_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables));
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

static void
load_global_variables (SnippetsDB *snippets_db)
{
	gchar *global_vars_user_path = NULL, *snippets_db_user_path = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	/* Load the internal global variables */
	load_internal_global_variables (snippets_db);

	snippets_db_user_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", NULL);
	global_vars_user_path = g_strconcat (snippets_db_user_path, "/", DEFAULT_GLOBAL_VARS_FILE, NULL);

	snippets_manager_parse_variables_xml_file (global_vars_user_path, snippets_db);

	g_free (snippets_db_user_path);
	g_free (global_vars_user_path);	
}

static void
load_default_snippets (SnippetsDB *snippets_db)
{
	GDir *user_snippets_dir = NULL;
	gchar *user_snippets_path = NULL, *cur_file_path = NULL;
	const gchar *cur_file_name = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	user_snippets_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DIR, "/", NULL);
	user_snippets_dir  = g_dir_open (user_snippets_path, 0, NULL);
	
	cur_file_name = g_dir_read_name (user_snippets_dir);
	while (cur_file_name)
	{
		cur_file_path = g_strconcat (user_snippets_path, "/", cur_file_name, NULL);

		snippets_db_load_file (snippets_db, cur_file_path, TRUE, TRUE, NATIVE_FORMAT);

		g_free (cur_file_path);
		cur_file_name = g_dir_read_name (user_snippets_dir);
	}
	g_dir_close (user_snippets_dir);

	g_free (user_snippets_path);
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
	g_return_val_if_fail (GTK_IS_LIST_STORE (global_vars_store), NULL);

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

	return g_utf8_collate (trigger_key1, trigger_key2);
}

static gint
keywords_tree_compare_func (gconstpointer a,
                            gconstpointer b,
                            gpointer user_data)
{
	gchar* keyword1 = (gchar *)a;
	gchar* keyword2 = (gchar *)b;

	return g_utf8_collate (keyword1, keyword2);
}

static gint
compare_snippets_groups_by_name (gconstpointer a,
                                 gconstpointer b)
{
	AnjutaSnippetsGroup *group1 = (AnjutaSnippetsGroup *)a;
	AnjutaSnippetsGroup *group2 = (AnjutaSnippetsGroup *)b;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (group1), 0);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (group2), 0);

	return g_utf8_collate (snippets_group_get_name (group1),
	                       snippets_group_get_name (group2));
}

static void
snippets_db_dispose (GObject* obj)
{
	/* Important: This does not free the memory in the internal structures. You first
	   must use snippets_db_close before disposing the snippets-database. */
	SnippetsDB *snippets_db = NULL;
	
	DEBUG_PRINT ("%s", "Disposing SnippetsDB …");

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	snippets_db = ANJUTA_SNIPPETS_DB (obj);
	g_return_if_fail (snippets_db->priv != NULL);
	
	g_list_free (snippets_db->priv->snippets_groups);
	g_hash_table_destroy (snippets_db->priv->snippet_keys_map);
	g_tree_unref (snippets_db->priv->keywords_tree);
	g_tree_unref (snippets_db->priv->trigger_keys_tree);

	snippets_db->priv->snippets_groups   = NULL;
	snippets_db->priv->snippet_keys_map  = NULL;
	snippets_db->priv->keywords_tree     = NULL;
	snippets_db->priv->trigger_keys_tree = NULL;
	
	G_OBJECT_CLASS (snippets_db_parent_class)->dispose (obj);
}

static void
snippets_db_finalize (GObject* obj)
{
	DEBUG_PRINT ("%s", "Finalizing SnippetsDB …");
	
	G_OBJECT_CLASS (snippets_db_parent_class)->finalize (obj);
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
 * A new #SnippetDB object.
 *
 * Returns: A new #SnippetsDB object.
 **/
SnippetsDB*	
snippets_db_new ()
{
	return ANJUTA_SNIPPETS_DB (g_object_new (snippets_db_get_type (), NULL));
}

/**
 * snippets_db_load:
 * @snippets_db: A #SnippetsDB object
 *
 * Loads the given @snippets_db with snippets/global-variables loaded from the default
 * folder.
 */
void                       
snippets_db_load (SnippetsDB *snippets_db)
{
	gchar *user_snippets_path = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	/* Make sure we have the user folder */
	user_snippets_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DIR, "/", NULL);
	g_mkdir_with_parents (user_snippets_path, 0755);

	/* Check if the default snippets file is in the user directory and copy them
	   over from the installation folder if they aren't*/
	copy_default_files_to_user_folder (snippets_db);

	/* Load the snippets and global variables */
	load_global_variables (snippets_db);
	load_default_snippets (snippets_db);
}

/**
 * snippets_db_close:
 * @snippets_db: A #SnippetsDB object.
 *
 * Saves the snippets and free's the loaded data from the internal structures (not the 
 * internal structures themselvs, so after calling snippets_db_load, the snippets_db
 * will be functional).
 */
void                       
snippets_db_close (SnippetsDB *snippets_db)
{
	GList *iter = NULL;
	AnjutaSnippetsGroup *cur_snippets_group = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (snippets_db->priv != NULL);

	/* Save all the files */
	snippets_db_save_all (snippets_db);

	/* Free the memory for the snippets-groups in the SnippetsDB */
	for (iter = g_list_first (snippets_db->priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippets_group = (AnjutaSnippetsGroup *)iter->data;
		g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (cur_snippets_group));

		g_object_unref (cur_snippets_group);
	}
	g_list_free (snippets_db->priv->snippets_groups);
	snippets_db->priv->snippets_groups = NULL;

	/* Free the hash-table memory */
	g_hash_table_ref (snippets_db->priv->snippet_keys_map);
	g_hash_table_destroy (snippets_db->priv->snippet_keys_map);
	
	/* Destroy the searching trees */
	g_tree_ref (snippets_db->priv->keywords_tree);
	g_tree_ref (snippets_db->priv->trigger_keys_tree);
	g_tree_destroy (snippets_db->priv->keywords_tree);
	g_tree_destroy (snippets_db->priv->trigger_keys_tree);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippet_packet_file_path != NULL, FALSE);

	/* Get the AnjutaSnippetsGroup object from the file */
	snippets_group = snippets_manager_parse_snippets_xml_file (snippet_packet_file_path,
	                                                           format_type);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);

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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (added_snippet), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);

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
				add_snippet_to_hash_table (snippets_db, added_snippet);
			}

			return added_to_group;
		}
	}
	
	return FALSE;
}

/**
 * snippets_db_get_snippet:
 * @snippets_db: A #SnippetsDB object
 * @trigger_key: The trigger-key of the requested snippet
 * @language: The snippet language. NULL for auto-detection.
 *
 * Gets a snippet from the database for the given trigger-key. If language is NULL,
 * it will get the snippet for the current editor language.
 *
 * Returns: The requested snippet (not a copy, should not be freed) or NULL if not found.
 **/
const AnjutaSnippet*	
snippets_db_get_snippet (SnippetsDB* snippets_db,
                         const gchar* trigger_key,
                         const gchar* language)
{
	AnjutaSnippet *snippet = NULL;
	gchar *snippet_key = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (trigger_key != NULL, NULL);

	/* Get the editor language if not provided */
	if (language == NULL)
	{
		IAnjutaDocumentManager *docman = NULL;
		IAnjutaDocument *doc = NULL;

		docman = anjuta_shell_get_interface (snippets_db->anjuta_shell, 
		                                     IAnjutaDocumentManager, 
		                                     NULL);
		g_return_val_if_fail (docman != NULL, NULL);

		/* Get the current doc and make sure it's an editor */
		doc = ianjuta_document_manager_get_current_document (docman, NULL);
		g_return_val_if_fail (IANJUTA_IS_EDITOR (doc), NULL);

		language = ianjuta_editor_language_get_language (IANJUTA_EDITOR_LANGUAGE (doc), NULL);
	}

	/* Calculate the snippet-key */
	snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, language);

	g_return_val_if_fail (snippet_key != NULL, NULL);
	/* Look up the the snippet in the hashtable */
	snippet = g_hash_table_lookup (snippets_db->priv->snippet_keys_map, snippet_key);
	
	return snippet;
}

/**
 * snippets_db_remove_snippet:
 * @snippets_db: A #SnippetsDB object.
 * @trigger-key: The snippet to be removed trigger-key.
 * @language: The language of the snippet. This must not be NULL, as it won't take
 *            the document language.
 * @remove_all_languages_support: If this is FALSE, it won't actually remove the snippet,
 *                                but remove the given language support for the snippet.
 *
 * Removes a snippet from the #SnippetDB (or removes it's language support).
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_remove_snippet (SnippetsDB* snippets_db,
                            const gchar* trigger_key,
                            const gchar* language,
                            gboolean remove_all_languages_support)
{
	AnjutaSnippet *deleted_snippet = NULL;
	AnjutaSnippetsGroup *deleted_snippet_group = NULL;
	gchar *snippet_key = NULL;
	GtkTreePath *path = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, language);
	g_return_val_if_fail (snippet_key != NULL, FALSE);
	g_return_val_if_fail (language != NULL, FALSE);
	
	/* Get the snippet to be deleted */
	deleted_snippet = g_hash_table_lookup (snippets_db->priv->snippet_keys_map,
	                                       snippet_key);
	g_return_val_if_fail (deleted_snippet != NULL, FALSE);

	if (remove_all_languages_support)
	{
		remove_snippet_from_hash_table (snippets_db, deleted_snippet);
		remove_snippet_from_searching_trees (snippets_db, deleted_snippet);
	}
	else
	{
		/* We remove just the current language support from the database */
		g_hash_table_remove (snippets_db->priv->snippet_keys_map, snippet_key);

		/* TODO - remove just this snippet-key from the searching tree's */
	}

	/* Emit the signal that the snippet was deleted */
	path = get_tree_path_for_snippet (snippets_db, deleted_snippet);

	g_signal_emit_by_name (G_OBJECT (snippets_db),
	                       "row-deleted",
	                       path, NULL);
	gtk_tree_path_free (path);
	
	/* Remove it from the snippets group */
	deleted_snippet_group = ANJUTA_SNIPPETS_GROUP (deleted_snippet->parent_snippets_group);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (deleted_snippet_group), FALSE);
	snippets_group_remove_snippet (deleted_snippet_group, 
	                               trigger_key, 
	                               language,
	                               remove_all_languages_support);

	return TRUE;
}

/**
 * snippets_db_set_snippet_keywords:
 * @snippets_db: A #SnippetsDB object.
 * @snippet: An #AnjutaSnippet object.
 * @new_keywords: The new list of keywords for the snippet.
 *
 * Changes the keywords of a snippet, modifying the internal structures as needed.
 *
 * Returns: TRUE on success.
 */
gboolean                   
snippets_db_set_snippet_keywords (SnippetsDB *snippets_db,
                                  AnjutaSnippet *snippet,
                                  const GList *new_keywords)
{
	SnippetsDBPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	/* TODO - modify the keyword searching tree */

	snippet_set_keywords_list (snippet, new_keywords);

	return TRUE;
}

gboolean                   
snippets_db_set_snippet_trigger (SnippetsDB *snippets_db,
                                 AnjutaSnippet *snippet,
                                 const gchar *new_trigger)
{
	SnippetsDBPrivate *priv = NULL;
	const gchar *old_trigger = NULL;
	gchar *cur_language = NULL, *cur_snippet_key = NULL;
	GList *languages = NULL, *iter = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter tree_iter;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	old_trigger = snippet_get_trigger_key (snippet);
	languages = (GList *)snippet_get_languages (snippet);

	/* If the old and new trigger keys are the same, we return TRUE and don't change
	   anything */
	if (!g_strcmp0 (old_trigger, new_trigger))
		return TRUE;
		
	/* Check to see if we have any conflicts with the new trigger-key */
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		cur_language = (gchar *)iter->data;
		cur_snippet_key = get_snippet_key_from_trigger_and_language (old_trigger,
		                                                             cur_language);

		if (g_hash_table_lookup (priv->snippet_keys_map, cur_snippet_key))
		{
			g_free (cur_snippet_key);
			return FALSE;
		}
		g_free (cur_snippet_key);
	}

	/* TODO - modify the trigger searching tree */


	/* Remove the entries with the old trigger-key from the hashtable and
	   add the entries with the new trigger-key */
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		cur_language = (gchar *)iter->data;
		cur_snippet_key = get_snippet_key_from_trigger_and_language (old_trigger, 
		                                                             cur_language);
		g_hash_table_remove (priv->snippet_keys_map, cur_snippet_key);

		cur_snippet_key = get_snippet_key_from_trigger_and_language (new_trigger,
		                                                             cur_language);
		g_hash_table_insert (priv->snippet_keys_map, cur_snippet_key, snippet);
	}

	/* Modify the snippet */
	snippet_set_trigger_key (snippet, new_trigger);

	/* Emit the signal that the database was changed */
	path = get_tree_path_for_snippet (snippets_db, snippet);
	snippets_db_get_iter (GTK_TREE_MODEL (snippets_db), &tree_iter, path);
	g_signal_emit_by_name (G_OBJECT (snippets_db),
	                       "row-changed",
	                       path, &tree_iter, NULL);
	gtk_tree_path_free (path);

	return TRUE;
}

gboolean                   
snippets_db_add_snippet_language (SnippetsDB *snippets_db,
                                  AnjutaSnippet *snippet,
                                  const gchar *language)
{
	SnippetsDBPrivate *priv = NULL;
	const gchar *trigger_key = NULL;
	gchar *snippet_key = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter tree_iter;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	/* Check for a conflict */
	trigger_key = snippet_get_trigger_key (snippet);
	snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, language);
	if (g_hash_table_lookup (priv->snippet_keys_map, snippet_key))
	{
		g_free (snippet_key);
		return FALSE;
	}
	g_free (snippet_key);

	snippet_add_language (snippet, language);

	/* Emit the signal that the database was changed */
	path = get_tree_path_for_snippet (snippets_db, snippet);
	snippets_db_get_iter (GTK_TREE_MODEL (snippets_db), &tree_iter, path);
	g_signal_emit_by_name (G_OBJECT (snippets_db),
	                       "row-changed",
	                       path, &tree_iter, NULL);
	gtk_tree_path_free (path);

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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);

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
			/* Remove from searching tree's */
			remove_snippets_group_from_searching_trees (snippets_db, replaced_group);
			
			/* Remove it's occurences from the hash-table */
			remove_snippets_group_from_hash_table (snippets_db, replaced_group);
			
			/* Delete the old group */
			g_object_unref (replaced_group);

			/* Make the node point to the new group */
			replaced_node->data = snippets_group;
		}
		else
		{
			return FALSE;
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
		conflicting_snippet = get_conflicting_snippet (snippets_db, cur_snippet);
		
		/* If we found a conflicting snippet */
		if (conflicting_snippet)
		{

			/* If we should overwrite the conflicing snippet, we remove the old one and
			   add the new one to the hash table */
			if (overwrite_snippets)
			{
				snippets_db_remove_snippet (snippets_db, 
				                            snippet_get_trigger_key (conflicting_snippet),
				                            snippet_get_any_language (conflicting_snippet),
				                            TRUE);

				add_snippet_to_hash_table (snippets_db, cur_snippet);
				add_snippet_to_searching_trees (snippets_db, cur_snippet);
			}
			else
			{
				snippets_group_remove_snippet (snippets_group,
				                               snippet_get_trigger_key (cur_snippet),
				                               snippet_get_any_language (cur_snippet),
				                               TRUE);
			}
		}
		else
		{
			add_snippet_to_hash_table (snippets_db, cur_snippet);
			add_snippet_to_searching_trees (snippets_db, cur_snippet);
		}
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
	GList *iter = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	GtkTreePath *path = NULL;
	SnippetsDBPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (group_name != NULL, FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);
	
	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		snippets_group = (AnjutaSnippetsGroup *)iter->data;
		g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);
	
		if (!g_strcmp0 (group_name, snippets_group_get_name (snippets_group)))
		{
			/* Remove the snippets from the searching tree's */
			remove_snippets_group_from_searching_trees (snippets_db, snippets_group);
			
			/* Remove the snippets in the group from the hash-table */
			remove_snippets_group_from_hash_table (snippets_db, snippets_group);

			/* Emit the signal that it was deleted */
			path = get_tree_path_for_snippets_group (snippets_db, snippets_group);
			g_signal_emit_by_name (G_OBJECT (snippets_db),
			                       "row-deleted",
			                       path, NULL);
			gtk_tree_path_free (path);
			
			/* Destroy the snippets-group object */
			g_object_unref (snippets_group);

			/* Delete the list node */
			iter->data = NULL;
			priv->snippets_groups = g_list_delete_link (priv->snippets_groups, iter);
			return TRUE;
		}
	}

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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);

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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (snippets_db->priv != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), NULL);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (snippets_db->priv != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), NULL);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
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
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
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
 *          Warning: This isn't a copy, shouldn't be freed or unrefed.
 */
GtkTreeModel*
snippets_db_get_global_vars_model (SnippetsDB* snippets_db)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (snippets_db->priv != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), NULL);
	
	return GTK_TREE_MODEL (snippets_db->priv->global_variables);
}

/* GtkTreeModel methods definition */

static GObject *
iter_get_data (GtkTreeIter *iter)
{
	GList *cur_node = NULL;

	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);
	cur_node = (GList *)iter->user_data;
	g_return_val_if_fail (G_IS_OBJECT (cur_node->data), NULL);
	
	return G_OBJECT (cur_node->data);
}

static gboolean
iter_is_snippets_group_node (GtkTreeIter *iter)
{
	GObject *data = iter_get_data (iter);
	
	return ANJUTA_IS_SNIPPETS_GROUP (data);
}

static gboolean
iter_is_snippet_node (GtkTreeIter *iter)
{
	GObject *data = iter_get_data (iter);

	return ANJUTA_IS_SNIPPET (data);
}

static void
iter_get_first_snippets_db_node (GtkTreeIter *iter,
                                 SnippetsDB *snippets_db)
{
	SnippetsDBPrivate *priv = NULL;
	
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	iter->user_data  = g_list_first (priv->snippets_groups);
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;
	iter->stamp      = snippets_db->stamp;
}

static gboolean
iter_nth (GtkTreeIter *iter, gint n)
{
	g_return_val_if_fail (iter != NULL, FALSE);
	
	iter->user_data = g_list_nth ((GList *)iter->user_data, n);

	return (iter->user_data != NULL);
}

static GList *
iter_get_list_node (GtkTreeIter *iter)
{
	g_return_val_if_fail (iter != NULL, NULL);

	return iter->user_data;
}

static GtkTreeModelFlags 
snippets_db_get_flags (GtkTreeModel *tree_model)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), (GtkTreeModelFlags)0);

	return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
snippets_db_get_n_columns (GtkTreeModel *tree_model)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), 0);

	return SNIPPETS_DB_MODEL_COL_N;
}

static GType
snippets_db_get_column_type (GtkTreeModel *tree_model,
                             gint index)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), G_TYPE_INVALID);
	g_return_val_if_fail (index >= 0 && index < SNIPPETS_DB_MODEL_COL_N, G_TYPE_INVALID);

	if (index == 0)
		return G_TYPE_OBJECT;
	else
		return G_TYPE_STRING;
}

static gboolean
snippets_db_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter *iter,
                      GtkTreePath *path)
{
	SnippetsDB *snippets_db = NULL;
	gint *indices = NULL, depth = 0, db_count = 0, group_count = 0;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* Get the indices and depth of the path */
	indices = gtk_tree_path_get_indices (path);
	depth   = gtk_tree_path_get_depth (path);
	if (depth > SNIPPETS_DB_MODEL_DEPTH)
		return FALSE;

	db_count = indices[0];
	if (depth == 2)
		group_count = indices[1];

	/* Get the top-level iter with the count being db_count */
	iter_get_first_snippets_db_node (iter, snippets_db);
	if (!iter_nth (iter, db_count))
		return FALSE;

	/* If depth is SNIPPETS_DB_MODEL_DEPTH, get the group_count'th child */
	if (depth == SNIPPETS_DB_MODEL_DEPTH)
		return snippets_db_iter_nth_child (tree_model, iter, iter, group_count);
	
	return TRUE;
}

static GtkTreePath*
snippets_db_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter *iter)
{
	GtkTreePath *path = NULL;
	GtkTreeIter *iter_copy = NULL;
	SnippetsDB *snippets_db = NULL;
	gint count = 0;
	GList *l_iter = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), NULL);
	g_return_val_if_fail (iter != NULL, NULL);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Make a new GtkTreePath object */
	path = gtk_tree_path_new ();

	/* Get the first index */
	l_iter = iter_get_list_node (iter);
	while (l_iter != NULL)
	{
		count ++;
		l_iter = g_list_previous (l_iter);
	}
	gtk_tree_path_append_index (path, count);
	
	/* If it's a snippet node, then it has a parent */
	count = 0;
	if (iter_is_snippet_node (iter))
	{
		iter_copy = gtk_tree_iter_copy (iter);

		snippets_db_iter_parent (tree_model, iter_copy, iter);
		l_iter = iter_get_list_node (iter_copy);
		while (l_iter != NULL)
		{
			count ++;
			l_iter = g_list_previous (l_iter);
		}
		gtk_tree_iter_free (iter);
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
	GObject *cur_object = NULL;
	gchar *cur_string = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column >= 0 && column < SNIPPETS_DB_MODEL_COL_N);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Initializations */
	g_value_init (value, snippets_db_get_column_type (tree_model, column));
	cur_object = iter_get_data (iter);
	
	/* Get the data in the node */
	switch (column)
	{
		case SNIPPETS_DB_MODEL_COL_CUR_OBJECT:
			g_value_set_object (value, cur_object);
			return;

		case SNIPPETS_DB_MODEL_COL_NAME:
			if (ANJUTA_IS_SNIPPET (cur_object))
				cur_string = g_strdup (snippet_get_name (ANJUTA_SNIPPET (cur_object)));
			else
				cur_string = g_strdup (snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (cur_object)));
			g_value_set_string (value, cur_string);
			return;

		case SNIPPETS_DB_MODEL_COL_TRIGGER:
			if (ANJUTA_IS_SNIPPET (cur_object))
				cur_string = g_strdup (snippet_get_trigger_key (ANJUTA_SNIPPET (cur_object)));
			else
				cur_string = g_strdup ("");
			g_value_set_string (value, cur_string);
			return;
			
		case SNIPPETS_DB_MODEL_COL_LANGUAGES:
			if (ANJUTA_IS_SNIPPET (cur_object))
				cur_string = g_strdup (snippet_get_languages_string (ANJUTA_SNIPPET (cur_object)));
			else
				cur_string = g_strdup ("");
			g_value_set_string (value, cur_string);		
	}
}

static gboolean
snippets_db_iter_next (GtkTreeModel *tree_model,
                       GtkTreeIter *iter)
{
	SnippetsDB* snippets_db = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Check if the stamp is correct */
	g_return_val_if_fail (snippets_db->stamp == iter->stamp,
	                      FALSE);

	/* Update the iter and return TRUE if it didn't reached the end*/
	iter->user_data = g_list_next ((GList *)iter->user_data);

	return (iter->user_data != NULL);
}

static gboolean
snippets_db_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter *iter,
                           GtkTreeIter *parent)
{
	return snippets_db_iter_nth_child (tree_model, iter, parent, 0);
}

static gboolean
snippets_db_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
	GList *snippets_list = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	/* If the parent field is NULL then it's the 1st level so it has children */
	if (iter_is_snippets_group_node (iter))
	{
		snippets_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (iter));
		snippets_list = (GList *)snippets_group_get_snippets_list (snippets_group);

		return (g_list_length (snippets_list) != 0);
	}
	else
		return FALSE;
}

static gint
snippets_db_iter_n_children (GtkTreeModel *tree_model,
                             GtkTreeIter *iter)
{
	const GList* snippets_list = NULL;
	SnippetsDB *snippets_db = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), -1);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* If a top-level count is requested */
	if (iter == NULL)
	{
		return (gint)g_list_length (snippets_db->priv->snippets_groups);
	}

	/* If iter points to a SnippetsGroup node */
	if (iter_is_snippets_group_node (iter))
	{
		/* Get the Snippets Group object and assert it */
		snippets_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (iter));
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
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* If it's a top level request */
	if (parent == NULL)
	{
		iter_get_first_snippets_db_node (iter, snippets_db);
		return iter_nth (iter, n);
	}

	if (iter_is_snippets_group_node (parent))
	{
		AnjutaSnippetsGroup *snippets_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (parent));
		GList *snippets_list = (GList *)snippets_group_get_snippets_list (snippets_group);

		iter->user_data2 = parent->user_data;
		iter->user_data  = g_list_first (snippets_list);
		iter->stamp      = parent->stamp;
		
		return iter_nth (iter, n);
	}

	/* If we got to this point, it's a snippet node, so it doesn't have a child*/
	return FALSE;
}

static gboolean
snippets_db_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter *iter,
                         GtkTreeIter *child)
{
	SnippetsDB *snippets_db = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (child != NULL, FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* If it's a snippets group node, it doesn't have a parent */
	if (iter_is_snippets_group_node (child))
		return FALSE;
	
	/* Fill the iter fields */
	iter->user_data  = child->user_data2;
	iter->user_data2 = NULL;
	iter->stamp      = child->stamp;
	
	return TRUE;
}

static GtkTreePath *
get_tree_path_for_snippets_group (SnippetsDB *snippets_db,
                                  AnjutaSnippetsGroup *snippets_group)
{
	GtkTreeIter iter;
	gint index = 0;
	const gchar *group_name = NULL;
	AnjutaSnippetsGroup *cur_group = NULL;
	GtkTreePath *path = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), NULL);
	                      
	group_name = snippets_group_get_name (snippets_group);
	path = gtk_tree_path_new ();
	
	iter_get_first_snippets_db_node (&iter, snippets_db);
	do 
	{
		cur_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (&iter));
		if (!g_strcmp0 (snippets_group_get_name (cur_group), group_name))
		{
			gtk_tree_path_append_index (path, index);		
			return path;
		}
		index ++;
		
	} while (snippets_db_iter_next (GTK_TREE_MODEL (snippets_db), &iter));

	gtk_tree_path_free (path);
	return NULL;
}

static GtkTreePath *
get_tree_path_for_snippet (SnippetsDB *snippets_db,
                           AnjutaSnippet *snippet)
{
	GtkTreePath *path = NULL;
	gint index1 = 0, index2 = 0;
	GtkTreeIter iter1, iter2;
	AnjutaSnippet *cur_snippet = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	path = gtk_tree_path_new ();

	iter_get_first_snippets_db_node (&iter1, snippets_db);
	do
	{
		index2 = 0;
		snippets_db_iter_nth_child (GTK_TREE_MODEL (snippets_db), &iter2, &iter1, 0);
		
		do
		{
			cur_snippet = ANJUTA_SNIPPET (iter_get_data (&iter2));
			g_return_val_if_fail (ANJUTA_IS_SNIPPET (cur_snippet), NULL);

			if (snippet_is_equal (snippet, cur_snippet))
			{
				gtk_tree_path_append_index (path, index1);
				gtk_tree_path_append_index (path, index2);
				return path;
			}
			
			index2 ++;
			
		} while (snippets_db_iter_next (GTK_TREE_MODEL (snippets_db), &iter2));
		
		index1 ++;
		
	} while (snippets_db_iter_next (GTK_TREE_MODEL (snippets_db), &iter1));

	gtk_tree_path_free (path);
	return NULL;
}
