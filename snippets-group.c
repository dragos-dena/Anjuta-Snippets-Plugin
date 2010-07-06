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
	
	GList* snippets;
};


G_DEFINE_TYPE (AnjutaSnippetsGroup, snippets_group, G_TYPE_OBJECT);

static void
snippets_group_dispose (GObject* snippets_group)
{
	AnjutaSnippetsGroup *anjuta_snippets_group = ANJUTA_SNIPPETS_GROUP (snippets_group);
	AnjutaSnippet *cur_snippet = NULL;
	GList *iter = NULL;

	/* Delete the name and description fields */
	g_free (anjuta_snippets_group->priv->name);
	anjuta_snippets_group->priv->name = NULL;
	
	/* Delete the snippets in the group */
	for (iter = g_list_first (anjuta_snippets_group->priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		g_object_unref (cur_snippet);
	}
	g_list_free (anjuta_snippets_group->priv->snippets);

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
	snippets_group->file_path = NULL;

	/* Initialize the private field */
	snippets_group->priv->name = NULL;
	snippets_group->priv->snippets = NULL;
}

/**
 * snippets_group_new:
 * @snippets_filename: The filename from which the snippet group was loaded.
 * @snippets_group_name: A name for the group. It's unique.
 *
 * Makes a new #AnjutaSnippetsGroup object.
 *
 * Returns: A new #AnjutaSnippetsGroup object or NULL on failure.
 **/
AnjutaSnippetsGroup* 
snippets_group_new (const gchar* snippets_file_path,
                    const gchar* snippets_group_name)
{
	AnjutaSnippetsGroup* snippets_group = NULL;
	
	/* Assertions */
	g_return_val_if_fail (snippets_group_name != NULL, NULL);
	
	/* Initialize the object */
	snippets_group = ANJUTA_SNIPPETS_GROUP (g_object_new (snippets_group_get_type (), NULL));
	
	/* Copy the name, description and filename */
	snippets_group->priv->name = g_strdup (snippets_group_name);
	snippets_group->file_path = g_strdup (snippets_file_path);
	
	return snippets_group;
}


const gchar*
snippets_group_get_name (AnjutaSnippetsGroup* snippets_group)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), NULL);

	return snippets_group->priv->name;
}

void
snippets_group_set_name (AnjutaSnippetsGroup* snippets_group,
                         const gchar* new_group_name)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group));

	g_free (snippets_group->priv->name);
	snippets_group->priv->name = g_strdup (new_group_name);
}

static gint
compare_snippets_by_name (gconstpointer a,
                          gconstpointer b)
{
	AnjutaSnippet *snippet_a = (AnjutaSnippet *)a,
	              *snippet_b = (AnjutaSnippet *)b;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet_a), 0);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet_b), 0);

	return g_utf8_collate (snippet_get_name (snippet_a),
	                       snippet_get_name (snippet_b));
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
	GList *iter = NULL, *to_be_replaced_node = NULL, *languages = NULL, *iter2 = NULL;
	AnjutaSnippet *cur_snippet = NULL, *to_be_replaced_snippet = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	languages = (GList *)snippet_get_languages (snippet);
	
	/* Check if there is a snippet with the same key */
	for (iter = g_list_first (snippets_group->priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		g_return_val_if_fail (ANJUTA_IS_SNIPPET (cur_snippet), FALSE);

		if (!g_strcmp0 (snippet_get_trigger_key (cur_snippet), snippet_get_trigger_key (snippet)))
		{
			/* We test if any language conflict occurs */
			gboolean conflict = FALSE;
			for (iter2 = g_list_first (languages); iter2 != NULL; iter2 = g_list_next (iter2))
				if (snippet_has_language (cur_snippet, (gchar *)iter2->data))
				{
					conflict = TRUE;
					break;
				}

			/* If we have a conflict it means we need to overwrite it (if we have permissions)
			   or the adding will fail (if we don't) */
			if (conflict)
			{
				to_be_replaced_snippet = cur_snippet;
				 /* Could of used iter here, but just to be clear */
				to_be_replaced_node = iter;
				break;
			}
		}
		
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
			return FALSE;
		}
	}
	/* If we didn't found a snippet with the same name we just append it to the end */
	else
	{
		snippets_group->priv->snippets = g_list_insert_sorted (snippets_group->priv->snippets,
		                                                       snippet,
		                                                       compare_snippets_by_name);
	}
	
	snippet->parent_snippets_group = G_OBJECT (snippets_group);
	
	return TRUE;
}

/**
 * snippets_group_remove_snippet:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 * @trigger_key: The trigger-key of the #AnjutaSnippet to be removed.
 * @language: The language of the #AnjutaSnippet to be removed.
 * @remove_all_languages_support: If it's FALSE it will remove just the support of the snippet for
 *                                the language. If it's TRUE it will actually remove the snippet.
 *
 * If remove_all_languages_support is TRUE, this will remove the snippet from the snippet-group,
 * if it's FALSE, it will just remove the language support for the given language.
 **/
void 
snippets_group_remove_snippet (AnjutaSnippetsGroup* snippets_group,
                               const gchar* trigger_key,
                               const gchar* language,
                               gboolean remove_all_languages_support)
{
	GList *iter = NULL;
	AnjutaSnippet *to_be_deleted_snippet = NULL, *cur_snippet;
	const gchar *cur_snippet_trigger = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group));
	g_return_if_fail (trigger_key != NULL);
	g_return_if_fail (language != NULL);

	/* Check if there is a snippet with the same key */
	for (iter = g_list_first (snippets_group->priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		if (!ANJUTA_IS_SNIPPET (cur_snippet))
			g_return_if_reached ();
		cur_snippet_trigger = snippet_get_trigger_key (cur_snippet);
		
		if (!g_strcmp0 (cur_snippet_trigger, trigger_key) &&
		    snippet_has_language (cur_snippet, language))
		{
			if (remove_all_languages_support)
			{
				to_be_deleted_snippet = cur_snippet;
				break;
			}
			else
			{
				snippet_remove_language (cur_snippet, language);
				return;
			}
		}
		
	}
	
	/* If we found a snippet that should be deleted we remove it from the list and unref it */
	if (to_be_deleted_snippet)
	{
		snippets_group->priv->snippets = g_list_remove (snippets_group->priv->snippets, to_be_deleted_snippet);
		g_object_unref (to_be_deleted_snippet);

	}
}

/**
 * snippets_group_get_snippets_list:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 *
 * Gets the snippets in this group.
 * Important: This returns the actual list as saved in the #AnjutaSnippetsGroup.
 *
 * Returns: A #GList with entries of type #AnjutaSnippet.
 **/
const GList* 
snippets_group_get_snippets_list (AnjutaSnippetsGroup* snippets_group)
{
	return snippets_group->priv->snippets;
}
