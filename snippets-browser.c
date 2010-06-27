/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-browser.c
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

#include "snippets-browser.h"
#include "snippets-group.h"
#include "snippets-interaction-interpreter.h"

#define ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_BROWSER, SnippetsBrowserPrivate))

struct _SnippetsBrowserPrivate
{
	GtkTreeView *snippets_view;

	SnippetsDB *snippets_db;
};


G_DEFINE_TYPE (SnippetsBrowser, snippets_browser, GTK_TYPE_WIDGET);

static void
snippets_browser_destroy (GtkObject *gtk_object)
{
	/* TODO */
}

static void
snippets_browser_dispose (GObject *object)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (object));

	GTK_OBJECT_CLASS (snippets_browser_parent_class)->destroy (GTK_OBJECT (object));
	G_OBJECT_CLASS (snippets_browser_parent_class)->dispose (G_OBJECT (object));
}

static void
snippets_browser_class_init (SnippetsBrowserClass* klass)
{
	GObjectClass *g_object_class = NULL;
	GtkObjectClass *gtk_object_class = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER_CLASS (klass));

	gtk_object_class = GTK_OBJECT_CLASS (klass);
	gtk_object_class->destroy = snippets_browser_destroy;

	g_object_class = G_OBJECT_CLASS (klass);
	g_object_class->dispose = snippets_browser_dispose;

	/* When the selection of the TreeView changes. The object passed here can be an
	   AnjutaSnippet or AnjutaSnippetsGroup depending on the selection. */
	g_signal_new ("current-selection-changed",
	              ANJUTA_TYPE_SNIPPETS_BROWSER,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsBrowserClass, current_selection_changed),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__OBJECT,
	              G_TYPE_NONE,
	              1,
	              G_TYPE_OBJECT,
	              NULL);

	/* The SnippetsBrowser asks for a maximize. If a maximize is possible,
	   the snippets_browser_show_editor should be called. */
	g_signal_new ("maximize-request",
	              ANJUTA_TYPE_SNIPPETS_BROWSER,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsBrowserClass, maximize_request),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE,
	              0,
	              NULL);
	
	g_type_class_add_private (klass, sizeof (SnippetsBrowserPrivate));
}

static void
snippets_browser_init (SnippetsBrowser* snippets_browser)
{
	SnippetsBrowserPrivate* priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	
	snippets_browser->priv = priv;
	
	/* Initialize the private field */
	snippets_browser->priv->snippets_view = NULL;
	snippets_browser->priv->snippets_db = NULL;
}

/* Private methods */

static void
snippets_view_name_data_func (GtkTreeViewColumn *column,
                              GtkCellRenderer *renderer,
                              GtkTreeModel *snippets_db_model,
                              GtkTreeIter *iter,
                              gpointer user_data)
{
	const gchar *name = NULL;
	GObject *cur_object = NULL;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	
	/* Get the name */
	gtk_tree_model_get (snippets_db_model, iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);
	if (ANJUTA_IS_SNIPPET (cur_object))
		name = snippet_get_name (ANJUTA_SNIPPET (cur_object));
	else
		name = snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (cur_object));

	g_object_set (renderer, "text", name, NULL);
}

static void
init_snippets_view (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *column = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (GTK_IS_TREE_VIEW (priv->snippets_view));
	g_return_if_fail (GTK_IS_TREE_MODEL (priv->snippets_db));
	
	gtk_tree_view_set_model (priv->snippets_view,
	                         GTK_TREE_MODEL (priv->snippets_db));

	/* FIXME - delete after debugging is done */
	renderer = gtk_cell_renderer_text_new ();
	column   = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, "Name");
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
	                                         snippets_view_name_data_func,
	                                         snippets_browser, NULL);
	gtk_tree_view_append_column (priv->snippets_view, column);

	GtkWidget *window;
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete_event", gtk_main_quit, NULL);
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (priv->snippets_view));
  gtk_widget_show_all (window);
}


/* Public methods */

/**
 * snippets_browser_new:
 *
 * Returns: A new #SnippetsBrowser object.
 */
SnippetsBrowser*
snippets_browser_new (void)
{
	return ANJUTA_SNIPPETS_BROWSER (g_object_new (snippets_browser_get_type (), NULL));
}

/**
 * snippets_browser_load:
 * @snippets_browser: A #SnippetsBrowser object.
 * @snippets_db: A #SnippetsDB object from which the browser should be loaded.
 *
 * Loads the #SnippetsBrowser with snippets that are found in the given database.
 */
void                       
snippets_browser_load (SnippetsBrowser *snippets_browser,
                       SnippetsDB *snippets_db)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	priv->snippets_db   = snippets_db;
	priv->snippets_view = GTK_TREE_VIEW (gtk_tree_view_new ());
	init_snippets_view (snippets_browser);
}

/**
 * snippets_browser_unload:
 * @snippets_browser: A #SnippetsBrowser object.
 * 
 * Unloads the current #SnippetsBrowser object.
 */
void                       
snippets_browser_unload (SnippetsBrowser *snippets_browser)
{
	/* TODO */
}

/**
 * snippets_browser_show_editor:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Shows the editor attached to the browser. Warning: This will take up considerably
 * more space than just having the browser shown.
 */
void                       
snippets_browser_show_editor (SnippetsBrowser *snippets_browser)
{
	/* TODO */
}


/**
 * snippets_browser_hide_editor:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Hides the editor attached to the browser. 
 */
void                       
snippets_browser_hide_editor (SnippetsBrowser *snippets_browser)
{
	/* TODO */
}
