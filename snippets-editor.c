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

#define EDITOR_UI      PACKAGE_DATA_DIR"/glade/snippets-editor.ui"

#define ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_EDITOR, SnippetsEditorPrivate))

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
	GtkImage *warning_image;

	/* Variables widgets */
	GtkTreeView *variables_view;
	GtkButton *variable_add_button;
	GtkButton *variable_remove_button;
	GtkButton *variable_insert_button;

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
	priv->warning_image = NULL;

	priv->variables_view = NULL;
	priv->variable_add_button = NULL;
	priv->variable_remove_button = NULL;
	priv->variable_insert_button = NULL;

	priv->save_button = NULL;
	priv->close_button = NULL;
	priv->editor_alignment = NULL;
}

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

	/* Get the widgets and assert them */
	priv->content_text_view = GTK_TEXT_VIEW (gtk_builder_get_object (bxml, "content_text_view"));
	priv->preview_button = GTK_TOGGLE_BUTTON (gtk_builder_get_object (bxml, "preview_button"));
	g_return_if_fail (GTK_IS_TEXT_VIEW (priv->content_text_view));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (priv->preview_button));
	
	priv->name_entry = GTK_ENTRY (gtk_builder_get_object (bxml, "name_entry"));
	priv->trigger_entry = GTK_ENTRY (gtk_builder_get_object (bxml, "trigger_entry"));
	priv->languages_combo_box = GTK_COMBO_BOX (gtk_builder_get_object (bxml, "languages_combo_box"));
	priv->snippets_group_combo_box = GTK_COMBO_BOX (gtk_builder_get_object (bxml, "snippets_group_combo_box"));
	priv->warning_image = GTK_IMAGE (gtk_builder_get_object (bxml, "warning_image"));
	g_return_if_fail (GTK_IS_ENTRY (priv->name_entry));
	g_return_if_fail (GTK_IS_ENTRY (priv->trigger_entry));
	g_return_if_fail (GTK_IS_COMBO_BOX (priv->languages_combo_box));
	g_return_if_fail (GTK_IS_COMBO_BOX (priv->snippets_group_combo_box));
	g_return_if_fail (GTK_IS_IMAGE (priv->warning_image));
	
	priv->variables_view = GTK_TREE_VIEW (gtk_builder_get_object (bxml, "variables_view"));
	priv->variable_add_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_add_button"));
	priv->variable_remove_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_remove_button"));
	priv->variable_insert_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_insert_button"));
	g_return_if_fail (GTK_IS_TREE_VIEW (priv->variables_view));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_add_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_remove_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_insert_button));
	
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

SnippetsEditor *
snippets_editor_new (SnippetsDB *snippets_db)
{
	SnippetsEditor *snippets_editor = 
	          ANJUTA_SNIPPETS_EDITOR (g_object_new (snippets_editor_get_type (), NULL));
	SnippetsEditorPrivate *priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), snippets_editor);

	priv->snippets_db = snippets_db;

	/* Load the UI for snippets-editor.ui */
	load_snippets_editor_ui (snippets_editor);

	/* TODO - connect the handlers */

	return snippets_editor;
}
