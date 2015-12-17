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

#ifndef NAUTILUS_OPEN_ATOM_H
#define NAUTILUS_OPEN_ATOM_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Declarations for the open terminal extension object.  This object will be
 * instantiated by nautilus.  It implements the GInterfaces
 * exported by libnautilus. */


#define NAUTILUS_TYPE_OPEN_ATOM	  (nautilus_open_atom_get_type ())
#define NAUTILUS_OPEN_ATOM(o)	  (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_OPEN_ATOM, NautilusOpenAtom))
#define NAUTILUS_IS_OPEN_ATOM(o)	  (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_OPEN_ATOM))
typedef struct _NautilusOpenAtom      NautilusOpenAtom;
typedef struct _NautilusOpenAtomClass NautilusOpenAtomClass;

struct _NautilusOpenAtom {
	GObject parent_slot;
};

struct _NautilusOpenAtomClass {
	GObjectClass parent_slot;
};

GType nautilus_open_atom_get_type      (void);
void  nautilus_open_atom_register_type (GTypeModule *module);

G_END_DECLS

#endif
