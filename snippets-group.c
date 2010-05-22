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
	AnjutaSnippetsGroup* anjuta_snippets_group = ANJUTA_SNIPPETS_GROUP (snippets_group);
	AnjutaSnippet* cur_snippet = NULL;
	guint iter = 0;
	
	/* Delete the private field */
	if (anjuta_snippets_group->priv)
	{
		/* Delete the name and description fields */
		g_free (anjuta_snippets_group->priv->name);
		anjuta_snippets_group->priv->name = NULL;
		g_free (anjuta_snippets_group->priv->description);
		anjuta_snippets_group->priv->description = NULL;
		
		/* Delete the snippets in the group */
		for (iter = 0; iter < g_list_length (anjuta_snippets_group->priv->snippets); iter ++)
		{
			cur_snippet = g_list_nth_data (anjuta_snippets_group->priv->snippets, iter);
			g_object_unref (cur_snippet);
		}
		g_list_free (anjuta_snippets_group->priv->snippets);

		g_free (anjuta_snippets_group->priv);
		anjuta_snippets_group->priv = NULL;
	}
	
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
}

static void
snippets_group_init (AnjutaSnippetsGroup* snippets_group)
{
	AnjutaSnippetsGroupPrivate* priv = g_new0 (AnjutaSnippetsGroupPrivate, 1);
	
	snippets_group->priv = priv;
}

/**
 * snippets_group_new:
 * @snippets_group_name: A name for the group. It's unique.
 * @snippets_group_description: A short description of the snippet.
 *
 * Makes a new #AnjutaSnippetsGroup object.
 *
 * Returns: A new #AnjutaSnippetsGroup object or NULL on failure.
 **/
AnjutaSnippetsGroup* snippets_group_new (const gchar* snippets_group_name,
                                         const gchar* snippets_group_description)
{
	AnjutaSnippetsGroup* snippets_group = NULL;
	
	/* Assertions */
	if (snippets_group_name == NULL || snippets_group_description == NULL)
		return NULL;
	
	/* Initialize the object */
	snippets_group = ANJUTA_SNIPPETS_GROUP (g_object_new (snippets_group_get_type (), NULL));
	
	/* Copy the name and description */
	snippets_group->priv->name = g_strdup (snippets_group_name);
	snippets_group->priv->description = g_strdup (snippets_group_description);
	
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
gboolean snippets_group_add_snippet (AnjutaSnippetsGroup* snippets_group,
                                     AnjutaSnippet* snippet,
                                     gboolean overwrite)
{
	guint iter = 0, to_be_replaced_position = 0;
	AnjutaSnippet *cur_snippet = NULL, *to_be_replaced_snippet = NULL;
	gchar* cur_snippet_key = NULL;
	gchar* added_snippet_key = NULL;
	
	/* Assertions */
	if (snippets_group == NULL || snippet == NULL)
		return FALSE;
		
	/* Get the key of the snippet */
	added_snippet_key = snippet_get_key (snippet);
	
	/* Check if there is a snippet with the same key */
	for (iter = 0; iter < g_list_length (snippets_group->priv->snippets); iter ++)
	{
		cur_snippet = g_list_nth_data (snippets_group->priv->snippets, iter);
		cur_snippet_key = snippet_get_key (cur_snippet);
		
		if (g_strcmp0 (cur_snippet_key, added_snippet_key))
		{
			to_be_replaced_snippet = cur_snippet;
			 /* Could of used iter here, but just to be clear */
			to_be_replaced_position = iter;
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
			snippets_group->priv->snippets = g_list_insert (snippets_group->priv->snippets, snippet, to_be_replaced_position);
			snippets_group->priv->snippets = g_list_remove (snippets_group->priv->snippets, to_be_replaced_snippet);
			g_object_unref (to_be_replaced_snippet);
		}
		else
		{
			g_free (added_snippet_key);
			return FALSE;
		}
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
void snippets_group_remove_snippet (AnjutaSnippetsGroup* snippets_group,
                                    const gchar* snippet_key)
{
	guint iter = 0;
	AnjutaSnippet *to_be_deleted_snippet = NULL, *cur_snippet;
	gchar* cur_snippet_key = NULL;
	
	/* Assertions */
	if (snippets_group == NULL || snippet_key == NULL)
		return;
	
	/* Check if there is a snippet with the same key */
	for (iter = 0; iter < g_list_length (snippets_group->priv->snippets); iter ++)
	{
		cur_snippet = g_list_nth_data (snippets_group->priv->snippets, iter);
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
GList* snippets_group_get_snippet_list (AnjutaSnippetsGroup* snippets_group)
{
	return snippets_group->priv->snippets;
}
