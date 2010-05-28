/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-group.h
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

#include "snippets-group.h"
#include <gio/gio.h>


#define ANJUTA_SNIPPETS_GROUP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_GROUP, AnjutaSnippetsGroupPrivate))

struct _AnjutaSnippetsGroupPrivate
{
	gchar* name;
	gchar* description;
	
	GList* snippets;
};


G_DEFINE_TYPE (AnjutaSnippetsGroup, snippets_group, G_TYPE_OBJECT);

static void
snippets_group_dispose (GObject* snippets_group)
{
	AnjutaSnippetsGroup *anjuta_snippets_group = ANJUTA_SNIPPETS_GROUP (snippets_group);
	AnjutaSnippet *cur_snippet = NULL;
	GList *iter = NULL;
	
	/* Delete the private field */
	if (anjuta_snippets_group->priv)
	{
		/* Delete the name and description fields */
		g_free (anjuta_snippets_group->priv->name);
		anjuta_snippets_group->priv->name = NULL;
		g_free (anjuta_snippets_group->priv->description);
		anjuta_snippets_group->priv->description = NULL;
		
		/* Delete the snippets in the group */
		for (iter = g_list_first (anjuta_snippets_group->priv->snippets); iter != NULL; iter = g_list_next (iter))
		{
			cur_snippet = (AnjutaSnippet *)cur_snippet;
			g_object_unref (cur_snippet);
		}
		g_list_free (anjuta_snippets_group->priv->snippets);

		g_free (anjuta_snippets_group->priv);
		anjuta_snippets_group->priv = NULL;
	}

	/* Delete the filename field */
	g_free (anjuta_snippets_group->file_path);
	anjuta_snippets_group->file_path = NULL;
	
	G_OBJECT_CLASS (snippets_group_parent_class)->dispose (snippets_group);
}

static void
snippets_group_finalize (GObject* snippets_group)
{
	G_OBJECT_CLASS (snippets_group_parent_class)->finalize (snippets_group);
}

static void
snippets_group_class_init (AnjutaSnippetsGroupClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippets_group_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippets_group_dispose;
	object_class->finalize = snippets_group_finalize;

	g_type_class_add_private (klass, sizeof (AnjutaSnippetsGroupPrivate));
}

static void
snippets_group_init (AnjutaSnippetsGroup* snippets_group)
{
	AnjutaSnippetsGroupPrivate* priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);
	
	snippets_group->priv = priv;
}

/**
 * snippets_group_new:
 * @snippets_filename: The filename from which the snippet group was loaded.
 * @snippets_group_name: A name for the group. It's unique.
 * @snippets_group_description: A short description of the snippet.
 *
 * Makes a new #AnjutaSnippetsGroup object.
 *
 * Returns: A new #AnjutaSnippetsGroup object or NULL on failure.
 **/
AnjutaSnippetsGroup* 
snippets_group_new (const gchar* snippets_file_path,
                    const gchar* snippets_group_name,
                    const gchar* snippets_group_description)
{
	AnjutaSnippetsGroup* snippets_group = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippets_group_name != NULL &&\
	                      snippets_group_description != NULL &&\
	                      snippets_file_path != NULL,
	                      NULL);
	
	/* Initialize the object */
	snippets_group = ANJUTA_SNIPPETS_GROUP (g_object_new (snippets_group_get_type (), NULL));
	
	/* Copy the name, description and filename */
	snippets_group->priv->name = g_strdup (snippets_group_name);
	snippets_group->priv->description = g_strdup (snippets_group_description);
	snippets_group->file_path = g_strdup (snippets_file_path);
	
	return snippets_group;
}

/**
 * snippets_group_add_snippet:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 * @snippet: A snippet to be added.
 * @overwrite: If on a conflict the conflicting old snippet should be overwrited.
 *
 * Adds a new #AnjutaSnippet to the snippet group.
 *
 * Returns: TRUE on success.
 **/
gboolean 
snippets_group_add_snippet (AnjutaSnippetsGroup* snippets_group,
                            AnjutaSnippet* snippet,
                            gboolean overwrite)
{
	GList *iter = NULL, *to_be_replaced_node = NULL;
	AnjutaSnippet *cur_snippet = NULL, *to_be_replaced_snippet = NULL;
	gchar* cur_snippet_key = NULL;
	gchar* added_snippet_key = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippets_group != NULL &&\
	                      snippet != NULL,
	                      FALSE);
		
	/* Get the key of the snippet */
	added_snippet_key = snippet_get_key (snippet);
	
	/* Check if there is a snippet with the same key */
	for (iter = g_list_first (snippets_group->priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		cur_snippet_key = snippet_get_key (cur_snippet);
		
		if (!g_strcmp0 (cur_snippet_key, added_snippet_key))
		{
			to_be_replaced_snippet = cur_snippet;
			 /* Could of used iter here, but just to be clear */
			to_be_replaced_node = iter;
			g_free (cur_snippet_key);
			break;
		}
		
		g_free (cur_snippet_key);
	}
	
	/* If we found a snippet with the same name */
	if (to_be_replaced_snippet)
	{
		if (overwrite)
		{
			snippets_group->priv->snippets = g_list_insert_before (snippets_group->priv->snippets,
			                                                       to_be_replaced_node,
			                                                       snippet);
			snippets_group->priv->snippets = g_list_remove (snippets_group->priv->snippets, to_be_replaced_snippet);
			g_object_unref (to_be_replaced_snippet);
		}
		else
		{
			g_free (added_snippet_key);
			return FALSE;
		}
	}
	/* If we didn't found a snippet with the same name we just append it to the end */
	else
	{
		snippets_group->priv->snippets = g_list_append (snippets_group->priv->snippets, snippet);
	}
	
	g_free (added_snippet_key);
	return TRUE;
}

/**
 * snippets_group_remove_snippet:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 * @snippet_key: The snippet-key of the #AnjutaSnippet to be removed.
 *
 * Removes a new #AnjutaSnippet from the snippet group.
 **/
void 
snippets_group_remove_snippet (AnjutaSnippetsGroup* snippets_group,
                               const gchar* snippet_key)
{
	GList *iter = NULL;
	AnjutaSnippet *to_be_deleted_snippet = NULL, *cur_snippet;
	gchar* cur_snippet_key = NULL;
	
	/* Assertions */
	g_return_if_fail (snippets_group != NULL && snippet_key != NULL);
	
	/* Check if there is a snippet with the same key */
	for (iter = g_list_first (snippets_group->priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		cur_snippet_key = snippet_get_key (cur_snippet);
		
		if (g_strcmp0 (cur_snippet_key, snippet_key))
		{
			to_be_deleted_snippet = cur_snippet;
			g_free (cur_snippet_key);
			break;
		}
		
		g_free (cur_snippet_key);
	}
	
	/* If we found a snippet that should be deleted we remove it from the list and unref it */
	if (to_be_deleted_snippet)
	{
		snippets_group->priv->snippets = g_list_remove (snippets_group->priv->snippets, to_be_deleted_snippet);
		g_object_unref (to_be_deleted_snippet);
	}
}

/**
 * snippets_group_get_snippet_list:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 *
 * Gets the snippets in this group.
 * Important: This returns the actual list as saved in the #AnjutaSnippetsGroup.
 *
 * Returns: A #GList with entries of type #AnjutaSnippet.
 **/
const GList* 
snippets_group_get_snippet_list (AnjutaSnippetsGroup* snippets_group)
{
	return snippets_group->priv->snippets;
}
