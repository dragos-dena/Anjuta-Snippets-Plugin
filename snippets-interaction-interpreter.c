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
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>

#define IN_WORD(c)    (g_ascii_isalnum (c) || c == '_')

#define ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_INTERACTION, SnippetsInteractionPrivate))

struct _SnippetsInteractionPrivate
{
	IAnjutaIterable *snippet_start_position;
	IAnjutaIterable *snippet_end_position;
	IAnjutaIterable *cursor_position;

	AnjutaSnippet *cur_snippet;
	
	IAnjutaEditor *cur_editor;
	gboolean editing;
	
	AnjutaShell *shell;
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

	/* Initialize the private field */
	priv->snippet_start_position = NULL;
	priv->snippet_end_position   = NULL;
	priv->cursor_position        = NULL;

	priv->cur_snippet = NULL;

	priv->cur_editor = NULL;
	priv->editing    = FALSE;

	priv->shell = NULL;
	
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

/* Private */

static gchar
char_at_iterator (IAnjutaEditor *editor,
                  IAnjutaIterable *iter)
{
	IAnjutaIterable *next = NULL;
	gchar *text = NULL, returned_char = 0;
	
	/* Assertions */
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), 0);
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (iter), 0);

	next = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_next (next, NULL);
	
	text = ianjuta_editor_get_text (editor, iter, next, NULL);
	if (text == NULL)
		return 0;

	returned_char = text[0];
	g_free (text);
	g_object_unref (next);

	return returned_char;	
}

/* Public methods */

SnippetsInteraction* 
snippets_interaction_new ()
{
	return ANJUTA_SNIPPETS_INTERACTION (g_object_new (snippets_interaction_get_type (), NULL));
}

void
snippets_interaction_start (SnippetsInteraction *snippets_interaction,
                            AnjutaShell *shell)
{
	SnippetsInteractionPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);

	priv->shell = shell;
}

void
snippets_interaction_destroy (SnippetsInteraction *snippets_interaction)
{
	/* TODO */
}


void                 
snippets_interaction_insert_snippet (SnippetsInteraction *snippets_interaction,
                                     SnippetsDB *snippets_db,
                                     AnjutaSnippet *snippet)
{
	SnippetsInteractionPrivate *priv = NULL;
	gchar *indent = NULL, *cur_line = NULL, *snippet_default_content = NULL;
	IAnjutaIterable *line_begin = NULL, *cur_pos = NULL;
	gint cur_line_no = -1, i = 0;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaDocument *cur_doc = NULL;
	IAnjutaEditor *cur_editor = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	g_return_if_fail (ANJUTA_IS_SHELL (priv->shell));

	/* Get the current document and check that it is an editor */
	docman = anjuta_shell_get_interface (priv->shell,
	                                     IAnjutaDocumentManager,
	                                     NULL);
	g_return_if_fail (IANJUTA_IS_DOCUMENT_MANAGER (docman));
	cur_doc = ianjuta_document_manager_get_current_document (docman, NULL);
	g_return_if_fail (IANJUTA_IS_EDITOR (cur_doc));
	cur_editor = IANJUTA_EDITOR (cur_doc);


	/* Get the current line */
	cur_line_no = ianjuta_editor_get_lineno (cur_editor, NULL);
	line_begin  = ianjuta_editor_get_line_begin_position (cur_editor, cur_line_no, NULL);
	cur_pos     = ianjuta_editor_get_position (cur_editor, NULL);
	cur_line    = ianjuta_editor_get_text (cur_editor, line_begin, cur_pos, NULL);

	/* Calculate the current indentation */
	indent = g_strdup (cur_line);
	while (cur_line[i] == ' ' || cur_line[i] == '\t')
		i ++;
	indent[i] = 0;

	/* Get the default content of the snippet */
	snippet_default_content = snippet_get_default_content (snippet, 
	                                                       G_OBJECT (snippets_db), 
	                                                       indent);
	g_return_if_fail (snippet_default_content != NULL);
	
	/* Insert the default content into the editor */
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (cur_editor), NULL);
	ianjuta_editor_insert (cur_editor, 
	                       cur_pos, 
	                       snippet_default_content, 
	                       -1,
	                       NULL);
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (cur_editor), NULL);
	ianjuta_document_grab_focus (IANJUTA_DOCUMENT (cur_editor), NULL);
	
	/* TODO -- init the snippet editing session */
	priv->cur_snippet = snippet;
	priv->cur_editor  = cur_editor;

	g_free (indent);
	g_free (snippet_default_content);
	g_object_unref (line_begin);
	g_object_unref (cur_pos);
	
}

void
snippets_interaction_trigger_insert_request (SnippetsInteraction *snippets_interaction,
                                             SnippetsDB *snippets_db)
{
	SnippetsInteractionPrivate *priv = NULL;
	IAnjutaIterable *rewind_iter = NULL, *cur_pos = NULL;
	gchar *trigger = NULL, cur_char = 0;
	gboolean reached_start = FALSE;
	AnjutaSnippet *snippet = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	g_return_if_fail (ANJUTA_IS_SHELL (priv->shell));

	if (!IANJUTA_IS_EDITOR (priv->cur_editor))
		return;

	/* Initialize the iterators */
	cur_pos     = ianjuta_editor_get_position (priv->cur_editor, NULL);
	rewind_iter = ianjuta_iterable_clone (cur_pos, NULL);

	/* If we are inside a word we can't insert a snippet */
	cur_char = char_at_iterator (priv->cur_editor, cur_pos);
	if (IN_WORD (cur_char))
		return;

	/* If we can't decrement the cur_pos iterator, then we are at the start of the document,
	   so the trigger key is NULL */
	if (!ianjuta_iterable_previous (rewind_iter, NULL))
		return;
	cur_char = char_at_iterator (priv->cur_editor, rewind_iter);

	/* We iterate until we get to the start of the word or the start of the document */
	while (IN_WORD (cur_char))
	{	
		if (!ianjuta_iterable_previous (rewind_iter, NULL))
		{
			reached_start = TRUE;
			break;
		}

		cur_char = char_at_iterator (priv->cur_editor, rewind_iter);
	}

	/* If we didn't reached the start of the document, we move the iterator forward one
	   step so we don't delete one extra char. */
	if (!reached_start)
		ianjuta_iterable_next (rewind_iter, NULL);

	/* We compute the trigger-key */
	trigger = ianjuta_editor_get_text (priv->cur_editor, rewind_iter, cur_pos, NULL);

	/* If there is a snippet for our trigger-key we delete the trigger from the editor
	   and insert the snippet. */
	snippet = snippets_db_get_snippet (snippets_db, trigger, NULL);
	if (ANJUTA_IS_SNIPPET (snippet))
	{
		ianjuta_editor_erase (priv->cur_editor, rewind_iter, cur_pos, NULL);
		snippets_interaction_insert_snippet (snippets_interaction, snippets_db, snippet);
	}

	g_free (trigger);
	g_object_unref (rewind_iter);
	g_object_unref (cur_pos);

}
