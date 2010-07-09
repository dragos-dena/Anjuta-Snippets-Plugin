/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-editor.c
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

#include "snippets-editor.h"
#include "snippet-variables-store.h"

#define EDITOR_UI      PACKAGE_DATA_DIR"/glade/snippets-editor.ui"

#define ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_EDITOR, SnippetsEditorPrivate))

#define LOCAL_TYPE_STR          "Local"
#define GLOBAL_TYPE_STR         "Global"
#define GLOBAL_UNDEFINED_MARKUP "Global <i>(Undefined)</i>"
#define UNDEFINED_BG_COLOR      "#ba0000"
#define UNDEFINED_FG_COLOR      "#ffffff"

#define NAME_COL_TITLE     _("Name")
#define TYPE_COL_TITLE     _("Type")
#define DEFAULT_COL_TITLE  _("Default value")
#define INSTANT_COL_TITLE  _("Instant value")


struct _SnippetsEditorPrivate
{
	SnippetsDB *snippets_db;
	AnjutaSnippet *snippet;
	AnjutaSnippet *backup_snippet;

	/* Snippet Content editor widgets */
	GtkTextView *content_text_view; /* TODO - this should be changed later with GtkSourceView */
	GtkToggleButton *preview_button;

	/* Snippet properties widgets */
	GtkEntry *name_entry;
	GtkEntry *trigger_entry;
	GtkComboBox *languages_combo_box;
	GtkComboBox *snippets_group_combo_box;
	GtkImage *languages_warning;
	GtkImage *group_warning;

	/* Variables widgets */
	GtkTreeView *variables_view;
	GtkButton *variable_add_button;
	GtkButton *variable_remove_button;
	GtkButton *variable_insert_button;

	/* Variables tree model */
	SnippetVarsStore *vars_store;
	GtkTreeModel *vars_store_sorted;

	/* Variables view cell renderers */
	GtkCellRenderer *name_text_cell;
	GtkCellRenderer *type_text_cell;
	GtkCellRenderer *type_pixbuf_cell;
	GtkCellRenderer *default_text_cell;
	GtkCellRenderer *instant_text_cell;
	
	/* Editor widgets */
	GtkButton *save_button;
	GtkButton *close_button;
	GtkAlignment *editor_alignment;

};


G_DEFINE_TYPE (SnippetsEditor, snippets_editor, GTK_TYPE_HBOX);


static void
snippets_editor_dispose (GObject *object)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (object));

	/* TODO - destroy the snippet if one */

	G_OBJECT_CLASS (snippets_editor_parent_class)->dispose (G_OBJECT (object));
}

static void
snippets_editor_class_init (SnippetsEditorClass* klass)
{
	GObjectClass *g_object_class = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR_CLASS (klass));

	g_object_class = G_OBJECT_CLASS (klass);
	g_object_class->dispose = snippets_editor_dispose;

	/* The SnippetsEditor saved the snippet. Basically, the SnippetsBrowser should
	   focus on the row where the snippet was saved. This is needed because when the
	   snippet is saved, the old one is deleted and the new one is inserted. The given
	   object is the newly inserted snippet. */
	g_signal_new ("snippet-saved",
	              ANJUTA_TYPE_SNIPPETS_EDITOR,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsEditorClass, snippet_saved),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__OBJECT,
	              G_TYPE_NONE,
	              1,
	              G_TYPE_OBJECT,
	              NULL);

	g_signal_new ("close-request",
	              ANJUTA_TYPE_SNIPPETS_EDITOR,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsEditorClass, close_request),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE,
	              0,
	              NULL);
	
	g_type_class_add_private (klass, sizeof (SnippetsEditorPrivate));
}

static void
snippets_editor_init (SnippetsEditor* snippets_editor)
{
	SnippetsEditorPrivate* priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	snippets_editor->priv = priv;

	/* Initialize the private field */
	priv->snippets_db = NULL;
	priv->snippet = NULL;	
	priv->backup_snippet = NULL;

	priv->content_text_view = NULL;
	priv->preview_button = NULL;

	priv->name_entry = NULL;
	priv->trigger_entry = NULL;
	priv->languages_combo_box = NULL;
	priv->snippets_group_combo_box = NULL;
	priv->languages_warning = NULL;
	priv->group_warning = NULL;
	
	priv->variables_view = NULL;
	priv->variable_add_button = NULL;
	priv->variable_remove_button = NULL;
	priv->variable_insert_button = NULL;

	priv->vars_store = NULL;
	priv->vars_store_sorted = NULL;

	priv->name_text_cell = NULL;
	priv->type_text_cell = NULL;
	priv->type_pixbuf_cell = NULL;
	priv->default_text_cell = NULL;
	priv->instant_text_cell = NULL;

	priv->save_button = NULL;
	priv->close_button = NULL;
	priv->editor_alignment = NULL;
}

/* Handlers */
static void  on_preview_button_toggled       (GtkToggleButton *preview_button,
                                              gpointer user_data);
static void  on_save_button_clicked          (GtkButton *save_button,
                                              gpointer user_data);
static void  on_close_button_clicked         (GtkButton *close_button,
                                              gpointer user_data);
static void  on_name_text_cell_edited        (GtkCellRendererText *cell,
                                              gchar *path_string,
                                              gchar *new_string,
                                              gpointer user_data);
static void  on_variables_view_row_activated (GtkTreeView *tree_view,
                                              GtkTreePath *path,
                                              GtkTreeViewColumn *col,
                                              gpointer user_data);
static void  on_default_text_cell_edited     (GtkCellRendererText *cell,
                                              gchar *path_string,
                                              gchar *new_string,
                                              gpointer user_data);

static void
load_snippets_editor_ui (SnippetsEditor *snippets_editor)
{
	GtkBuilder *bxml = NULL;
	SnippetsEditorPrivate *priv = NULL;
	GError *error = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);
	
	/* Load the UI file */
	bxml = gtk_builder_new ();
	if (!gtk_builder_add_from_file (bxml, EDITOR_UI, &error))
	{
		g_warning ("Couldn't load editor ui file: %s", error->message);
		g_error_free (error);
	}

	/* Edit content widgets */
	priv->content_text_view = GTK_TEXT_VIEW (gtk_builder_get_object (bxml, "content_text_view"));
	priv->preview_button = GTK_TOGGLE_BUTTON (gtk_builder_get_object (bxml, "preview_button"));
	g_return_if_fail (GTK_IS_TEXT_VIEW (priv->content_text_view));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (priv->preview_button));

	/* Edit properties widgets */
	priv->name_entry = GTK_ENTRY (gtk_builder_get_object (bxml, "name_entry"));
	priv->trigger_entry = GTK_ENTRY (gtk_builder_get_object (bxml, "trigger_entry"));
	priv->languages_combo_box = GTK_COMBO_BOX (gtk_builder_get_object (bxml, "languages_combo_box"));
	priv->snippets_group_combo_box = GTK_COMBO_BOX (gtk_builder_get_object (bxml, "snippets_group_combo_box"));
	priv->languages_warning = GTK_IMAGE (gtk_builder_get_object (bxml, "languages_warning"));
	priv->group_warning = GTK_IMAGE (gtk_builder_get_object (bxml, "group_warning"));
	g_return_if_fail (GTK_IS_ENTRY (priv->name_entry));
	g_return_if_fail (GTK_IS_ENTRY (priv->trigger_entry));
	g_return_if_fail (GTK_IS_COMBO_BOX (priv->languages_combo_box));
	g_return_if_fail (GTK_IS_COMBO_BOX (priv->snippets_group_combo_box));
	g_return_if_fail (GTK_IS_IMAGE (priv->languages_warning));
	g_return_if_fail (GTK_IS_IMAGE (priv->group_warning));

	/* Edit variables widgets */
	priv->variables_view = GTK_TREE_VIEW (gtk_builder_get_object (bxml, "variables_view"));
	priv->variable_add_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_add_button"));
	priv->variable_remove_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_remove_button"));
	priv->variable_insert_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_insert_button"));
	g_return_if_fail (GTK_IS_TREE_VIEW (priv->variables_view));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_add_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_remove_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_insert_button));

	/* General widgets */
	priv->save_button = GTK_BUTTON (gtk_builder_get_object (bxml, "save_button"));
	priv->close_button = GTK_BUTTON (gtk_builder_get_object (bxml, "close_button"));
	priv->editor_alignment = GTK_ALIGNMENT (gtk_builder_get_object (bxml, "editor_alignment"));
	g_return_if_fail (GTK_IS_BUTTON (priv->save_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->close_button));
	g_return_if_fail (GTK_IS_ALIGNMENT (priv->editor_alignment));

	/* Add the gtk_alignment as the child of the snippets editor */
	gtk_widget_unparent (GTK_WIDGET (priv->editor_alignment));
	gtk_box_pack_start (GTK_BOX (snippets_editor),
	                    GTK_WIDGET (priv->editor_alignment),
	                    TRUE,
	                    TRUE,
	                    0);

	g_object_unref (bxml);
}

/* Variables View cell data functions and the sort function	*/

static gint
compare_var_type (SnippetVariableType type1, SnippetVariableType type2)
{
	/* Local goes before global */
	if (type1 == type2)
		return 0;
	if (type1 == SNIPPET_VAR_TYPE_LOCAL)
		return -1;
	return 1;
}

static gint
compare_var_in_snippet (gboolean in_snippet1, gboolean in_snippet2)
{
	/* Those that are in snippet go before those that aren't */
	if ((in_snippet1 && in_snippet2) || (!in_snippet1 && !in_snippet2))
		return 0;
	if (in_snippet1)
		return -1;
	return 1;
}

static gint
vars_store_sort_func (GtkTreeModel *vars_store,
                      GtkTreeIter *iter1,
                      GtkTreeIter *iter2,
                      gpointer user_data)
{
	gboolean in_snippet1 = FALSE, in_snippet2 = FALSE;
	gchar *name1 = NULL, *name2 = NULL;
	SnippetVariableType type1, type2;
	gint compare_value = 0;
	
	/* Get the values from the model */
	gtk_tree_model_get (vars_store, iter1,
	                    VARS_STORE_COL_NAME, &name1,
	                    VARS_STORE_COL_TYPE, &type1,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet1,
	                    -1);
	gtk_tree_model_get (vars_store, iter2,
	                    VARS_STORE_COL_NAME, &name2,
	                    VARS_STORE_COL_TYPE, &type2,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet2,
	                    -1);

	/* We first check if both variables are in the snippet */
	compare_value = compare_var_in_snippet (in_snippet1, in_snippet2);

	/* If there are both in snippet we compare by type */
	if (!compare_value && in_snippet1)
		compare_value = compare_var_type (type1, type2);

	/* If we didn't got a compare_value until this point, we compare by name */
	if (!compare_value)
		compare_value = g_strcmp0 (name1, name2);

	g_free (name1);
	g_free (name2);
	
	return compare_value;
}

static void
set_text_cell_colors (GtkCellRenderer *cell, 
                      SnippetVariableType type,
                      gboolean undefined)
{
	if (undefined && type == SNIPPET_VAR_TYPE_GLOBAL)
	{
		g_object_set (cell, "background", UNDEFINED_BG_COLOR, NULL);
		g_object_set (cell, "foreground", UNDEFINED_FG_COLOR, NULL);
	}
	else
	{
		g_object_set (cell, "background-set", FALSE, NULL);
		g_object_set (cell, "foreground-set", FALSE, NULL);
	}
}

static void
variables_view_name_text_data_func (GtkTreeViewColumn *column,
                                    GtkCellRenderer *cell,
                                    GtkTreeModel *tree_model,
                                    GtkTreeIter *iter,
                                    gpointer user_data)
{
	gboolean in_snippet = FALSE, undefined = FALSE;
	gchar *name = NULL, *name_with_markup = NULL;
	SnippetVariableType type;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_NAME, &name,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	if (in_snippet)
		name_with_markup = g_strconcat ("<b>", name, "</b>", NULL);
	else
		name_with_markup = g_strdup (name);

	g_object_set (cell, "editable", in_snippet, NULL);
	g_object_set (cell, "sensitive", in_snippet, NULL);
	g_object_set (cell, "markup", name_with_markup, NULL);

	set_text_cell_colors (cell, type, undefined);

	g_free (name);
	g_free (name_with_markup);
}

static void
variables_view_type_text_data_func (GtkTreeViewColumn *column,
                                    GtkCellRenderer *cell,
                                    GtkTreeModel *tree_model,
                                    GtkTreeIter *iter,
                                    gpointer user_data)
{
	SnippetVariableType type;
	gboolean in_snippet = FALSE;
	gboolean undefined = FALSE;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_TYPE, &type,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    -1);

	if (type == SNIPPET_VAR_TYPE_LOCAL)
		g_object_set (cell, "text", LOCAL_TYPE_STR, NULL);
	else
	if (type == SNIPPET_VAR_TYPE_GLOBAL && !undefined)
		g_object_set (cell, "text", GLOBAL_TYPE_STR, NULL);
	else
	if (type == SNIPPET_VAR_TYPE_GLOBAL && undefined)
		g_object_set (cell, "markup", GLOBAL_UNDEFINED_MARKUP, NULL);
	else
		g_return_if_reached ();

	set_text_cell_colors (cell, type, undefined);

	g_object_set (cell, "sensitive", in_snippet, NULL);
}

static void
variables_view_type_pixbuf_data_func (GtkTreeViewColumn *column,
                                      GtkCellRenderer *cell,
                                      GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      gpointer user_data)
{
	SnippetVariableType type;
	gboolean undefined = FALSE;

	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_TYPE, &type,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    -1);

	if (type == SNIPPET_VAR_TYPE_GLOBAL && undefined)
		g_object_set (cell, "visible", TRUE, NULL);
	else
		g_object_set (cell, "visible", FALSE, NULL);

	if (undefined && type == SNIPPET_VAR_TYPE_GLOBAL)
		g_object_set (cell, "cell-background", UNDEFINED_BG_COLOR, NULL);
	else
		g_object_set (cell, "cell-background-set", FALSE, NULL);
	
	g_object_set (cell, "stock-id", GTK_STOCK_DIALOG_WARNING, NULL);
}

static void
variables_view_default_text_data_func (GtkTreeViewColumn *column,
                                       GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       gpointer user_data)
{
	gchar *default_value = NULL;
	gboolean in_snippet = FALSE, undefined = FALSE;
	SnippetVariableType type;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_DEFAULT_VALUE, &default_value,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	g_object_set (cell, "text", default_value, NULL);
	g_object_set (cell, "editable", in_snippet, NULL);
	
	set_text_cell_colors (cell, type, undefined);

	g_free (default_value);
}

static void
variables_view_instant_text_data_func (GtkTreeViewColumn *column,
                                       GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       gpointer user_data)
{
	gboolean undefined = FALSE;
	SnippetVariableType type;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	set_text_cell_colors (cell, type, undefined);

}

static void
init_variables_view (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeViewColumn *col = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Initialize the sorted model */
	priv->vars_store_sorted = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (priv->vars_store));
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (priv->vars_store_sorted),
	                                         vars_store_sort_func,
	                                         NULL, NULL);
	gtk_tree_view_set_model (priv->variables_view, GTK_TREE_MODEL (priv->vars_store_sorted));

	/* Column 1 - Name */
	col = gtk_tree_view_column_new ();
	priv->name_text_cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_title (col, NAME_COL_TITLE);
	gtk_tree_view_column_pack_start (col, priv->name_text_cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, priv->name_text_cell,
	                                         variables_view_name_text_data_func,
	                                         snippets_editor, NULL);
	g_object_set (G_OBJECT (col), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);

	/* Column 2 - Type */
	col = gtk_tree_view_column_new ();
	priv->type_text_cell = gtk_cell_renderer_text_new ();
	priv->type_pixbuf_cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_set_title (col, TYPE_COL_TITLE);
	gtk_tree_view_column_pack_start (col, priv->type_text_cell, FALSE);
	gtk_tree_view_column_pack_end (col, priv->type_pixbuf_cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, priv->type_text_cell,
	                                         variables_view_type_text_data_func,
	                                         snippets_editor, NULL);
	gtk_tree_view_column_set_cell_data_func (col, priv->type_pixbuf_cell,
	                                         variables_view_type_pixbuf_data_func,
	                                         snippets_editor, NULL);
	g_object_set (G_OBJECT (col), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);
	
	/* Column 3 - Default Value (just for those variables that are in the snippet) */
	col = gtk_tree_view_column_new ();
	priv->default_text_cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_title (col, DEFAULT_COL_TITLE);
	gtk_tree_view_column_pack_start (col, priv->default_text_cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, priv->default_text_cell,
	                                         variables_view_default_text_data_func,
	                                         snippets_editor, NULL);
	g_object_set (G_OBJECT (col), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);

	/* Column 4 - Instant value */
	priv->instant_text_cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (INSTANT_COL_TITLE,
	                                                priv->instant_text_cell,
	                                                "text", VARS_STORE_COL_INSTANT_VALUE,
	                                                NULL);
	gtk_tree_view_column_set_cell_data_func (col, priv->instant_text_cell,
	                                         variables_view_instant_text_data_func,
	                                         snippets_editor, NULL);
	g_object_set (G_OBJECT (col), "resizable", TRUE, NULL);
	g_object_set (G_OBJECT (priv->instant_text_cell), "editable", FALSE, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);

}

static void
init_editor_handlers (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	g_signal_connect (GTK_OBJECT (priv->preview_button),
	                  "toggled",
	                  GTK_SIGNAL_FUNC (on_preview_button_toggled),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->save_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_save_button_clicked),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->close_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_close_button_clicked),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->name_text_cell),
	                  "edited",
	                  G_CALLBACK (on_name_text_cell_edited),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->variables_view),
	                  "row-activated",
	                  GTK_SIGNAL_FUNC (on_variables_view_row_activated),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->default_text_cell),
	                  "edited",
	                  G_CALLBACK (on_default_text_cell_edited),
	                  snippets_editor);
	
}

SnippetsEditor *
snippets_editor_new (SnippetsDB *snippets_db)
{
	SnippetsEditor *snippets_editor = 
	          ANJUTA_SNIPPETS_EDITOR (g_object_new (snippets_editor_get_type (), NULL));
	SnippetsEditorPrivate *priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), snippets_editor);

	priv->snippets_db = snippets_db;

	/* Load the variables tree model */
	priv->vars_store = snippet_vars_store_new ();

	/* Load the UI for snippets-editor.ui */
	load_snippets_editor_ui (snippets_editor);

	/* Initialize the variables tree view */
	init_variables_view (snippets_editor);

	/* TODO - connect the handlers */
	init_editor_handlers (snippets_editor);


	
	return snippets_editor;
}

static void
load_content_to_editor (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	gchar *text = NULL;
	GtkTextBuffer *content_buffer = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* If we don't have a snippet loaded we don't do anything */
	if (!ANJUTA_IS_SNIPPET (priv->snippet))
		return;

	if (gtk_toggle_button_get_active (priv->preview_button))
	{
		text = snippet_get_default_content (priv->snippet,
		                                    G_OBJECT (priv->snippets_db),
		                                    "");
	}
	else
	{
		text = g_strdup (snippet_get_content (priv->snippet));
	}
	
	content_buffer = gtk_text_view_get_buffer (priv->content_text_view);
	gtk_text_buffer_set_text (content_buffer, text, -1);
	g_free (text);
}

/* Warning: this will take the text as it is. It won't check if it's a preview. */
static void
save_content_from_editor (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTextIter start_iter, end_iter;
	gchar *text = NULL;
	GtkTextBuffer *content_buffer = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* If we don't have a snippet loaded, don't do anything */
	if (!ANJUTA_IS_SNIPPET (priv->snippet))
		return;

	/* Get the text in the GtkTextBuffer */
	content_buffer = gtk_text_view_get_buffer (priv->content_text_view);
	gtk_text_buffer_get_start_iter (content_buffer, &start_iter);
	gtk_text_buffer_get_end_iter (content_buffer, &end_iter);
	text = gtk_text_buffer_get_text (content_buffer, &start_iter, &end_iter, FALSE);

	/* Save it to the snippet */
	snippet_set_content (priv->snippet, text);
	
	g_free (text);
}

void 
snippets_editor_set_snippet (SnippetsEditor *snippets_editor, 
                             AnjutaSnippet *snippet)
{
	SnippetsEditorPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Delete the old snippet */
	if (ANJUTA_IS_SNIPPET (priv->snippet))
		g_object_unref (priv->snippet);
	/* Set the current snippet */
	priv->backup_snippet = snippet;
	priv->snippet = snippet_copy (snippet);

	/* Initialize the snippet content editor */
	load_content_to_editor (snippets_editor);

	/* Initialize the name property */
	gtk_entry_set_text (priv->name_entry, snippet_get_name (snippet));

	/* Initialize the trigger-key property */
	gtk_entry_set_text (priv->trigger_entry, snippet_get_trigger_key (snippet));

	/* Initialize the languages combo-box property */
	/* TODO */

	/* Initialize the keywords text-view property */
	/* TODO */

	/* Initialize the variables tree-view - load the variables tree model with the variables
	   from the current snippet*/
	snippet_vars_store_unload (priv->vars_store);
	snippet_vars_store_load (priv->vars_store, priv->snippets_db, priv->snippet);

}

void 
snippets_editor_set_snippet_new (SnippetsEditor *snippets_editor)
{

}

void
snippets_editor_delete_current_snippet (SnippetsEditor *snippets_editor)
{

}


static void  
on_preview_button_toggled (GtkToggleButton *preview_button,
                           gpointer user_data)
{
	SnippetsEditor *snippets_editor = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	snippets_editor = ANJUTA_SNIPPETS_EDITOR (user_data);

	/* If we go in the preview mode, we should save the content */
	if (gtk_toggle_button_get_active (preview_button))
		save_content_from_editor (snippets_editor);

	load_content_to_editor (snippets_editor);
}

static void  
on_save_button_clicked (GtkButton *save_button,
                        gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	SnippetsEditor *snippets_editor = NULL;
	AnjutaSnippetsGroup *parent_snippets_group = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	snippets_editor = ANJUTA_SNIPPETS_EDITOR (user_data);
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));

	/* If there isn't a snippet editing */
	if (!ANJUTA_IS_SNIPPET (priv->snippet))
		return;

	/* The user should have a snippets group selected */
	if (!ANJUTA_IS_SNIPPETS_GROUP (priv->snippet->parent_snippets_group))
		return;

	/* TODO - check the trigger-key/languages aren't conflicting and show warning
	   windows */

	/* Save the content */
	if (!gtk_toggle_button_get_active (priv->preview_button))
		save_content_from_editor (snippets_editor);

	/* Delete the back-up snippet */
	snippets_db_remove_snippet (priv->snippets_db, 
	                            snippet_get_trigger_key (priv->backup_snippet),
	                            snippet_get_any_language (priv->backup_snippet),
	                            TRUE);
	parent_snippets_group = ANJUTA_SNIPPETS_GROUP (priv->snippet->parent_snippets_group);
	snippets_db_add_snippet (priv->snippets_db,
	                         priv->snippet,
	                         snippets_group_get_name (parent_snippets_group));

	/* Move the new snippet as the back-up one */
	priv->backup_snippet = priv->snippet;
	priv->snippet = snippet_copy (priv->backup_snippet);

	/* Emit the signal that the snippet was saved */
	g_signal_emit_by_name (snippets_editor, "snippet-saved", priv->backup_snippet);

}

static void
on_close_button_clicked (GtkButton *close_button,
                         gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));

	g_signal_emit_by_name (ANJUTA_SNIPPETS_EDITOR (user_data), "close-request");
}

static void 
on_name_text_cell_edited (GtkCellRendererText *cell,
                          gchar *path_string,
                          gchar *new_string,
                          gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	gchar *old_name = NULL;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* We don't accept empty strings as variables names */
	if (!g_strcmp0 (new_string, ""))
		return;

	/* Get the old name at the given path */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->vars_store_sorted), &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->vars_store_sorted), &iter,
	                    VARS_STORE_COL_NAME, &old_name,
	                    -1);

	/* If the name wasn't changed we don't do anything */
	if (!g_strcmp0 (old_name, new_string))
	{
		g_free (old_name);
		return;
	}

	/* Set the new name */
	snippet_vars_store_set_variable_name (priv->vars_store, old_name, new_string);
	
	g_free (old_name);
}

static void
on_variables_view_row_activated (GtkTreeView *tree_view,
                                 GtkTreePath *path,
                                 GtkTreeViewColumn *col,
                                 gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	gchar *name = NULL;
	GtkTreeIter iter;
	SnippetVariableType type, opposed_type;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* We check that the title column was activated */
	if (g_strcmp0 (gtk_tree_view_column_get_title (col), TYPE_COL_TITLE))
		return;

	/* Get the name at the given path */
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->vars_store_sorted), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->vars_store_sorted), &iter,
	                    VARS_STORE_COL_NAME, &name,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	if (type == SNIPPET_VAR_TYPE_LOCAL)
		opposed_type = SNIPPET_VAR_TYPE_GLOBAL;
	else
		opposed_type = SNIPPET_VAR_TYPE_LOCAL;

	snippet_vars_store_set_variable_type (priv->vars_store, name, opposed_type);

	g_free (name);
	
}

static void
on_default_text_cell_edited (GtkCellRendererText *cell,
                             gchar *path_string,
                             gchar *new_string,
                             gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	gchar *name = NULL;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* Get the name at the given path */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->vars_store_sorted), &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->vars_store_sorted), &iter,
	                    VARS_STORE_COL_NAME, &name,
	                    -1);

	snippet_vars_store_set_variable_default (priv->vars_store, name, new_string);

	g_free (name);

}
