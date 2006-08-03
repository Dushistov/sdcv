## Process this file with automake to produce Makefile.in
SUBDIRS= lib pixmaps sounds win32

bin_PROGRAMS = stardict

INCLUDES =							                            \
	$(STARDICT_CFLAGS)						                    \
	-DDATADIR=\""$(datadir)"\"	\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"	\
	-DSTARDICT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DSTARDICT_DATA_DIR=\""$(datadir)/stardict"\"


stardict_DEPENDENCIES = lib/libstardict.a
stardict_LDFLAGS = 
stardict_LDADD = $(STARDICT_LIBS) lib/libstardict.a

if !GNOME_SUPPORT
CONFIG_FILE_MODULE = inifile.cpp inifile.hpp
else
CONFIG_FILE_MODULE = gconf_file.cpp gconf_file.hpp
IDL_FILE = GNOME_Stardict.idl

IDL_SOURCES = \
	GNOME_Stardict-stubs.c			\
	GNOME_Stardict-common.c			\
	GNOME_Stardict-skels.c			\
	GNOME_Stardict.h		

STARIDCT_APP_SERVER = \
	stardict-application-server.cpp stardict-application-server.h
endif

stardict_SOURCES = 			                \
	$(IDL_SOURCES)			                \
	$(STARIDCT_APP_SERVER)                  \
	iskeyspressed.hpp            \
	$(ISKEYSPRESSED)                        \
	stardict.cpp stardict.h			        \
	mainwin.cpp mainwin.h	                \
	eggtrayicon.c eggtrayicon.h	            \
	docklet.cpp docklet.h	                \
	conf.cpp conf.h	                        \
	skin.cpp skin.h	                        \
	prefsdlg.cpp prefsdlg.h	                \
	dictmanagedlg.cpp dictmanagedlg.h	    \
	floatwin.cpp floatwin.h	                \
	readword.cpp readword.h	                \
	selection.cpp selection.h	            \
	splash.cpp splash.h		                \
	utils.cpp utils.h                       \
	gtktextviewpango.cpp gtktextviewpango.h \
	pangoview.cpp pangoview.h               \
	articleview.cpp articleview.h           \
	class_factory.cpp class_factory.hpp     \
	config_file.hpp              \
	$(CONFIG_FILE_MODULE)                   \
	x11_iskeyspressed.cpp x11_iskeyspressed.hpp \
	gtk_iskeyspressed.cpp gtk_iskeyspressed.hpp \
	progresswin.cpp progresswin.hpp

if GNOME_SUPPORT
$(IDL_SOURCES): $(IDL_FILE)
	$(ORBIT_IDL) -I $(LIBBONOBO_IDL) -I $(BONOBO_ACTIVATION_IDL) $(IDL_FILE)

idl_DATA = $(IDL_FILE)
idldir = $(datadir)/idl

EXTRA_DIST = 				\
	$(IDL_FILE)

CLEANFILES = $(IDL_SOURCES)
endif
