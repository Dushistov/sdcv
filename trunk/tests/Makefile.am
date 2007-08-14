#noinst_PROGRAMS = validate_stardict normalize

#validate_stardict_SOURCES = validate_stardict.cpp
SUBDIRS = stardict-test_dict-2.4.2

LDADD = @MAKEDICT_LIBS@
INCLUDES = @MAKEDICT_CFLAGS@  -I$(top_builddir) -I$(top_builddir)/src

SH_TESTS = \
	t_list t_use t_interactive \
	t_utf8output t_utf8input \
	t_datadir

TESTS=$(SH_TESTS)
EXTRA_DIST=$(SH_TESTS)
