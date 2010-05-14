/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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

#include "plugin.h"
#include <libanjuta/interfaces/ianjuta-snippets-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <gio/gio.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>

#define ICON_FILE	"anjuta-snippets-manager.png"
#define PREFERENCES_UI	PACKAGE_DATA_DIR"/glade/snippets-manager-preferences.ui"
#define SNIPPETS_MANAGER_PREFERENCES_ROOT "snippets_preferences_root"

static gpointer parent_class;


gboolean
snippet_insert (SnippetsManagerPlugin * plugin, const gchar *keyword)
{

/*	TODO*/
	return TRUE;
}


static gboolean
snippets_manager_activate (AnjutaPlugin * plugin)
{
	DEBUG_PRINT ("%s", "SnippetsManager: Activating SnippetsManager plugin ...");
	return TRUE;
}

static gboolean
snippets_manager_deactivate (AnjutaPlugin * plugin)
{
	DEBUG_PRINT ("%s", "SnippetsManager: Deactivating SnippetsManager plugin ...");	
	return TRUE;
}

static void
snippets_manager_finalize (GObject * obj)
{
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
snippets_manager_dispose (GObject * obj)
{
	SnippetsManagerPlugin *snippets_manager = ANJUTA_PLUGIN_SNIPPETS_MANAGER (obj);
	
	if (snippets_manager->snippets_db != NULL)
	{
		g_object_unref (snippets_manager->snippets_db);
		snippets_manager->snippets_db = NULL;
	}
	
	if (snippets_manager->snippet_interpreter != NULL)
	{
		g_object_unref (snippets_manager->snippet_interpreter);
		snippets_manager->snippet_interpreter = NULL;
	}
	
	if(snippets_manager->snippet_browser != NULL)
	{
		g_object_unref (snippets_manager->snippet_browser);
		snippets_manager->snippet_browser = NULL;
	}
	
	if(snippets_manager->snippet_editor != NULL)
	{
		g_object_unref (snippets_manager->snippet_editor);
		snippets_manager->snippet_editor = NULL;
	}
		
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
snippets_manager_plugin_instance_init (GObject * obj)
{
	SnippetsManagerPlugin *snippets_manager = ANJUTA_PLUGIN_SNIPPETS_MANAGER (obj);
	
	snippets_manager->overwrite_on_conflict = FALSE;
	snippets_manager->show_only_document_language_snippets = FALSE;
	
	/* TODO */
	snippets_manager->snippets_db = snippets_db_new();
	snippets_manager->snippet_interpreter = NULL;
	snippets_manager->snippet_browser = NULL;
	snippets_manager->snippet_editor = NULL;
}

static void
snippets_manager_plugin_class_init (GObjectClass * klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = snippets_manager_activate;
	plugin_class->deactivate = snippets_manager_deactivate;
	klass->dispose = snippets_manager_dispose;
	klass->finalize = snippets_manager_finalize;
}



/* IAnjutaSnippetsManager interface */

static gboolean 
isnippets_manager_iface_insert(IAnjutaSnippetsManager* snippets_manager, const gchar* key, GError** err)
{
	SnippetsManagerPlugin* plugin = ANJUTA_PLUGIN_SNIPPETS_MANAGER (snippets_manager);
	snippet_insert(plugin, key);
	return TRUE;
}

static void
isnippets_manager_iface_init (IAnjutaSnippetsManagerIface *iface)
{
	iface->insert = isnippets_manager_iface_insert;
}



/* IAnjutaPreferences interface */

static void
ipreferences_merge (IAnjutaPreferences* ipref,
					AnjutaPreferences* prefs,
					GError** e)
{
	GError* error = NULL;
	GtkBuilder* bxml = gtk_builder_new ();
	
	if (!gtk_builder_add_from_file (bxml, PREFERENCES_UI, &error))
	{
		g_warning ("Couldn't load preferences ui file: %s", error->message);
		g_error_free (error);
	}

	anjuta_preferences_add_from_builder (prefs, bxml, SNIPPETS_MANAGER_PREFERENCES_ROOT, _("Snippets Manager"),
								 ICON_FILE);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref,
					  AnjutaPreferences* prefs,
					  GError** e)
{
	anjuta_preferences_remove_page (prefs, _("Snippets Manager"));
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}


ANJUTA_PLUGIN_BEGIN (SnippetsManagerPlugin, snippets_manager_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (isnippets_manager, IANJUTA_TYPE_SNIPPETS_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END

ANJUTA_SIMPLE_PLUGIN (SnippetsManagerPlugin, snippets_manager_plugin);
