/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-import-export.c
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

#include "snippets-import-export.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

void 
snippets_manager_import_snippets (SnippetsDB *snippets_db)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

}

void snippets_manager_export_snippets (SnippetsDB *snippets_db)
{

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

}