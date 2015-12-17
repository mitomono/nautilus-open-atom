/*
 *  nautilus-open-atom.c
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  Author: Adrian Lara Roldan <mitomono@gmail.com>
 *
 *	year: 2016
 *
 *	based in extension nautilus-open-terminal
 *	Author: Christian Neumair <chris@gnome-de.org>
 *	https://github.com/GNOME/nautilus-open-terminal
 *
 */

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#include "nautilus-open-atom.h"

#include <gconf/gconf-client.h>
#include <libintl.h>

static GType type_list[1];

void
nautilus_module_initialize (GTypeModule *module)
{
	g_print ("Initializing nautilus-open-atom extension\n");

	nautilus_open_atom_register_type (module);
	type_list[0] = NAUTILUS_TYPE_OPEN_ATOM;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

        gconf_client_add_dir(gconf_client_get_default(),
                             "/desktop/gnome/lockdown",
                             0,
                             NULL);
}

void
nautilus_module_shutdown (void)
{
	g_print ("Shutting down nautilus-open-atom extension\n");
}

void
nautilus_module_list_types (const GType **types,
			    int          *num_types)
{
	*types = type_list;
	*num_types = G_N_ELEMENTS (type_list);
}
