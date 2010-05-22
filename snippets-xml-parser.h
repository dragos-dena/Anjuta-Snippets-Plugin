/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-xml-parser.h
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

#include <glib.h>
#include "snippets-group.h"
#include "snippets-db.h"

AnjutaSnippetsGroup* snippets_manager_parse_xml_file (const gchar* snippet_packet_filename,
                                                      FormatType format_type);
gboolean             snippets_manager_save_xml_file  (const gchar* snippet_packet_filename,
                                                      FormatType format_type,
                                                      AnjutaSnippetsGroup* snippets_group);
