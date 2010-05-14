/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-db.h
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

#ifndef __SNIPPETS_DB_H__
#define __SNIPPETS_DB_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "snippet.h"

G_BEGIN_DECLS

typedef struct _SnippetsDB SnippetsDB;
typedef struct _SnippetsDBClass SnippetsDBClass;

#define ANJUTA_TYPE_SNIPPETS_DB            (snippets_db_get_type ())
#define ANJUTA_SNIPPETS_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_SNIPPETS_DB_TYPE, SnippetsDB))
#define ANJUTA_SNIPPETS_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_SNIPPETS_DB_TYPE, SnippetsDBClass))
#define ANJUTA_IS_SNIPPETS_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_SNIPPETS_DB_TYPE))
#define ANJUTA_IS_SNIPPETS_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_SNIPPETS_DB_TYPE))

struct _SnippetsDB
{
	GObject object;

	/*< private >*/
	/* A tree model to be used by the Snippet Browser. */
	GtkTreeStore* snippets_tree;
	
	/* A binary balanced tree with the trigger keys for searching purposes. */
	GTree* trigger_keys_tree;
	
	/* A binary balanced tree with the keywords for searching purposes */
	GTree* keywords_tree;
	
	/* A hashtable with the snippet-key's as keys and pointers to the aproppiate Snippet as value */
	GHashTable* snippet_keys_map;
	
	/* A list of Snippet Group's. Here the actual Snippet's are stored. */
	GList* snippet_groups;
};

struct _SnippetsDBClass
{
	GObjectClass klass;

};

typedef enum
{
	NATIVE_FORMAT = 0,
	GEDIT_FORMAT
} FormatType;


GType			snippets_db_get_type				(void) G_GNUC_CONST;
SnippetsDB*		snippets_db_new						(void);
gint32			snippets_db_load_file				(SnippetsDB* snippets_db,
													 const gchar* snippet_packet_file_path,
													 gboolean overwrite,
													 GList* conflicting_snippets,
													 FormatType format_type);
gboolean		snippets_db_save_file				(SnippetsDB* snippets_db,
													 gint32 snippet_packet_loaded_id);
gboolean		snippets_db_save_all				(SnippetsDB* snippets_db);
GList*			snippets_db_search					(SnippetsDB* snippets_db,
													 const gchar* search_string,
													 const gchar* snippet_language,
													 guint16 maximum_results);
gboolean		snippets_db_add_snippet				(SnippetsDB* snippets_db,
													 AnjutaSnippet* added_snippet,
													 gboolean overwrite);
AnjutaSnippet*	snippets_db_get_snippet				(SnippetsDB* snippets_db,
													 const gchar* snippet_key);
gboolean		snippets_db_remove_snippet			(SnippetsDB* snippets_db,
													 const gchar* snippet_key);
gboolean		snippets_db_add_snippet_group		(SnippetsDB* snippets_db,
													 const gchar* group_name,
													 const gchar* group_description);
gboolean		snippets_db_remove_snippet_group	(SnippetsDB* snippets_db,
													 const gchar* group_name);	 
GtkTreeModel*	snippets_db_get_tree_model			(SnippetsDB* snippets_db,
													 const gchar* snippets_language);

G_END_DECLS

#endif /* __SNIPPETS_DB_H__ */
