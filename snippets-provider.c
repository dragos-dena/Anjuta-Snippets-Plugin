/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-provider.c
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

#include <libanjuta/interfaces/ianjuta-provider.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include "snippets-provider.h"
#include "snippet.h"
#include "snippets-group.h"


#define TRIGGER_RELEVANCE        1000
#define NAME_RELEVANCE           1000
#define FIRST_KEYWORD_RELEVANCE  100
#define KEYWORD_RELEVANCE_DEC    8

#define ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                                   ANJUTA_TYPE_SNIPPETS_PROVIDER,\
                                                   SnippetsProviderPrivate))

struct _SnippetsProviderPrivate
{
	SnippetsDB *snippets_db;
	SnippetsInteraction *snippets_interaction;
	
};


/* IAnjutaProvider methods declaration */

static void             snippets_provider_iface_init     (IAnjutaProviderIface* iface);
static void             snippets_provider_populate       (IAnjutaProvider *self,
                                                          IAnjutaIterable *cursor,
                                                          GError **error);
static IAnjutaIterable* snippets_provider_get_start_iter (IAnjutaProvider *self,
                                                          GError **error);
static void             snippets_provider_activate       (IAnjutaProvider *self,
                                                          IAnjutaIterable *iter,
                                                          gpointer data,
                                                          GError **error);
static const gchar*     snippets_provider_get_name       (IAnjutaProvider *self,
                                                          GError **error);

G_DEFINE_TYPE_WITH_CODE (SnippetsProvider,
			 snippets_provider,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_PROVIDER, snippets_provider_iface_init))


static void
snippets_provider_dispose (GObject *obj)
{
	/* TODO */
}

static void
snippets_provider_init (SnippetsProvider *obj)
{
	SnippetsProviderPrivate *priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (obj);

	priv->snippets_db          = NULL;
	priv->snippets_interaction = NULL;

	obj->anjuta_shell = NULL;

}

static void
snippets_provider_class_init (SnippetsProviderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippets_provider_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippets_provider_dispose;
	g_type_class_add_private (klass, sizeof (SnippetsProviderPrivate));	

}

static void 
snippets_provider_iface_init (IAnjutaProviderIface* iface)
{
	iface->populate       = snippets_provider_populate;
	iface->get_start_iter = snippets_provider_get_start_iter;
	iface->activate       = snippets_provider_activate;
	iface->get_name       = snippets_provider_get_name;

}

/* Private methods */

static IAnjutaEditorAssistProposal*
get_proposal_for_snippet (AnjutaSnippet *snippet)
{
	return NULL;
}

/* Public methods */

SnippetsProvider*
snippets_provider_new (SnippetsDB *snippets_db,
                       SnippetsInteraction *snippets_interaction)
{
	SnippetsProvider *snippets_provider = NULL;
	SnippetsProviderPrivate *priv = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction), NULL);

	snippets_provider = ANJUTA_SNIPPETS_PROVIDER (g_object_new (snippets_provider_get_type (), NULL));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);

	priv->snippets_db          = snippets_db;
	priv->snippets_interaction = snippets_interaction;

	return snippets_provider;
}

void
snippets_provider_request (SnippetsProvider *snippets_provider)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	g_return_if_fail (ANJUTA_IS_SHELL (snippets_provider->anjuta_shell));

	/* TODO */
}
/* IAnjutaProvider methods declarations */

static void
snippets_provider_populate (IAnjutaProvider *self,
                            IAnjutaIterable *cursor,
                            GError **error)
{

}

static IAnjutaIterable*
snippets_provider_get_start_iter (IAnjutaProvider *self,
                                  GError **error)
{
	return NULL;
}

static void
snippets_provider_activate (IAnjutaProvider *self,
                            IAnjutaIterable *iter,
                            gpointer data,
                            GError **error)
{

}

static const gchar*
snippets_provider_get_name (IAnjutaProvider *self,
                            GError **error)
{
	return _("Code Snippets");
}
