AC_PREREQ(2.61)
AC_INIT(tcpauth, 0.01, habets@google.com)
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/wrap.c])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

CFLAGS="-std=gnu11 -Wall $CFLAGS"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debug
  C Compiler.....: $CC $CFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
