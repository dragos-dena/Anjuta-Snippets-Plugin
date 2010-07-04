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

#define ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_EDITOR, SnippetsEditorPrivate))

struct _SnippetsEditorPrivate
{
	SnippetsDB *snippets_db;
	AnjutaSnippet *snippet;
	AnjutaSnippet *backup_snippet;

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
}
