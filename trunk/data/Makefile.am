desktopdir 	 = $(datadir)/applications
desktop_in_files = stardict.desktop.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

if GNOME_SUPPORT
schemasdir	 = $(sysconfdir)/gconf/schemas
schemas_in_files = stardict.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_Stardict.server.in
server_DATA = $(server_in_files:.server.in=.server)
@INTLTOOL_SERVER_RULE@
endif

man_MANS = stardict.1

if GNOME_SUPPORT
EXTRA_DIST = $(desktop_in_files)	\
	$(server_in_files)		\
	$(schemas_in_files)		\
	$(man_MANS)
else
EXTRA_DIST = $(desktop_in_files)        \
        $(man_MANS)
endif

CLEANFILES = \
	$(server_DATA) \
	$(desktop_DATA)	\
	$(schemas_DATA)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p >/dev/null; \
	                done \
	        fi
else
install-data-local:
endif
