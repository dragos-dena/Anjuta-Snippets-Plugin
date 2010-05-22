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
	gchar* snippets_group_name;
	gchar* snippets_group_description;
	
	GList* snippets;
};


G_DEFINE_TYPE (AnjutaSnippetsGroup, snippets_group, G_TYPE_OBJECT);

static void
snippets_group_dispose (GObject* snippets_group)
{
	/* TODO */
	G_OBJECT_CLASS (snippets_group_parent_class)->dispose (snippets_group);
}

static void
snippets_group_finalize (GObject* snippets_group)
{
	/* TODO */
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
	/* TODO */
	AnjutaSnippetsGroupPrivate* priv;
	
	snippets_group->priv = priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);
}

/**
 * snippets_group_new:
 * @snippets_group_name: A name for the group. It's unique.
 * @snippets_group_description: A short description of the snippet.
 *
 * Makes a new #AnjutaSnippetsGroup object.
 *
 * Returns: A new #AnjutaSnippetsGroup object.
 **/
AnjutaSnippetsGroup* snippets_group_new (const gchar* snippets_group_name,
                                         const gchar* snippets_group_description)
{

	return NULL;
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

	return FALSE;
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

}

/**
 * snippets_group_get_snippet_list:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 *
 * Gets the snippets in this group.
 *
 * Returns: A #GList with entries of type #AnjutaSnippet.
 **/
GList* snippets_group_get_snippet_list (AnjutaSnippetsGroup* snippets_group)
{
	return snippets_group->priv->snippets;
}
