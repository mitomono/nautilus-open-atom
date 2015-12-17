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
 #include <config.h> /* for GETTEXT_PACKAGE */
#endif

#include "nautilus-open-atom.h"

#include <libnautilus-extension/nautilus-menu-provider.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcmp */
#include <unistd.h> /* for chdir */
#include <sys/stat.h>

static void nautilus_open_atom_instance_init (NautilusOpenAtom      *cvs);
static void nautilus_open_atom_class_init    (NautilusOpenAtomClass *class);

static GType atom_type = 0;

typedef enum {
	/* local files. Always open "conventionally", i.e. cd and spawn. */
	FILE_INFO_LOCAL,
	FILE_INFO_DESKTOP,
	FILE_INFO_OTHER
} AtomFileInfo;

static AtomFileInfo
get_nautilus_file_info (const char *uri)
{
	AtomFileInfo ret;
	char *uri_scheme;

	uri_scheme = g_uri_parse_scheme (uri);
	if (uri_scheme == NULL) {
		ret = FILE_INFO_OTHER;
	} else if (strcmp (uri_scheme, "file") == 0) {
		ret = FILE_INFO_LOCAL;
	} else if (strcmp (uri_scheme, "x-nautilus-desktop") == 0) {
		ret = FILE_INFO_DESKTOP;
	} else {
		ret = FILE_INFO_OTHER;
	}

	g_free (uri_scheme);

	return ret;
}

static GConfClient *gconf_client = NULL;

static inline gboolean
desktop_opens_home_dir (void)
{
	return gconf_client_get_bool (gconf_client,
				      "/apps/nautilus-open-atom/desktop_opens_home_dir",
				      NULL);
}

static inline gboolean
desktop_is_home_dir ()
{
	return gconf_client_get_bool (gconf_client,
				      "/apps/nautilus/preferences/desktop_is_home_dir",
				      NULL);
}

static inline char *
get_gvfs_path_for_uri (const char *uri)
{
	GFile *file;
	char *path;

	file = g_file_new_for_uri (uri);
	path = g_file_get_path (file);
	g_object_unref (file);

	return path;
}

static char *
get_atom_command_for_file_info (NautilusFileInfo *file_info,
				    const char *command_to_run,
				    gboolean remote_atom)
{
	char *uri, *path, *quoted_path;
	char *command;

	uri = nautilus_file_info_get_activation_uri (file_info);

	path = NULL;
	command = NULL;

	switch (get_nautilus_file_info (uri)) {
		case FILE_INFO_LOCAL:
			if (uri != NULL) {
				path = g_filename_from_uri (uri, NULL, NULL);
			}
			break;

		case FILE_INFO_DESKTOP:
			if (desktop_is_home_dir () || desktop_opens_home_dir ()) {
				path = g_strdup (g_get_home_dir ());
			} else {
				path = g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP));
			}
			break;
			/* fall through */
		case FILE_INFO_OTHER:
			if (uri != NULL) {
				/* map back remote URI to local path */
				path = get_gvfs_path_for_uri (uri);
			}
			break;

		default:
			g_assert_not_reached ();
	}

	if (command == NULL && path != NULL) {
		quoted_path = g_shell_quote (path);
		if (command_to_run != NULL) {
			command = g_strdup_printf ("%s %s",command_to_run, quoted_path);
		} else {
			/* interactive shell */
			command = g_strdup_printf ("atom %s", quoted_path);
		}

		g_free (quoted_path);
	}

	g_free (path);
	g_free (uri);

	return command;
}

static char *
make_shell_command (const char *command)
{
	char *quoted, *shell_command;

	quoted = g_shell_quote (command);
	shell_command = g_strconcat ("/bin/sh -c ", quoted, NULL);

	g_free (quoted);
	return shell_command;
}

static void
open_atom_on_screen (const char *command,
			 GdkScreen  *screen)
{
	GAppInfo *app;
	GdkAppLaunchContext *ctx;
	GdkDisplay *display;
	GError *error = NULL;
	char *command_line;

	command_line = make_shell_command (command);
	app = g_app_info_create_from_commandline (command_line, NULL, G_APP_INFO_CREATE_SUPPORTS_URIS, &error);
	g_free (command_line);

	if (app != NULL) {
		display = gdk_screen_get_display (screen);
		ctx = gdk_display_get_app_launch_context (display);
		gdk_app_launch_context_set_screen (ctx, screen);

		g_app_info_launch (app, NULL, G_APP_LAUNCH_CONTEXT (ctx), &error);

		g_object_unref (app);
		g_object_unref (ctx);
	}

	if (error != NULL) {
		g_message ("Could not start application on atom: %s", error->message);
		g_error_free (error);
	}
}


static void
open_atom (NautilusMenuItem *item,
	       NautilusFileInfo *file_info)
{
	char *atom_command, *command_to_run;
	GdkScreen *screen;
	gboolean remote_atom;

	screen = g_object_get_data (G_OBJECT (item), "NautilusOpenAtom::screen");
	command_to_run = g_object_get_data (G_OBJECT (item), "NautilusOpenAtom::command-to-run");
	remote_atom = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (item), "NautilusOpenTerminal::remote-atom"));
	atom_command = get_atom_command_for_file_info (file_info, command_to_run, remote_atom);
	if (atom_command != NULL) {
		open_atom_on_screen (atom_command, screen);
	}
	g_free (atom_command);
}

static void
open_atom_callback (NautilusMenuItem *item,
			NautilusFileInfo *file_info)
{
	open_atom (item, file_info);
}

static NautilusMenuItem *
open_atom_menu_item_new (NautilusFileInfo *file_info,
			     AtomFileInfo  nautilus_file_info,
			     GdkScreen        *screen,
			     const char       *command_to_run,
			     gboolean          remote_atom,
			     gboolean          is_file_item)
{
	NautilusMenuItem *ret;
	char *action_name;
	const char *name;
	const char *tooltip;

	if (command_to_run == NULL) {
		switch (nautilus_file_info) {
			case FILE_INFO_LOCAL:
			case FILE_INFO_OTHER:
				name = _("Open in A_tom");

				if (is_file_item) {
					tooltip = _("Open the currently selected folder in a atom");
				} else {
					tooltip = _("Open the currently open folder in a atom");
				}
				break;

			case FILE_INFO_DESKTOP:
				if (desktop_opens_home_dir ()) {
					name = _("Open A_tom");
					tooltip = _("Open a atom");
				} else {
					name = _("Open in A_tom");
					tooltip = _("Open the currently open folder in a atom");
				}
				break;

			default:
				g_assert_not_reached ();
		}
	}else {
		g_assert_not_reached ();
	}

	if (command_to_run != NULL) {
		action_name = g_strdup_printf (remote_atom ?
					       "NautilusOpenAtom::open_remote_atom_%s" :
					       "NautilusOpenAtom::open_atom_%s",
					       command_to_run);
	} else {
		action_name = g_strdup (remote_atom ?
					"NautilusOpenAtom::open_remote_atom" :
					"NautilusOpenAtom::open_atom");
	}
	ret = nautilus_menu_item_new (action_name, name, tooltip, "atom");
	g_free (action_name);

	g_object_set_data (G_OBJECT (ret),
			   "NautilusOpenAtom::screen",
			   screen);
	g_object_set_data_full (G_OBJECT (ret), "NautilusOpenAtom::command-to-run",
				g_strdup (command_to_run),
				(GDestroyNotify) g_free);
	g_object_set_data (G_OBJECT (ret), "NautilusOpenAtom::remote-atom",
			   GUINT_TO_POINTER (remote_atom));


	g_object_set_data_full (G_OBJECT (ret), "file-info",
				g_object_ref (file_info),
				(GDestroyNotify) g_object_unref);


	g_signal_connect (ret, "activate",
			  G_CALLBACK (open_atom_callback),
			  file_info);


	return ret;
}

static gboolean
atom_locked_down (void)
{
	return gconf_client_get_bool (gconf_client,
                                      "/desktop/gnome/lockdown/disable_command_line",
                                      NULL);
}

/* used to determine for remote URIs whether GVFS is capable of mapping them to ~/.gvfs */
static gboolean
uri_has_local_path (const char *uri)
{
	GFile *file;
	char *path;
	gboolean ret;

	file = g_file_new_for_uri (uri);
	path = g_file_get_path (file);

	ret = (path != NULL);

	g_free (path);
	g_object_unref (file);

	return ret;
}

static GList *
nautilus_open_atom_get_background_items (NautilusMenuProvider *provider,
					     GtkWidget		  *window,
					     NautilusFileInfo	  *file_info)
{
	gchar *uri;
	GList *items;
	NautilusMenuItem *item;
	AtomFileInfo  nautilus_file_info;

        if (atom_locked_down ()) {
            return NULL;
        }

	items = NULL;

	uri = nautilus_file_info_get_activation_uri (file_info);
	nautilus_file_info = get_nautilus_file_info (uri);

	if (nautilus_file_info == FILE_INFO_DESKTOP ||
	    uri_has_local_path (uri) || nautilus_file_info == FILE_INFO_OTHER) {

        item = open_atom_menu_item_new (file_info, nautilus_file_info, gtk_widget_get_screen (window),
						    NULL, TRUE, FALSE);

		items = g_list_append (items, item);
	}

	g_free (uri);

	return items;
}

GList *
nautilus_open_atom_get_file_items (NautilusMenuProvider *provider,
				       GtkWidget            *window,
				       GList                *files)
{
	gchar *uri;
	GList *items;
	NautilusMenuItem *item;
	AtomFileInfo  nautilus_file_info;

        if (atom_locked_down ()) {
            return NULL;
        }

	if (g_list_length (files) != 1 ||
	    (!nautilus_file_info_is_directory (files->data) &&
	     nautilus_file_info_get_file_type (files->data) != G_FILE_TYPE_SHORTCUT &&
	     nautilus_file_info_get_file_type (files->data) != G_FILE_TYPE_MOUNTABLE)) {
		return NULL;
	}

	items = NULL;

	uri = nautilus_file_info_get_activation_uri (files->data);
	nautilus_file_info = get_nautilus_file_info (uri);

	switch (nautilus_file_info) {
		case FILE_INFO_LOCAL:
		case FILE_INFO_OTHER:
			if (uri_has_local_path (uri)) {
				item = open_atom_menu_item_new (files->data, nautilus_file_info, gtk_widget_get_screen (window),
								    NULL, nautilus_file_info == FILE_INFO_DESKTOP, TRUE);
				items = g_list_append (items, item);
			}
			break;

		case FILE_INFO_DESKTOP:
			break;

		default:
			g_assert_not_reached ();
	}

	g_free (uri);

	return items;
}

static void
nautilus_open_atom_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
	iface->get_background_items = nautilus_open_atom_get_background_items;
	iface->get_file_items = nautilus_open_atom_get_file_items;
}

static void
nautilus_open_atom_instance_init (NautilusOpenAtom *cvs)
{
}

static void
nautilus_open_atom_class_init (NautilusOpenAtomClass *class)
{
	g_assert (gconf_client == NULL);
	gconf_client = gconf_client_get_default ();
}

static void
nautilus_open_atom_class_finalize (NautilusOpenAtomClass *class)
{
	g_assert (gconf_client != NULL);
	g_object_unref (gconf_client);
	gconf_client = NULL;
}

GType
nautilus_open_atom_get_type (void)
{
	return atom_type;
}

void
nautilus_open_atom_register_type (GTypeModule *module)
{
	static const GTypeInfo info = {
		sizeof (NautilusOpenAtomClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) nautilus_open_atom_class_init,
		(GClassFinalizeFunc) nautilus_open_atom_class_finalize,
		NULL,
		sizeof (NautilusOpenAtom),
		0,
		(GInstanceInitFunc) nautilus_open_atom_instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) nautilus_open_atom_menu_provider_iface_init,
		NULL,
		NULL
	};

	atom_type = g_type_module_register_type (module,
						     G_TYPE_OBJECT,
						     "NautilusOpenAtom",
						     &info, 0);

	g_type_module_add_interface (module,
				     atom_type,
				     NAUTILUS_TYPE_MENU_PROVIDER,
				     &menu_provider_iface_info);
}
