INCLUDES =						\
	-DG_LOG_DOMAIN=\"Nautilus-Open-atom\"	\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	-Werror                                         \
	$(DISABLE_DEPRECATED)				\
	$(NAUTILUS_CFLAGS)				\
	$(GCONF_CFLAGS)

nautilus_extensiondir=$(NAUTILUS_EXTENSION_DIR)

nautilus_extension_LTLIBRARIES=libnautilus-open-atom.la

libnautilus_open_atom_la_SOURCES = \
	nautilus-open-atom.h       \
	nautilus-open-atom.c       \
	nautilus-open-atom.h       \
	open-atom.c

libnautilus_open_atom_la_LDFLAGS = -module -avoid-version
libnautilus_open_atom_la_LIBADD  = $(NAUTILUS_LIBS) $(GCONF_LIBS)
