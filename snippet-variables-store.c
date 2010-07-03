/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet-variables-store.c
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

#include "snippet-variables-store.h"

#define ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPET_VARS_STORE, SnippetVarsStorePrivate))

struct _SnippetVarsStorePrivate
{
	SnippetsDB *snippets_db;
	AnjutaSnippet *snippet;
	
};

G_DEFINE_TYPE (SnippetVarsStore, snippet_vars_store, GTK_TYPE_LIST_STORE);

static void
snippet_vars_store_init (SnippetVarsStore *snippet_vars_store)
{
	SnippetVarsStorePrivate *priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (snippet_vars_store);
	
	snippet_vars_store->priv = priv;

	/* Initialize the private field */
	priv->snippets_db = NULL;
	priv->snippet     = NULL;
	
}

static void
snippet_vars_store_class_init (SnippetVarsStoreClass *snippet_vars_store_class)
{
	snippet_vars_store_parent_class = g_type_class_peek_parent (snippet_vars_store_class);

	g_type_class_add_private (snippet_vars_store_class, sizeof (SnippetVarsStorePrivate));
}


SnippetVarsStore* 
snippet_vars_store_new ()
{
	return ANJUTA_SNIPPET_VARS_STORE (g_object_new (snippet_vars_store_get_type (), NULL));
}

/**
 * reload_vars_store:
 * @vars_store: A #SnippetVarsStore object.
 *
 * Reloads the GtkListStore with the current values of the variables in the internal
 * snippet and snippets-db. If priv->snippet or priv->snippets_db is NULL, it will clear the 
 * GtkListStore.
 */
static void
reload_vars_store (SnippetVarsStore *vars_store)
{
	SnippetVarsStorePrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	priv = ANJUTA_SNIPPETS_VAR_STORE_GET_PRIVATE (vars_store);

	/* TODO */
}

static void
on_global_vars_store_row_changed (GtkTreeModel *tree_model,
                                  GtkTreePath *path,
                                  GtkTreeIter *iter,
                                  gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_VARS_STORE (user_data));

	reload_vars_store (ANJUTA_SNIPPETS_VARS_STORE (user_data));
}


static void
on_global_vars_store_row_inserted (GtkTreeModel *tree_model,
                                   GtkTreePath *path,
                                   GtkTreeIter *iter,
                                   gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_VARS_STORE (user_data));

	reload_vars_store (ANJUTA_SNIPPETS_VARS_STORE (user_data));
}

static void
on_global_vars_store_row_removed (GtkTreeModel *tree_model,
                                  GtkTreePath *path,
                                  gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_VARS_STORE (user_data));

	reload_vars_store (ANJUTA_SNIPPETS_VARS_SOTRE (user_data));
}

void                       
snippet_vars_store_load (SnippetVarsStore *vars_store,
                         SnippetsDB *snippets_db,
                         AnjutaSnippet *snippet)
{
	SnippetsVarsStorePrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_VARS_STORE (vars_store));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	priv = ANJUTA_SNIPPETS_VAR_STORE_GET_PRIVATE (vars_store);
	
	g_object_ref (snippets_db);
	g_object_unref (snippet);
	priv->snippets_db = snippets_db;
	priv->snippet = snippet;

	/* This will fill the GtkListStore with values of variables from snippets_db 
	   and snippet */
	reload_vars_store (vars_store);

	/* We connect to the signals that change the GtkTreeModel of the global variables.
	   This is to make sure our store is synced with the global variables model. */
	g_signal_connect (GTK_OBJECT (snippets_db_get_global_vars_model (snippets_db)),
	                  "row-inserted",
	                  GTK_SIGNAL_FUNC (on_global_vars_model_row_inserted),
	                  vars_store);
	g_signal_connect (GTK_OBJECT (snippets_db_get_global_vars_model (snippets_db)),
	                  "row-changed",
	                  GTK_SIGNAL_FUNC (on_global_vars_model_row_changed),
	                  vars_store);
	g_signal_connect (GTK_OBJECT (snippets_db_get_global_vars_model (snippets_db)),
	                  "row-deleted",
	                  GTK_SIGNAL_FUNC (on_global_vars_model_row_deleted),
	                  vars_store);
}

void
snippet_vars_store_unload (SnippetVarsStore *vars_store)
{
	SnippetsVarsStorePrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_VARS_STORE (vars_store));
	priv = ANJUTA_SNIPPETS_VAR_STORE_GET_PRIVATE (vars_store);

	if (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db))
	{	
		g_object_unref (priv->snippets_db);
		priv->snippets_db = NULL;
	}
	if (ANJUTA_IS_SNIPPET (priv->snippet))
	{
		g_object_unref (priv->snippet);
		priv->snippet = NULL;
	}

	/* This will clear the GtkListStore */
	reload_vars_store (vars_store);
}

void
snippet_vars_store_set_variable_name (SnippetVarsStore *vars_store,
                                      const gchar *old_variable_name,
                                      const gchar *new_variable_name)
{
	/* TODO */
}

void 
snippet_vars_store_set_variable_type (SnippetVarsStore *vars_store,
                                      const gchar *variable_name,
                                      SnippetVariableType old_type,
                                      SnippetVariableType new_type)
{
	/* TODO */
}

void
snippet_vars_store_set_variable_default (SnippetVarsStore *vars_store,
                                         const gchar *variable_name,
                                         const gchar *default_value)
{
	/* TODO */
}

void
snippet_vars_store_add_variable (SnippetVarsStore *vars_store,
                                 const gchar *variable_name)
{
	/* TODO */
}

void
snippet_vars_store_remove_variable (SnippetVarsStore *vars_store,
                                    const gchar *variable_name)
{
	/* TODO */
}
