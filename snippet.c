/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet.c
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

#include "snippet.h"



typedef struct _SnippetVariable
{
	/* Variable name. */
	gchar* variable_name;

	/* Variable default value. */
	gchar* default_value;

	/* If the variable is global, it will not await for the user to edit it, but the SnippetDB will
	   fill it with a stored value. Example: username or email are global variables. */
	gboolean is_global;

	/* The number of appearances the variable has in the snippet. */
	gint16 appearances;

	/* A vector with the relative positions for all the appearances of the variable in the
	   snippet default body. */
	guint32* relative_position;
} SnippetVariable;
