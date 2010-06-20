/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-interaction-interpreter.c
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

#include "snippets-interaction-interpreter.h"
#include <libanjuta/interfaces/ianjuta-editor.h>

#define ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_INTERACTION, SnippetsInteractionPrivate))

struct _SnippetsInteractionPrivate
{
	IAnjutaIterable *snippet_start_position;
	IAnjutaIterable *snippet_end_position;
	IAnjutaIterable *cursor_position;

	AnjutaSnippet *cur_snippet;
};

G_DEFINE_TYPE (SnippetsInteraction, snippets_interaction, G_TYPE_OBJECT);

static void
snippets_interaction_dispose (GObject *obj)
{
	/* TODO */
}

static void
snippets_interaction_finalize (GObject *obj)
{
	/* TODO */
}

static void
snippets_interaction_init (SnippetsInteraction *snippets_interaction)
{
	SnippetsInteractionPrivate* priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	
	snippets_interaction->priv = priv;

	/* Initialize the private field */
	snippets_interaction->priv->snippet_start_position = NULL;
	snippets_interaction->priv->snippet_end_position   = NULL;
	snippets_interaction->priv->cursor_position        = NULL;

	snippets_interaction->priv->cur_snippet = NULL;
}

static void
snippets_interaction_class_init (SnippetsInteractionClass *snippets_interaction_class)
{
	GObjectClass *klass = G_OBJECT_CLASS (snippets_interaction_class);
	snippets_interaction_parent_class = g_type_class_peek_parent (snippets_interaction_class);
	klass->dispose  = snippets_interaction_dispose;
	klass->finalize = snippets_interaction_finalize;

	g_type_class_add_private (snippets_interaction_class, sizeof (SnippetsInteractionPrivate));
}


SnippetsInteraction* 
snippets_interaction_new ()
{
	return ANJUTA_SNIPPETS_INTERACTION (g_object_new (snippets_interaction_get_type (), NULL));
}

void
snippets_interaction_start (SnippetsInteraction *snippets_interaction,
                            AnjutaShell *shell)
{
	/* TODO */
}

void
snippets_interaction_destroy (SnippetsInteraction *snippets_interaction)
{
	/* TODO */
}

void
snippets_interaction_interpret_snippet (SnippetsInteraction *snippets_interaction,
                                        const AnjutaSnippet *snippet)
{
	/* TODO */
}
