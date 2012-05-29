#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(GamesBot, 0.1, rydencillo@gmail.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([include/gamesbot.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile src/Makefile games/Makefile])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([])

# Checks for functions
AC_CHECK_FUNCS([])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.

# Set the games dir, extracted from GStreamer autoconf scripts
dnl AC_DEFINE PLUGINDIR to the full location where plug-ins will be installed
dnl AC_SUBST plugindir, to be used in Makefile.am's
m4_include([as-ac-expand.m4])
AS_AC_EXPAND(GAMESDIR, ${datadir}/$PACKAGE_TARNAME/games)
AC_DEFINE_UNQUOTED(GAMESDIR, "$GAMESDIR",
  [directory where games are located])
AC_MSG_NOTICE([Using $GAMESDIR as the games install location])

dnl plugin directory configure-time variable for use in Makefile.am
gamesdir="\$(datadir)/$PACKAGE_TARNAME/games"
AC_SUBST(gamesdir)


AC_OUTPUT
