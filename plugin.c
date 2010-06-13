/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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
#include "plugin.h"
#include "snippet.h"
#include <libanjuta/interfaces/ianjuta-snippets-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <gio/gio.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

#define ICON_FILE	"anjuta-snippets-manager.png"
#define PREFERENCES_UI	PACKAGE_DATA_DIR"/glade/snippets-manager-preferences.ui"
#define SNIPPETS_MANAGER_PREFERENCES_ROOT "snippets_preferences_root"

#define GLOBAL_VAR_NEW_NAME   "new_global_var_name"
#define GLOBAL_VAR_NEW_VALUE  "new_global_var_value"

static gpointer parent_class;

typedef struct _GlobalVariablesUpdateData
{
	SnippetsDB *snippets_db;
	GtkTreeView *global_vars_view;
} GlobalVariablesUpdateData;


gboolean
snippet_insert (SnippetsManagerPlugin * plugin, 
                const gchar *keyword)
{
	AnjutaSnippet *requested_snippet = NULL;
	SnippetsManagerPlugin *snippets_manager_plugin = NULL;
	gchar *indent = NULL, *cur_line = NULL, *snippet_default_content = NULL;
	IAnjutaIterable *line_begin = NULL, *cur_pos = NULL;
	gint cur_line_no = -1, i = 0;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_SNIPPETS_MANAGER (plugin),
	                      FALSE);
	snippets_manager_plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (plugin);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (snippets_manager_plugin->cur_editor),
	                      FALSE);

	/* Get the current line */
	cur_line_no = ianjuta_editor_get_lineno (snippets_manager_plugin->cur_editor,
	                                         NULL);
	line_begin = ianjuta_editor_get_line_begin_position (snippets_manager_plugin->cur_editor,
	                                                     cur_line_no, NULL);
	cur_pos = ianjuta_editor_get_position (snippets_manager_plugin->cur_editor, NULL);
	cur_line = ianjuta_editor_get_text (snippets_manager_plugin->cur_editor,
	                                    line_begin, cur_pos, NULL);

	/* Calculate the current indentation */
	indent = g_malloc0 ((strlen (cur_line) + 1) * sizeof (gchar));
	while (cur_line[i] == ' ' || cur_line[i] == '\t')
		i ++;

	/* Get the snippet default content */
	requested_snippet = (AnjutaSnippet *)snippets_db_get_snippet (snippets_manager_plugin->snippets_db,\
	                                                              keyword);
	snippet_default_content = snippet_get_default_content (requested_snippet,
	                                                       G_OBJECT (snippets_manager_plugin->snippets_db), 
	                                                       indent);

	/* Insert the default content into the editor */
	ianjuta_editor_insert (snippets_manager_plugin->cur_editor, 
	                       cur_pos, 
	                       snippet_default_content, 
	                       -1,
	                       NULL);

	g_free (indent);
	g_free (snippet_default_content);

	return TRUE;
}

static void
on_added_current_document (AnjutaPlugin *plugin, 
                           const gchar *name,
                           const GValue *value, 
                           gpointer data)
{
	GObject *cur_editor = NULL;
	SnippetsManagerPlugin *snippets_manager_plugin = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_PLUGIN_SNIPPETS_MANAGER (plugin));
	snippets_manager_plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (plugin);

	/* Get the current document and test if it's an IAnjutaEditor */
	cur_editor = g_value_get_object (value);
	if (!IANJUTA_IS_EDITOR (cur_editor))
		return;
	snippets_manager_plugin->cur_editor = IANJUTA_EDITOR (cur_editor);

}

static void
on_removed_current_document (AnjutaPlugin *plugin,
                             const char *name, 
                             gpointer data)
{
	SnippetsManagerPlugin *snippets_manager_plugin = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_PLUGIN_SNIPPETS_MANAGER (plugin));
	snippets_manager_plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (plugin);

	snippets_manager_plugin->cur_editor = NULL;
}

static gboolean
snippets_manager_activate (AnjutaPlugin * plugin)
{
	SnippetsManagerPlugin *snippets_manager_plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (plugin);

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_SNIPPETS_MANAGER (snippets_manager_plugin),
	                      FALSE);

	/* Link the AnjutaShell to the SnippetsDB */
	snippets_manager_plugin->snippets_db->anjuta_shell = plugin->shell;

	/* Add a watch for the current document */
	snippets_manager_plugin->cur_editor_watch_id = 
	                         anjuta_plugin_add_watch (plugin,
	                                                  IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
	                                                  on_added_current_document,
	                                                  on_removed_current_document,
	                                                  NULL);

	DEBUG_PRINT ("%s", "SnippetsManager: Activating SnippetsManager plugin ...");

	return TRUE;
}

static gboolean
snippets_manager_deactivate (AnjutaPlugin * plugin)
{
	SnippetsManagerPlugin *snippets_manager_plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (plugin);

	DEBUG_PRINT ("%s", "SnippetsManager: Deactivating SnippetsManager plugin ...");

	//snippet_insert (snippets_manager_plugin, "w/e");
	return TRUE;
}

static void
snippets_manager_finalize (GObject * obj)
{
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
snippets_manager_dispose (GObject * obj)
{
	SnippetsManagerPlugin *snippets_manager = ANJUTA_PLUGIN_SNIPPETS_MANAGER (obj);
	
	if (snippets_manager->snippets_db != NULL)
	{
		g_object_unref (snippets_manager->snippets_db);
		snippets_manager->snippets_db = NULL;
	}
	
	if (snippets_manager->snippet_interpreter != NULL)
	{
		g_object_unref (snippets_manager->snippet_interpreter);
		snippets_manager->snippet_interpreter = NULL;
	}
	
	if(snippets_manager->snippet_browser != NULL)
	{
		g_object_unref (snippets_manager->snippet_browser);
		snippets_manager->snippet_browser = NULL;
	}
	
	if(snippets_manager->snippet_editor != NULL)
	{
		g_object_unref (snippets_manager->snippet_editor);
		snippets_manager->snippet_editor = NULL;
	}
		
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
snippets_manager_plugin_instance_init (GObject * obj)
{
	SnippetsManagerPlugin *snippets_manager = ANJUTA_PLUGIN_SNIPPETS_MANAGER (obj);
	
	snippets_manager->overwrite_on_conflict = FALSE;
	snippets_manager->show_only_document_language_snippets = FALSE;

	snippets_manager->cur_editor = NULL;
	snippets_manager->cur_editor_watch_id = -1;

	/* TODO */
	snippets_manager->snippets_db = snippets_db_new ();
	snippets_manager->snippet_interpreter = NULL;
	snippets_manager->snippet_browser = NULL;
	snippets_manager->snippet_editor = NULL;

}

static void
snippets_manager_plugin_class_init (GObjectClass * klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = snippets_manager_activate;
	plugin_class->deactivate = snippets_manager_deactivate;
	klass->dispose = snippets_manager_dispose;
	klass->finalize = snippets_manager_finalize;
}



/* IAnjutaSnippetsManager interface */

static gboolean 
isnippets_manager_iface_insert (IAnjutaSnippetsManager* snippets_manager, const gchar* key, GError** err)
{
	SnippetsManagerPlugin* plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (snippets_manager);
	snippet_insert(plugin, key);
	return TRUE;
}

static void
isnippets_manager_iface_init (IAnjutaSnippetsManagerIface *iface)
{
	iface->insert = isnippets_manager_iface_insert;
}



/* IAnjutaPreferences interface */

static void
on_global_vars_name_changed (GtkCellRendererText *cell,
                             gchar *path_string,
                             gchar *new_text,
                             gpointer user_data)
{
	GtkTreeModel *global_vars_model = NULL;
	SnippetsDB *snippets_db = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gchar *name = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (user_data));
	snippets_db = ANJUTA_SNIPPETS_DB (user_data);
	global_vars_model = snippets_db_get_global_vars_model (snippets_db);
	g_return_if_fail (GTK_IS_TREE_MODEL (global_vars_model));
	
	/* Get the iter */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (global_vars_model, &iter, path);

	/* Get the current type and change it */
	gtk_tree_model_get (global_vars_model, &iter,
	                    GLOBAL_VARS_MODEL_COL_NAME, &name,
	                    -1);
	snippets_db_set_global_variable_name (snippets_db, 
	                                      name, 
	                                      new_text);
	g_free (name);
}

static void
global_vars_view_name_data_func (GtkTreeViewColumn *col,
                                 GtkCellRenderer *cell,
                                 GtkTreeModel *global_vars_model,
                                 GtkTreeIter *iter,
                                 gpointer user_data)
{
	gchar *name = NULL;
	gboolean is_internal = FALSE;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (cell));
	
	/* Get the name */
	gtk_tree_model_get (global_vars_model, iter,
	                    GLOBAL_VARS_MODEL_COL_NAME, &name,
	                    -1);

	/* Check if it's internal */
	gtk_tree_model_get (global_vars_model, iter,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
	                    -1);
	if (is_internal)
	{
		gchar *temp = NULL;
		temp = g_strconcat ("<b>", name, "</b> <i>(Internal)</i>", NULL);
		g_free (name);
		name = temp;
		g_object_set (cell, "sensitive", FALSE, NULL);
		g_object_set (cell, "editable", FALSE, NULL);
	}
	else
	{
		gchar *temp = NULL;
		temp = g_strconcat ("<b>", name, "</b>", NULL);
		g_free (name);
		name = temp;
		g_object_set (cell, "sensitive", TRUE, NULL);
		g_object_set (cell, "editable", TRUE, NULL);
	}
	
	g_object_set (cell, "markup", name, NULL);
	g_free (name);
}

static void
on_global_vars_type_toggled (GtkCellRendererToggle *cell,
                             gchar *path_string,
                             gpointer user_data)
{
	GtkTreeModel *global_vars_model = NULL;
	SnippetsDB *snippets_db = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gboolean is_command = FALSE;
	gchar *name = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (user_data));
	snippets_db = ANJUTA_SNIPPETS_DB (user_data);
	global_vars_model = snippets_db_get_global_vars_model (snippets_db);
	g_return_if_fail (GTK_IS_TREE_MODEL (global_vars_model));
	
	/* Get the iter */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (global_vars_model, &iter, path);

	/* Get the current type and change it */
	gtk_tree_model_get (global_vars_model, &iter,
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, &is_command,
	                    GLOBAL_VARS_MODEL_COL_NAME, &name,
	                    -1);
	snippets_db_set_global_variable_type (snippets_db, 
	                                      name, 
	                                      (is_command) ? FALSE : TRUE);
	g_free (name);
	
}

static void
global_vars_view_type_data_func (GtkTreeViewColumn *col,
                                 GtkCellRenderer *cell,
                                 GtkTreeModel *global_vars_model,
                                 GtkTreeIter *iter,
                                 gpointer user_data)
{
	gboolean is_command = FALSE, is_internal = TRUE;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TOGGLE (cell));


	/* Check if it's internal */
	gtk_tree_model_get (global_vars_model, iter,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
	                    -1);
	if (is_internal)
	{
		g_object_set (cell, "sensitive", FALSE, NULL);
		gtk_cell_renderer_toggle_set_activatable (GTK_CELL_RENDERER_TOGGLE (cell), FALSE);
		gtk_cell_renderer_toggle_set_active (GTK_CELL_RENDERER_TOGGLE (cell), FALSE);
	}
	else
	{
		gtk_tree_model_get (global_vars_model, iter,
		                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, &is_command,
		                    -1);
		g_object_set (cell, "sensitive", TRUE, NULL);
		gtk_cell_renderer_toggle_set_activatable (GTK_CELL_RENDERER_TOGGLE (cell), TRUE);
		gtk_cell_renderer_toggle_set_active (GTK_CELL_RENDERER_TOGGLE (cell), is_command);
	}
}

static void
on_global_vars_text_changed (GtkCellRendererText *cell,
                             gchar *path_string,
                             gchar *new_text,
                             gpointer user_data)
{
	GtkTreeModel *global_vars_model = NULL;
	SnippetsDB *snippets_db = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gchar *name = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (user_data));
	snippets_db = ANJUTA_SNIPPETS_DB (user_data);
	global_vars_model = snippets_db_get_global_vars_model (snippets_db);
	g_return_if_fail (GTK_IS_TREE_MODEL (global_vars_model));
	
	/* Get the iter */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (global_vars_model, &iter, path);

	/* Get the current type and change it */
	gtk_tree_model_get (global_vars_model, &iter,
	                    GLOBAL_VARS_MODEL_COL_NAME, &name,
	                    -1);
	snippets_db_set_global_variable_value (snippets_db, 
	                                       name, 
	                                       new_text);
	g_free (name);
}

static void
global_vars_view_text_data_func (GtkTreeViewColumn *col,
                                 GtkCellRenderer *cell,
                                 GtkTreeModel *global_vars_model,
                                 GtkTreeIter *iter,
                                 gpointer user_data)
{
	gchar *name = NULL, *text = NULL;
	SnippetsDB *snippets_db = NULL;
	gboolean is_internal = FALSE;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (cell) &&
	                  ANJUTA_IS_SNIPPETS_DB (user_data));
	snippets_db = ANJUTA_SNIPPETS_DB (user_data);
	
	/* Get the name */
	gtk_tree_model_get (global_vars_model, iter,
	                    GLOBAL_VARS_MODEL_COL_NAME, &name,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
	                    -1);

	if (is_internal)
	{
		g_object_set (cell, "editable", FALSE, NULL);
	}
	else
	{
		g_object_set (cell, "editable", TRUE, NULL);
	}

	text = snippets_db_get_global_variable_text (snippets_db, name);
	
	g_object_set (cell, "text", text, NULL);
	g_free (name);
	g_free (text);	
}

static void
global_vars_view_value_data_func (GtkTreeViewColumn *col,
                                 GtkCellRenderer *cell,
                                 GtkTreeModel *global_vars_model,
                                 GtkTreeIter *iter,
                                 gpointer user_data)
{
	gchar *name = NULL, *value = NULL;
	SnippetsDB *snippets_db = NULL;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (cell) &&
	                  ANJUTA_IS_SNIPPETS_DB (user_data));
	snippets_db = ANJUTA_SNIPPETS_DB (user_data);
	
	/* Get the name */
	gtk_tree_model_get (global_vars_model, iter,
	                    GLOBAL_VARS_MODEL_COL_NAME, &name,
	                    -1);

	value = snippets_db_get_global_variable (snippets_db, name);
	
	g_object_set (cell, "text", value, NULL);
	g_free (name);	
}

static void
set_up_global_variables_view (SnippetsDB *snippets_db, 
                              GtkTreeView *global_vars_view)
{
	GtkCellRenderer *cell = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeModel *global_vars_model = NULL;

	/* Assertions */
	global_vars_model = snippets_db_get_global_vars_model (snippets_db);
	g_return_if_fail (GTK_IS_TREE_MODEL (global_vars_model) &&
	                  GTK_IS_TREE_VIEW (global_vars_view));

	/* Set up the model */
	gtk_tree_view_set_model (global_vars_view,
	                         global_vars_model);
	
	/* Set up the name cell */
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Name");
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, cell,
	                                         global_vars_view_name_data_func,
	                                         NULL, NULL);
	gtk_tree_view_append_column (global_vars_view, col);
	g_signal_connect (GTK_OBJECT (cell), 
	                  "edited",
	                  GTK_SIGNAL_FUNC (on_global_vars_name_changed),
	                  snippets_db);
	
	/* Set up the type cell */
	cell = gtk_cell_renderer_toggle_new ();
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Command?");
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, cell,
	                                         global_vars_view_type_data_func,
	                                         NULL, NULL);
	gtk_tree_view_append_column (global_vars_view, col);
	g_signal_connect (GTK_OBJECT (cell), 
	                  "toggled", 
	                  GTK_SIGNAL_FUNC (on_global_vars_type_toggled), 
	                  snippets_db);

	/* Set up the text cell */
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Variable text");
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, cell,
	                                         global_vars_view_text_data_func,
	                                         snippets_db, NULL);
	gtk_tree_view_append_column (global_vars_view, col);
	g_signal_connect (GTK_OBJECT (cell), 
	                  "edited",
	                  GTK_SIGNAL_FUNC (on_global_vars_text_changed),
	                  snippets_db);

	/* Set up the instant value cell */
	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell, "editable", FALSE, NULL);
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Instant value");
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, cell,
	                                         global_vars_view_value_data_func,
	                                         snippets_db, NULL);
	gtk_tree_view_append_column (global_vars_view, col);

}

static void
on_add_variable_b_clicked (GtkButton *button,
                           gpointer user_data)
{
	GlobalVariablesUpdateData *update_data = (GlobalVariablesUpdateData *)user_data;
	GtkTreeView *global_vars_view = NULL;
	GtkTreeModel *global_vars_model = NULL;
	SnippetsDB *snippets_db = NULL;
	GtkTreeIter iter;
	gboolean iter_has_next = TRUE;
	gchar *name = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (update_data->snippets_db) &&
	                  GTK_IS_TREE_VIEW (update_data->global_vars_view));
	snippets_db = ANJUTA_SNIPPETS_DB (update_data->snippets_db);
	global_vars_view = GTK_TREE_VIEW (update_data->global_vars_view);
	global_vars_model = snippets_db_get_global_vars_model (snippets_db);
	
	/* Insert it into the SnippetsDB */
	snippets_db_add_global_variable (snippets_db,
	                                 GLOBAL_VAR_NEW_NAME,
	                                 GLOBAL_VAR_NEW_VALUE,
	                                 FALSE, FALSE);

	/* Get to the new inserted variable */
	iter_has_next = gtk_tree_model_get_iter_first (global_vars_model, &iter);
	while (iter_has_next)
	{
		gtk_tree_model_get (global_vars_model, &iter,
		                    GLOBAL_VARS_MODEL_COL_NAME, &name,
		                    -1);
		if (!g_strcmp0 (name, GLOBAL_VAR_NEW_NAME))
		{
			GtkTreePath *path = gtk_tree_model_get_path (global_vars_model, &iter);

			gtk_tree_view_set_cursor (global_vars_view,
			                          path,
			                          gtk_tree_view_get_column (global_vars_view, 0),
			                          TRUE);

			g_free (name);
			return;
		}

		g_free (name);
		iter_has_next = gtk_tree_model_iter_next (global_vars_model, &iter);
	}
}

static void
on_delete_variable_b_clicked (GtkButton *button,
                              gpointer user_data)
{
	GlobalVariablesUpdateData *update_data = (GlobalVariablesUpdateData *)user_data;
	GtkTreeView *global_vars_view = NULL;
	GtkTreeModel *global_vars_model = NULL;
	SnippetsDB *snippets_db = NULL;
	GtkTreeSelection *global_vars_selection = NULL;
	gchar *name = NULL;
	gboolean has_selection = FALSE;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (update_data->snippets_db) &&
	                  GTK_IS_TREE_VIEW (update_data->global_vars_view));
	snippets_db = ANJUTA_SNIPPETS_DB (update_data->snippets_db);
	global_vars_view = GTK_TREE_VIEW (update_data->global_vars_view);
	global_vars_model = snippets_db_get_global_vars_model (snippets_db);
	global_vars_selection = gtk_tree_view_get_selection (global_vars_view);

	/* Get the selected iter */
	has_selection = gtk_tree_selection_get_selected (global_vars_selection, 
	                                                 &global_vars_model, 
	                                                 &iter);
	                                                 
	/* If there is a selection delete the selected item */
	if (has_selection)
	{
		gtk_tree_model_get (global_vars_model, &iter,
		                    GLOBAL_VARS_MODEL_COL_NAME, &name,
		                    -1);
		snippets_db_remove_global_variable (snippets_db, name);
		g_free (name);
	
	}
}

static void
ipreferences_merge (IAnjutaPreferences* ipref,
					AnjutaPreferences* prefs,
					GError** e)
{
	GError* error = NULL;
	GtkBuilder* bxml = gtk_builder_new ();
	GtkTreeView *global_vars_view = NULL;
	GtkButton *add_variable_b = NULL, *delete_variable_b = NULL, *reset_default_b = NULL;
	GtkCheckButton *overwrite_cb = NULL, *show_only_lang_cb = NULL; 
	GtkFileChooserButton *default_folder_b = NULL;
	SnippetsManagerPlugin *snippets_manager_plugin = NULL;
	GlobalVariablesUpdateData *global_vars_update_data = NULL;
	
	/* Assertions */
	snippets_manager_plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (ipref);
	g_return_if_fail (ANJUTA_IS_PLUGIN_SNIPPETS_MANAGER (snippets_manager_plugin));
	
	if (!gtk_builder_add_from_file (bxml, PREFERENCES_UI, &error))
	{
		g_warning ("Couldn't load preferences ui file: %s", error->message);
		g_error_free (error);
	}
	anjuta_preferences_add_from_builder (prefs, bxml, SNIPPETS_MANAGER_PREFERENCES_ROOT, _("Snippets Manager"),
								 ICON_FILE);

	/* Get the Gtk objects */
	global_vars_view  = GTK_TREE_VIEW (gtk_builder_get_object (bxml, "global_vars_view"));
	add_variable_b    = GTK_BUTTON (gtk_builder_get_object (bxml, "add_var_button"));
	delete_variable_b = GTK_BUTTON (gtk_builder_get_object (bxml, "delete_var_button"));
	overwrite_cb      = GTK_CHECK_BUTTON (gtk_builder_get_object (bxml, "overwrite_cb"));
	show_only_lang_cb = GTK_CHECK_BUTTON (gtk_builder_get_object (bxml, "show_only_lang_cb"));
	reset_default_b   = GTK_BUTTON (gtk_builder_get_object (bxml, "reset_default_b"));
	default_folder_b  = GTK_FILE_CHOOSER_BUTTON (gtk_builder_get_object (bxml, "default_folder_b"));
	g_return_if_fail (GTK_IS_TREE_VIEW (global_vars_view) &&
	                  GTK_IS_BUTTON (add_variable_b) &&
	                  GTK_IS_BUTTON (delete_variable_b) &&
	                  GTK_IS_CHECK_BUTTON (overwrite_cb) &&
	                  GTK_IS_CHECK_BUTTON (show_only_lang_cb) &&
	                  GTK_IS_BUTTON (reset_default_b) &&
	                  GTK_IS_FILE_CHOOSER_BUTTON (default_folder_b));

	/* Set up the Global Variables GtkTreeView */
	set_up_global_variables_view (snippets_manager_plugin->snippets_db, global_vars_view);

	/* Connect the addition/deletion buttons */
	global_vars_update_data = g_malloc (sizeof (GlobalVariablesUpdateData));
	global_vars_update_data->snippets_db = snippets_manager_plugin->snippets_db;
	global_vars_update_data->global_vars_view = global_vars_view;
	g_signal_connect (GTK_OBJECT (add_variable_b),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_add_variable_b_clicked),
	                  global_vars_update_data);
	g_signal_connect (GTK_OBJECT (delete_variable_b),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_delete_variable_b_clicked),
	                  global_vars_update_data);
	
	g_object_unref (bxml);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref,
					  AnjutaPreferences* prefs,
					  GError** e)
{
	anjuta_preferences_remove_page (prefs, _("Snippets Manager"));
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}


ANJUTA_PLUGIN_BEGIN (SnippetsManagerPlugin, snippets_manager_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (isnippets_manager, IANJUTA_TYPE_SNIPPETS_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END

ANJUTA_SIMPLE_PLUGIN (SnippetsManagerPlugin, snippets_manager_plugin);
