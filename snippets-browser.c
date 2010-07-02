/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-browser.c
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

#include "snippets-browser.h"
#include "snippets-group.h"
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>

#define BROWSER_UI      PACKAGE_DATA_DIR"/glade/snippets-browser.ui"

#define ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_BROWSER, SnippetsBrowserPrivate))

struct _SnippetsBrowserPrivate
{
	GtkTreeView *snippets_view;

	SnippetsDB *snippets_db;

	GtkButton *add_button;
	GtkButton *delete_button;
	GtkButton *insert_button;
	GtkToggleButton *edit_button;

	GtkVBox *snippets_view_vbox;
	GtkScrolledWindow *snippets_view_cont;
	
	GtkWidget *snippets_editor_frame;
	GtkWidget *browser_hpaned;

	GtkTreeModel *filter;

	gboolean maximized;

	SnippetsInteraction *snippets_interaction;
};


G_DEFINE_TYPE (SnippetsBrowser, snippets_browser, GTK_TYPE_HBOX);

static void
snippets_browser_destroy (GtkObject *gtk_object)
{
	/* TODO */
}

static void
snippets_browser_dispose (GObject *object)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (object));

	GTK_OBJECT_CLASS (snippets_browser_parent_class)->destroy (GTK_OBJECT (object));
	G_OBJECT_CLASS (snippets_browser_parent_class)->dispose (G_OBJECT (object));
}

static void
snippets_browser_class_init (SnippetsBrowserClass* klass)
{
	GObjectClass *g_object_class = NULL;
	GtkObjectClass *gtk_object_class = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER_CLASS (klass));

	gtk_object_class = GTK_OBJECT_CLASS (klass);
	gtk_object_class->destroy = snippets_browser_destroy;

	g_object_class = G_OBJECT_CLASS (klass);
	g_object_class->dispose = snippets_browser_dispose;

	/* When the selection of the TreeView changes. The object passed here can be an
	   AnjutaSnippet or AnjutaSnippetsGroup depending on the selection. */
	g_signal_new ("current-selection-changed",
	              ANJUTA_TYPE_SNIPPETS_BROWSER,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsBrowserClass, current_selection_changed),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__OBJECT,
	              G_TYPE_NONE,
	              1,
	              G_TYPE_OBJECT,
	              NULL);

	/* The SnippetsBrowser asks for a maximize. If a maximize is possible,
	   the snippets_browser_show_editor should be called. */
	g_signal_new ("maximize-request",
	              ANJUTA_TYPE_SNIPPETS_BROWSER,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsBrowserClass, maximize_request),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE,
	              0,
	              NULL);

	/* Like above, the SnippetsBrowser asks for a unmaximize. */
	g_signal_new ("unmaximize-request",
	              ANJUTA_TYPE_SNIPPETS_BROWSER,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsBrowserClass, unmaximize_request),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE,
	              0,
	              NULL);
	
	g_type_class_add_private (klass, sizeof (SnippetsBrowserPrivate));
}

static void
snippets_browser_init (SnippetsBrowser* snippets_browser)
{
	SnippetsBrowserPrivate* priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
		/* Add the hbox as the child of the snippets browser */

	snippets_browser->priv = priv;
	snippets_browser->show_only_document_language_snippets = TRUE;
	snippets_browser->anjuta_shell = NULL;
	
	/* Initialize the private field */
	priv->snippets_view = NULL;
	priv->snippets_db = NULL;
	priv->add_button = NULL;
	priv->delete_button = NULL;
	priv->insert_button = NULL;
	priv->edit_button = NULL;
	priv->snippets_view_cont = NULL;
	priv->snippets_view_vbox = NULL;
	priv->browser_hpaned = NULL;
	priv->snippets_editor_frame = NULL;
	priv->filter = NULL;
	priv->maximized = FALSE;
	priv->snippets_interaction = NULL;
	
}

/* Handlers */

static void    on_add_button_clicked      (GtkButton *add_button,
                                           gpointer user_data);
static void    on_delete_button_clicked   (GtkButton *delete_button,
                                           gpointer user_data);
static void    on_insert_button_clicked   (GtkButton *insert_button,
                                           gpointer user_data);
static void    on_edit_button_toggled     (GtkToggleButton *edit_button,
                                           gpointer user_data);
static void    on_snippets_view_key_press (GtkWidget *snippets_view,
                                           GdkEventKey *event_key,
                                           gpointer user_data);

/* Private methods */

static void
init_browser_layout (SnippetsBrowser *snippets_browser)
{
	GError *error = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkBuilder *bxml = NULL;
	
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	/* Load the UI file */
	bxml = gtk_builder_new ();
	if (!gtk_builder_add_from_file (bxml, BROWSER_UI, &error))
	{
		g_warning ("Couldn't load preferences ui file: %s", error->message);
		g_error_free (error);
	}

	/* Get the Gtk objects */
	priv->add_button      = GTK_BUTTON (gtk_builder_get_object (bxml, "add_button"));
	priv->delete_button   = GTK_BUTTON (gtk_builder_get_object (bxml, "delete_button"));
	priv->insert_button   = GTK_BUTTON (gtk_builder_get_object (bxml, "insert_button"));
	priv->edit_button     = GTK_TOGGLE_BUTTON (gtk_builder_get_object (bxml, "edit_button"));
	priv->snippets_view_cont = GTK_SCROLLED_WINDOW (gtk_builder_get_object (bxml, "snippets_view_cont"));
	priv->snippets_view_vbox = GTK_VBOX (gtk_builder_get_object (bxml, "snippets_view_vbox"));

	/* Assert the objects */
	g_return_if_fail (GTK_IS_BUTTON (priv->add_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->delete_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->insert_button));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (priv->edit_button));
	g_return_if_fail (GTK_IS_SCROLLED_WINDOW (priv->snippets_view_cont));
	g_return_if_fail (GTK_IS_VBOX (priv->snippets_view_vbox));
	
	/* Add the Snippets View to the scrolled window */
	gtk_container_add (GTK_CONTAINER (priv->snippets_view_cont),
	                   GTK_WIDGET (priv->snippets_view));
	                   
	/* Add the hbox as the child of the snippets browser */
	gtk_widget_unparent (GTK_WIDGET (priv->snippets_view_vbox));
	gtk_box_pack_start (GTK_BOX (snippets_browser),
	                    GTK_WIDGET (priv->snippets_view_vbox),
	                    TRUE,
	                    TRUE,
	                    0);

	/* Init the HPaned and the Frame which are hidden until the editor is shown */
	priv->browser_hpaned = gtk_hpaned_new ();
	priv->snippets_editor_frame = gtk_frame_new (_("Snippets Editor"));
	gtk_paned_pack2 (GTK_PANED (priv->browser_hpaned),
	                 priv->snippets_editor_frame,
	                 TRUE, FALSE);

}

static void
init_browser_handlers (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	g_signal_connect (GTK_OBJECT (priv->snippets_view),
	                  "key-press-event",
	                  GTK_SIGNAL_FUNC (on_snippets_view_key_press),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->add_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_add_button_clicked),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->delete_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_delete_button_clicked),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->insert_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_insert_button_clicked),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->edit_button),
	                  "toggled",
	                  GTK_SIGNAL_FUNC (on_edit_button_toggled),
	                  snippets_browser);
}

static void
snippets_view_name_text_data_func (GtkTreeViewColumn *column,
                                   GtkCellRenderer *renderer,
                                   GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gpointer user_data)
{
	gchar *name = NULL;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));
	
	/* Get the name */
	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_NAME, &name,
	                    -1);
	                    
	g_object_set (renderer, "text", name, NULL);
	g_free (name);
}

static void
snippets_view_name_pixbuf_data_func (GtkTreeViewColumn *column,
                                     GtkCellRenderer *renderer,
                                     GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gpointer user_data)
{
	GObject *cur_object = NULL;
	gchar *stock_id = NULL;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_PIXBUF (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);

	if (ANJUTA_IS_SNIPPET (cur_object))
		stock_id = GTK_STOCK_FILE;
	else
		stock_id = GTK_STOCK_DIRECTORY;

	g_object_unref (cur_object);
	g_object_set (renderer, "stock-id", stock_id, NULL);
}

static void
snippets_view_trigger_data_func (GtkTreeViewColumn *column,
                                 GtkCellRenderer *renderer,
                                 GtkTreeModel *tree_model,
                                 GtkTreeIter *iter,
                                 gpointer user_data)
{
	gchar *trigger = NULL, *trigger_markup = NULL;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_TRIGGER, &trigger,
	                    -1);
	trigger_markup = g_strconcat ("<b>", trigger, "</b>", NULL);
	g_object_set (renderer, "markup", trigger_markup, NULL);

	g_free (trigger);
	g_free (trigger_markup);
}

static void
snippets_view_languages_data_func (GtkTreeViewColumn *column,
                                  GtkCellRenderer *renderer,
                                  GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gpointer user_data)
{ 
	gchar *languages = NULL;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_LANGUAGES, &languages,
	                    -1);

	g_object_set (renderer, "text", languages, NULL);

	g_free (languages);
}

static gboolean
snippets_db_language_filter_func (GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaDocument *doc = NULL;
	const gchar *language = NULL;
	GObject *cur_object = NULL;
	SnippetsBrowserPrivate *priv = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data), FALSE);
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	
	/* If we have the setting to show all snippets or if we are editing,
	    we just return TRUE */
	if (!snippets_browser->show_only_document_language_snippets ||
	    priv->maximized)
	{
		return TRUE;
	}

	/* Get the current object */
	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);

	/* If it's a SnippetsGroup we render it */
	if (ANJUTA_IS_SNIPPETS_GROUP (cur_object))
	{
		g_object_unref (cur_object);
		return TRUE;
	}
	else 
	if (!ANJUTA_IS_SNIPPET (cur_object))
	{
		g_return_val_if_reached (FALSE);
	}

	/* Get the current document manager. If it doesn't exist we show all snippets */
	docman = anjuta_shell_get_interface (snippets_browser->anjuta_shell, 
	                                     IAnjutaDocumentManager, 
	                                     NULL);
	if (docman == NULL)
		return TRUE;
	
	/* Get the current doc and if it isn't an editor show all snippets */
	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	if (!IANJUTA_IS_EDITOR (doc))
		return TRUE;

	/* Get the language */
	language = ianjuta_editor_language_get_language (IANJUTA_EDITOR_LANGUAGE (doc), NULL);
	if (language == NULL)
		return TRUE;
		
	return snippet_has_language (ANJUTA_SNIPPET (cur_object), language);
}

static void
init_snippets_view (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	GtkCellRenderer *text_renderer = NULL, *pixbuf_renderer = NULL;
	GtkTreeViewColumn *column = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (GTK_IS_TREE_VIEW (priv->snippets_view));
	g_return_if_fail (GTK_IS_TREE_MODEL (priv->snippets_db));

	/* Set up the filter */
	priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->snippets_db), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter),
	                                        snippets_db_language_filter_func,
	                                        snippets_browser,
	                                        NULL);
	gtk_tree_view_set_model (priv->snippets_view, priv->filter);


	/* Column 1 - Name */
	column          = gtk_tree_view_column_new ();
	text_renderer   = gtk_cell_renderer_text_new ();
	pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_set_title (column, _("Name"));
	gtk_tree_view_column_pack_start (column, pixbuf_renderer, FALSE);
	gtk_tree_view_column_pack_end (column, text_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, pixbuf_renderer,
	                                         snippets_view_name_pixbuf_data_func,
	                                         snippets_browser, NULL);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
	                                         snippets_view_name_text_data_func,
	                                         snippets_browser, NULL);
	g_object_set (G_OBJECT (column), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->snippets_view, column, -1);

	/* Column 2 - Trigger */
	column        = gtk_tree_view_column_new ();
	text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_title (column, _("Trigger"));
	gtk_tree_view_column_pack_start (column, text_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
	                                         snippets_view_trigger_data_func,
	                                         snippets_browser, NULL);
	g_object_set (G_OBJECT (column), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->snippets_view, column, -1);

	/* Column 3- Languages */
	column        = gtk_tree_view_column_new ();
	text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_title (column, _("Languages"));
	gtk_tree_view_column_pack_start (column, text_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer, 
	                                         snippets_view_languages_data_func,
	                                         snippets_browser, NULL);
	g_object_set (G_OBJECT (column), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->snippets_view, column, -1);
}


/* Public methods */

/**
 * snippets_browser_new:
 *
 * Returns: A new #SnippetsBrowser object.
 */
SnippetsBrowser*
snippets_browser_new (void)
{
	return ANJUTA_SNIPPETS_BROWSER (g_object_new (snippets_browser_get_type (), NULL));
}

/**
 * snippets_browser_load:
 * @snippets_browser: A #SnippetsBrowser object.
 * @snippets_db: A #SnippetsDB object from which the browser should be loaded.
 * @snippets_interaction: A #SnippetsInteraction object which is used for interacting with the editor.
 *
 * Loads the #SnippetsBrowser with snippets that are found in the given database.
 */
void                       
snippets_browser_load (SnippetsBrowser *snippets_browser,
                       SnippetsDB *snippets_db,
                       SnippetsInteraction *snippets_interaction)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	priv->snippets_db   = snippets_db;
	
	/* Set up the Snippets View */
	priv->snippets_view = GTK_TREE_VIEW (gtk_tree_view_new ());
	init_snippets_view (snippets_browser);

	/* Set up the layout */
	init_browser_layout (snippets_browser);

	/* Initialize the snippet handlers */
	init_browser_handlers (snippets_browser);

	priv->snippets_interaction = snippets_interaction;
}

/**
 * snippets_browser_unload:
 * @snippets_browser: A #SnippetsBrowser object.
 * 
 * Unloads the current #SnippetsBrowser object.
 */
void                       
snippets_browser_unload (SnippetsBrowser *snippets_browser)
{
	/* TODO */
}

/**
 * snippets_browser_show_editor:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Shows the editor attached to the browser. Warning: This will take up considerably
 * more space than just having the browser shown.
 */
void                       
snippets_browser_show_editor (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (priv->maximized == FALSE);

	/* Unparent the SnippetsView from the SnippetsBrowser */
	gtk_container_remove (GTK_CONTAINER (snippets_browser),
	                      GTK_WIDGET (priv->snippets_view_vbox));

	/* Add the SnippetsView to the HPaned */
	gtk_paned_pack1 (GTK_PANED (priv->browser_hpaned),
	                 GTK_WIDGET (priv->snippets_view_vbox),
	                 TRUE, FALSE);

	/* Add the HPaned in the SnippetsBrowser */
	gtk_box_pack_start (GTK_BOX (snippets_browser),
	                    priv->browser_hpaned,
	                    TRUE,
	                    TRUE,
	                    0);

	/* TODO - add the SnippetsEditor in the frame */
	
	gtk_widget_show (priv->browser_hpaned);
	gtk_widget_show (priv->snippets_editor_frame);

	priv->maximized = TRUE;

	snippets_browser_refilter_snippets_view (snippets_browser);
	
	gtk_widget_set_sensitive (GTK_WIDGET (priv->insert_button), FALSE);
}


/**
 * snippets_browser_hide_editor:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Hides the editor attached to the browser. 
 */
void                       
snippets_browser_hide_editor (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (priv->maximized == TRUE);
	
	/* TODO - hide the editor */
	gtk_widget_hide (priv->browser_hpaned);
	gtk_widget_hide (priv->snippets_editor_frame);

	/* Remove the SnippetsView from the HPaned */
	gtk_container_remove (GTK_CONTAINER (priv->browser_hpaned),
	                      GTK_WIDGET (priv->snippets_view_vbox));

	/* Remove the HPaned from the SnippetsBrowser */
	g_object_ref (priv->browser_hpaned);
	gtk_container_remove (GTK_CONTAINER (snippets_browser),
	                      GTK_WIDGET (priv->browser_hpaned));

	/* Add the SnippetsView to the SnippetsBrowser */
	gtk_box_pack_start (GTK_BOX (snippets_browser),
	                    GTK_WIDGET (priv->snippets_view_vbox),
	                    TRUE,
	                    TRUE,
	                    0);

	priv->maximized = FALSE;

	snippets_browser_refilter_snippets_view (snippets_browser);

	gtk_widget_set_sensitive (GTK_WIDGET (priv->insert_button), TRUE);
}

/**
 * snippets_browser_refilter_snippets_view:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Refilters the Snippets View (if snippets_browser->show_only_document_language_snippets
 * is TRUE), showing just the snippets of the current document. If the Snippets Browser
 * has the editor shown, it will show all the snippets, regardless of the option.
 */
void                       
snippets_browser_refilter_snippets_view (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

/* Handlers definitions */

static void    
on_add_button_clicked (GtkButton *add_button,
                       gpointer user_data)
{
	/* TODO */
}

static void    
on_delete_button_clicked (GtkButton *delete_button,
                          gpointer user_data)
{
	GtkTreeIter iter;
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	gboolean has_selection = FALSE;
	GObject *cur_object = NULL;
	GtkTreeSelection *selection = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	selection = gtk_tree_view_get_selection (priv->snippets_view);
	has_selection = gtk_tree_selection_get_selected (selection,
	                                                 &priv->filter,
	                                                 &iter);
	if (has_selection)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->filter), &iter,
		                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
		                    -1);

		if (ANJUTA_IS_SNIPPET (cur_object))
		{
			const gchar *trigger_key = NULL, *language = NULL;

			trigger_key = snippet_get_trigger_key (ANJUTA_SNIPPET (cur_object));
			language = snippet_get_any_language (ANJUTA_SNIPPET (cur_object));
			g_return_if_fail (trigger_key != NULL);
			g_return_if_fail (language != NULL);
			
			snippets_db_remove_snippet (priv->snippets_db, trigger_key, language, TRUE);
		}
		else
		{
			/* It's a SnippetsGroup object */
			const gchar *name = NULL;

			name = snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (cur_object));
			g_return_if_fail (name != NULL);
			snippets_db_remove_snippets_group (priv->snippets_db, name);
		}
	}
	
	g_object_unref (cur_object);
}

static void    
on_insert_button_clicked (GtkButton *insert_button,
                          gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreeSelection *selection = NULL;
	GObject *cur_object = NULL;
	GtkTreeIter iter;
	gboolean has_selection = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (priv->snippets_interaction));

	selection = gtk_tree_view_get_selection (priv->snippets_view);
	has_selection = gtk_tree_selection_get_selected (selection,
	                                                 &priv->filter,
	                                                 &iter);
	if (has_selection)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->filter), &iter,
		                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
		                    -1);

		if (!ANJUTA_IS_SNIPPET (cur_object))
			return;

		snippets_interaction_insert_snippet (priv->snippets_interaction,
		                                     G_OBJECT (priv->snippets_db),
		                                     ANJUTA_SNIPPET (cur_object));
	}
}

static void    
on_edit_button_toggled (GtkToggleButton *edit_button,
                        gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	/* Request a maximize/unmaximize (which should be caught by the plugin) */
	if (!priv->maximized)
		g_signal_emit_by_name (G_OBJECT (snippets_browser),
		                       "maximize-request");
	else
		g_signal_emit_by_name (G_OBJECT (snippets_browser),
		                       "unmaximize-request");
	
}

static void    
on_snippets_view_key_press (GtkWidget *snippets_view,
                            GdkEventKey *event_key,
                            gpointer user_data)
{

}
