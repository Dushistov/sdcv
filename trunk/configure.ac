dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)

AC_INIT(sdcv, 0.5, dushistov@mail.ru)
AC_CONFIG_SRCDIR(src/sdcv.cpp)

dnl Don't include maintainer make-rules by default
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE([dist-bzip2])

AM_CONFIG_HEADER(config.h)
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_LANG([C++])
AC_PROG_CXXCPP
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl ================================================================
dnl Gettext stuff.
dnl ================================================================
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.14.1)
GETTEXT_PACKAGE=sdcv
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_ICONV

AC_FUNC_MMAP

AC_CHECK_LIB(z,zlibVersion,LIBS="$LIBS -lz",[AC_MSG_ERROR([zlib not found])])

# Check for readline support
SDCV_CHECK_READLINE

PKG_CHECK_MODULES(SDCV, glib-2.0 >= 2.6.1)

SDCV_CFLAGS="-Wall $SDCV_CFLAGS"

AC_SUBST(SDCV_LIBS)
AC_SUBST(SDCV_CFLAGS)

dnl fill cflags for libstardict
PKG_CHECK_MODULES(LIB_STARDICT, glib-2.0 >= 2.6.1)
AC_SUBST(LIB_STARDICT_CFLAGS)

AC_OUTPUT([
 Makefile 
 m4/Makefile 
 src/lib/Makefile 
 src/Makefile 
 doc/Makefile 
 doc/uk/Makefile
 po/Makefile.in
 tests/Makefile
 tests/stardict-test_dict-2.4.2/Makefile
])

echo "

Type \"make\" to compile.

"
