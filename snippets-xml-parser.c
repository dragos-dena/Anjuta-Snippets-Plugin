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


static gboolean 
snippets_manager_save_native_xml_file (const gchar *snippet_packet_filename, 
                                       GList* snippets, 
                                       const gchar* group_name, 
                                       const gchar* group_description)
{
	/* TODO */
	return FALSE;
}

static gboolean 
snippets_manager_save_gedit_xml_file (const gchar *snippet_packet_filename, 
                                      GList* snippets, 
                                      const gchar* group_name, 
                                      const gchar* group_description)
{
	/* TODO */
	return FALSE;
}

static GList* 
snippets_manager_parse_native_xml_file (const gchar* snippet_packet_filename)
{
	/* TODO */
	return NULL;
}

static GList* 
snippets_manager_parse_gedit_xml_file (const gchar* snippet_packet_filename)
{
	/* TODO */
	return NULL;
}


/**
 * snippets_manager_parse_xml_file:
 * @snippet_packet_filename: The name (path) of the XML file describing the Snippet Group
 * @format_Type: The type of the XML file (see snippets-db.h for the supported types)
 *
 * Parses the given XML file.
 *
 * Returns: A list of snippets (of type Snippet) on success or NULL on failure.
 **/
GList*	
snippets_manager_parse_xml_file (const gchar* snippet_packet_filename,
                                 FormatType format_type)
{
	switch (format_type)
	{
		case NATIVE_FORMAT:
			return snippets_manager_parse_native_xml_file (snippet_packet_filename);
		
		case GEDIT_FORMAT:
			return snippets_manager_parse_gedit_xml_file (snippet_packet_filename);
		
		default:
			return NULL;
	}
}

/**
 * snippets_manager_parse_xml_file:
 * @snippet_packet_filename: The name (path) of the XML file describing the Snippet Group where it should be saved
 * @format_Type: The type of the XML file (see snippets-db.h for the supported types)
 * @snippets: A list of snippets (of type Snippet)
 * @group_name: The name of the group of the given snippets.
 * @group_description: The description of the group of the given snippets.
 *
 * Saves the given snippets to a snippet-packet XML file.
 *
 * Returns: TRUE on success.
 **/
gboolean
snippets_manager_save_xml_file (const gchar* snippet_packet_filename,
                                FormatType format_type,
                                GList* snippets,
                                const gchar* group_name,
                                const gchar* group_description)
{
	switch (format_type)
	{
		case NATIVE_FORMAT:
			return snippets_manager_save_native_xml_file (snippet_packet_filename, 
														  snippets, 
														  group_name, 
														  group_description);
		
		case GEDIT_FORMAT:
			return snippets_manager_save_gedit_xml_file (snippet_packet_filename,
														 snippets,
														 group_name,
														 group_description);
		
		default:
			return FALSE;
	}
}
