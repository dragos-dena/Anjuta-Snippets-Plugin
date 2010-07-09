/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-xml-parser.c
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

#include "snippets-xml-parser.h"
#include "snippet.h"
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>

#define NATIVE_XML_ROOT             "anjuta-snippet-packet"
#define NATIVE_XML_NAME_TAG         "name"
#define NATIVE_XML_DESCRIPTION_TAG  "description"
#define NATIVE_XML_GROUP_TAG        "anjuta-snippets"
#define NATIVE_XML_SNIPPET_TAG      "anjuta-snippet"
#define NATIVE_XML_LANGUAGES_TAG    "languages"
#define NATIVE_XML_VARIABLES_TAG    "variables"
#define NATIVE_XML_VARIABLE_TAG     "variable"
#define NATIVE_XML_CONTENT_TAG      "snippet-content"
#define NATIVE_XML_KEYWORDS_TAG     "keywords"
#define NATIVE_XML_NAME_PROP        "name"
#define NATIVE_XML_GLOBAL_PROP      "is_global"
#define NATIVE_XML_DEFAULT_PROP     "default"
#define NATIVE_XML_TRIGGER_PROP     "trigger"
#define NATIVE_XML_TRUE             "true"
#define NATIVE_XML_FALSE            "false"

#define GLOBAL_VARS_XML_ROOT         "anjuta-global-variables"
#define GLOBAL_VARS_XML_VAR_TAG      "global-variable"
#define GLOBAL_VARS_XML_NAME_PROP    "name"
#define GLOBAL_VARS_XML_COMMAND_PROP "is_command"
#define GLOBAL_VARS_XML_TRUE         "true"
#define GLOBAL_VARS_XML_FALSE        "false"


static gboolean 
snippets_manager_save_native_xml_file (const gchar *snippet_packet_path, 
                                       AnjutaSnippetsGroup* snippets_group)
{
	/* TODO */
	return FALSE;
}

static gboolean 
snippets_manager_save_gedit_xml_file (const gchar *snippet_packet_path, 
                                      AnjutaSnippetsGroup* snippets_group)
{
	/* TODO */
	return FALSE;
}

static AnjutaSnippet*
snippets_manager_parse_native_snippet_node (xmlNodePtr snippet_node)
{
	AnjutaSnippet* snippet = NULL;
	gchar *trigger_key = NULL, *snippet_name = NULL, *snippet_content = NULL, *cur_var_name = NULL, 
	      *cur_var_default = NULL, *cur_var_global = NULL, *keywords_temp = NULL,
	      **keywords_temp_array = NULL, *keyword_temp = NULL, *languages_temp = NULL,
	      **languages_temp_array = NULL, *language_temp = NULL;
	GList *variable_names = NULL, *variable_default_values = NULL,
	      *variable_globals = NULL, *keywords = NULL, *iter = NULL, 
	      *snippet_languages = NULL;
	xmlNodePtr cur_field_node = NULL, cur_variable_node = NULL;
	gboolean cur_var_is_global = FALSE;
	gint i = 0;
	
	/* Assert that the snippet_node is indeed a anjuta-snippet tag */
	g_return_val_if_fail (!g_strcmp0 ((gchar *)snippet_node->name, NATIVE_XML_SNIPPET_TAG), NULL);
	
	/* Get the snippet name, trigger-key and language properties */
	trigger_key = (gchar *)xmlGetProp (snippet_node, (const xmlChar *)NATIVE_XML_TRIGGER_PROP);
	snippet_name = (gchar *)xmlGetProp (snippet_node, (const xmlChar *)NATIVE_XML_NAME_PROP);

	/* Make sure we got all the above properties */
	if (trigger_key == NULL || snippet_name == NULL)
	{
		g_free (trigger_key);
		g_free (snippet_name);
		return NULL;
	}
	
	/* Get the snippet fields (variables, content and keywords) in the following loop */
	cur_field_node = snippet_node->xmlChildrenNode;
	while (cur_field_node != NULL)
	{
		/* We will look in all the variables and save them */
		if (!g_strcmp0 ((gchar*)cur_field_node->name, NATIVE_XML_VARIABLES_TAG))
		{
			cur_variable_node = cur_field_node->xmlChildrenNode;
			while (cur_variable_node != NULL)
			{
				/* Check if it's a variable tag */
				if (g_strcmp0 ((gchar *)cur_variable_node->name, NATIVE_XML_VARIABLE_TAG))
				{
					cur_variable_node = cur_variable_node->next;
					continue;
				}

				/* Get the variable properties */
				cur_var_name = (gchar*)xmlGetProp (cur_variable_node,\
				                                   (const xmlChar*)NATIVE_XML_NAME_PROP);
				cur_var_default = (gchar*)xmlGetProp (cur_variable_node,\
				                                      (const xmlChar*)NATIVE_XML_DEFAULT_PROP);
				cur_var_global = (gchar*)xmlGetProp (cur_variable_node,\
				                                     (const xmlChar*)NATIVE_XML_GLOBAL_PROP);
				if (!g_strcmp0 (cur_var_global, NATIVE_XML_TRUE))
					cur_var_is_global = TRUE;
				else
					cur_var_is_global = FALSE;
				g_free (cur_var_global);

				/* Add the properties to the lists */
				variable_names = g_list_append (variable_names, cur_var_name);
				variable_default_values = g_list_append (variable_default_values, cur_var_default);
				variable_globals = g_list_append (variable_globals, GINT_TO_POINTER (cur_var_is_global));
				
				cur_variable_node = cur_variable_node->next;
			}
		}

		/* Get the actual snippet content */
		if (!g_strcmp0 ((gchar*)cur_field_node->name, NATIVE_XML_CONTENT_TAG))
		{
			snippet_content = (gchar *)xmlNodeGetContent (cur_field_node);
		}

		/* Parse the keywords of the snippet */
		if (!g_strcmp0 ((gchar*)cur_field_node->name, NATIVE_XML_KEYWORDS_TAG))
		{
			keywords_temp = (gchar *)xmlNodeGetContent (cur_field_node);
			keywords_temp_array = g_strsplit (keywords_temp, " ", -1);
			
			i = 0;
			while (keywords_temp_array[i])
			{
				if (g_strcmp0 (keywords_temp_array[i], ""))
				{
					keyword_temp = g_strdup (keywords_temp_array[i]);
					keywords = g_list_append (keywords, keyword_temp);
				}
				i ++;
			}
			
			g_free (keywords_temp);
			g_strfreev (keywords_temp_array);
		}

		/* Parse the languages of the snippet */
		if (!g_strcmp0 ((gchar *)cur_field_node->name, NATIVE_XML_LANGUAGES_TAG))
		{
			languages_temp = (gchar *)xmlNodeGetContent (cur_field_node);
			languages_temp_array = g_strsplit (languages_temp, " ", -1);

			i = 0;
			while (languages_temp_array[i])
			{
				if (g_strcmp0 (languages_temp_array[i], ""))
				{
					language_temp = g_utf8_strdown (languages_temp_array[i], -1);
					snippet_languages = g_list_append (snippet_languages, language_temp); 
				}
				i ++;
			}

			g_free (languages_temp);
			g_strfreev (languages_temp_array);
		}
		
		cur_field_node = cur_field_node->next;
	}
	
	/* Make a new AnjutaSnippet object */
	snippet = snippet_new (trigger_key, 
	                       snippet_languages, 
	                       snippet_name, 
	                       snippet_content, 
	                       variable_names, 
	                       variable_default_values, 
	                       variable_globals, 
	                       keywords);

	/* Free the memory (the data is copied on the snippet constructor) */
	g_free (trigger_key);
	g_free (snippet_name);
	g_free (snippet_content);
	for (iter = g_list_first (variable_names); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	for (iter = g_list_first (variable_default_values); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	g_list_free (variable_names);
	g_list_free (variable_default_values);
	g_list_free (variable_globals);
	for (iter = g_list_first (snippet_languages); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	g_list_free (snippet_languages);
	for (iter = g_list_first (keywords); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	g_list_free (keywords);

	return snippet;
}

static AnjutaSnippetsGroup* 
snippets_manager_parse_native_xml_file (const gchar *snippet_packet_path)
{
	AnjutaSnippetsGroup* snippets_group = NULL;
	AnjutaSnippet* cur_snippet = NULL;
	xmlDocPtr snippet_packet_doc = NULL;
	xmlNodePtr cur_node = NULL, cur_snippet_node = NULL;
	gchar *group_name = NULL;
	
	/* Parse the XML file and load it into a xmlDoc */
	snippet_packet_doc = xmlParseFile (snippet_packet_path);
	g_return_val_if_fail (snippet_packet_doc != NULL, NULL);

	/* Get the root and assert it */
	cur_node = xmlDocGetRootElement (snippet_packet_doc);
	if (cur_node == NULL ||\
	    g_strcmp0 ((gchar *)cur_node->name, NATIVE_XML_ROOT))
	{
		xmlFreeDoc (snippet_packet_doc);
		return NULL;
	}

	/* Get the name and description fields */
	cur_node = cur_node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		/* Get the SnippetsGroup name */
		if (!g_strcmp0 ((gchar*)cur_node->name, NATIVE_XML_NAME_TAG))
		{
			group_name = g_strdup ((gchar *)xmlNodeGetContent (cur_node));
		}
		
		cur_node = cur_node->next;
	}

	/* Make a new AnjutaSnippetsGroup object */
	snippets_group = snippets_group_new (snippet_packet_path, 
	                                     group_name);
	g_free (group_name);

	/* Parse the snippets in the XML file */
	cur_node = xmlDocGetRootElement (snippet_packet_doc);
	cur_node = cur_node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		if (!g_strcmp0 ((gchar *)cur_node->name, NATIVE_XML_GROUP_TAG))
		{
			/* We will iterate over the snippets with cur_snippet_node */
			cur_snippet_node = cur_node->xmlChildrenNode;

			while (cur_snippet_node != NULL)
			{
				/* Make sure it's a snippet node */
				if (g_strcmp0 ((gchar *)cur_snippet_node->name, NATIVE_XML_SNIPPET_TAG))
				{
					cur_snippet_node = cur_snippet_node->next;
					continue;
				}

				/* Get a new AnjutaSnippet object from the current node */
				cur_snippet = snippets_manager_parse_native_snippet_node (cur_snippet_node);

				/* If we have a valid snippet, add it to the snippet group */
				if (cur_snippet != NULL)
				{
					snippets_group_add_snippet (snippets_group, cur_snippet);
				}
				
				cur_snippet_node = cur_snippet_node->next;
			}
			break;
		}
		
		cur_node = cur_node->next;
	}
	
	xmlFreeDoc (snippet_packet_doc);

	
	return snippets_group;
}

static AnjutaSnippetsGroup*
snippets_manager_parse_gedit_xml_file (const gchar* snippet_packet_path)
{
	/* TODO */
	return NULL;
}


/**
 * snippets_manager_parse_xml_file:
 * @snippet_packet_path: The path of the XML file describing the Snippet Group
 * @format_Type: The type of the XML file (see snippets-db.h for the supported types)
 *
 * Parses the given XML file.
 *
 * Returns: A #AnjutaSnippetsGroup object on success or NULL on failure.
 **/
AnjutaSnippetsGroup*	
snippets_manager_parse_snippets_xml_file (const gchar* snippet_packet_path,
                                          FormatType format_type)
{
	switch (format_type)
	{
		case NATIVE_FORMAT:
			return snippets_manager_parse_native_xml_file (snippet_packet_path);
		
		case GEDIT_FORMAT:
			return snippets_manager_parse_gedit_xml_file (snippet_packet_path);
		
		default:
			return NULL;
	}
}

/**
 * snippets_manager_parse_xml_file:
 * @snippet_packet_path: The path of the XML file describing the Snippet Group where it should be saved
 * @format_Type: The type of the XML file (see snippets-db.h for the supported types)
 * @snippets_group: A #AnjutaSnippetsGroup object.
 *
 * Saves the given snippets to a snippet-packet XML file.
 *
 * Returns: TRUE on success.
 **/
gboolean
snippets_manager_save_snippets_xml_file (const gchar* snippet_packet_path,
                                         FormatType format_type,
                                         AnjutaSnippetsGroup* snippets_group)
{
	switch (format_type)
	{
		case NATIVE_FORMAT:
			return snippets_manager_save_native_xml_file (snippet_packet_path, 
                                                          snippets_group);
		
		case GEDIT_FORMAT:
			return snippets_manager_save_gedit_xml_file (snippet_packet_path,
                                                         snippets_group);
		
		default:
			return FALSE;
	}
}

/**
 * snippets_manager_parse_variables_xml_file:
 * @global_vars_path: A path with a XML file describing snippets global variables.
 * @snippets_db: A #SnippetsDB object where the global variables should be loaded.
 *
 * Loads the global variables from the given XML file in the given #SnippetsDB object.
 *
 * Returns: TRUE on success.
 */
gboolean snippets_manager_parse_variables_xml_file (const gchar* global_vars_path,
                                                    SnippetsDB* snippets_db)
{
	xmlDocPtr global_vars_doc = NULL;
	xmlNodePtr cur_var_node = NULL;
	gchar *cur_var_name = NULL, *cur_var_is_command = NULL, *cur_var_content = NULL;
	gboolean cur_var_is_command_bool = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (global_vars_path != NULL, FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);

	/* Parse the XML file */
	global_vars_doc = xmlParseFile (global_vars_path);
	g_return_val_if_fail (global_vars_doc != NULL, FALSE);

	/* Get the root and assert it */
	cur_var_node = xmlDocGetRootElement (global_vars_doc);
	if (cur_var_node == NULL ||\
	    g_strcmp0 ((gchar *)cur_var_node->name, GLOBAL_VARS_XML_ROOT))
	{
		xmlFreeDoc (global_vars_doc);
		return FALSE;
	}

	/* Get the name and description fields */
	cur_var_node = cur_var_node->xmlChildrenNode;
	while (cur_var_node != NULL)
	{
		/* Get the current Snippet Global Variable */
		if (!g_strcmp0 ((gchar*)cur_var_node->name, GLOBAL_VARS_XML_VAR_TAG))
		{
			/* Get the name, is_command properties and the content */
			cur_var_name = (gchar*)xmlGetProp (cur_var_node,\
		                                       (const xmlChar*)GLOBAL_VARS_XML_NAME_PROP);
			cur_var_is_command = (gchar*)xmlGetProp (cur_var_node,\
		                                       (const xmlChar*)GLOBAL_VARS_XML_COMMAND_PROP);
		    cur_var_content = g_strdup ((gchar *)xmlNodeGetContent (cur_var_node));
			if (!g_strcmp0 (cur_var_is_command, GLOBAL_VARS_XML_TRUE))
				cur_var_is_command_bool = TRUE;
			else
				cur_var_is_command_bool = FALSE;

			/* Add the Global Variable to the Snippet Database */
			snippets_db_add_global_variable (snippets_db,
			                                 cur_var_name,
			                                 cur_var_content,
			                                 cur_var_is_command_bool,
			                                 TRUE);
			
		    g_free (cur_var_content);
		    g_free (cur_var_name);
		    g_free (cur_var_is_command);
		}
		
		cur_var_node = cur_var_node->next;
	}

	return TRUE;
}

/**
 * snippets_manager_save_variables_xml_file:
 * @global_vars_path: A path with a XML file describing snippets global variables.
 * @variables_name: A #GList with the name of the variables.
 * @variables_values: A #GList with the values of the variables.
 * @variables_shell_commands: A #Glist with #gboolean values showing if the value
 *                            of the given variable is a shell command. 
 *
 * Saves the given snippets global variables in a XML file at the given path.
 *
 * Returns: TRUE on success.
 */
gboolean snippets_manager_save_variables_xml_file (const gchar* global_variables_path,
                                                   GList* variables_names,
                                                   GList* variables_values,
                                                   GList* variables_shell_commands)
{
	/* TODO */
	return FALSE;
}
