# check required autoconf version
AC_PREREQ([2.59])

# define distribution properties
AC_INIT([hclib], [0.1], [vc8@rice.edu])

# check if srcdir is correct
AC_CONFIG_SRCDIR([inc/hclib.h])

AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.9 tar-ustar no-define foreign dist-bzip2])

AC_COPYRIGHT([])

# configure script revision
AC_REVISION([$Revision: 0.1 $])

AC_PROG_CC
AC_PROG_LIBTOOL
AC_LANG(C)

AC_ARG_WITH([ocr], [AS_HELP_STRING([--with-ocr], [defines the root directory of an ocr installation.])], [OCR_ROOT=$withval], [])
# Use -isystem instead of -I to avoid warnings (converted to errors by
# -Werror) in Rose headers.  For example, they redefine our PACKAGE_*
# macros.
OCR_CPPFLAGS="-isystem $OCR_ROOT/include"
OCR_LDFLAGS="-L$OCR_ROOT/lib"
OCR_LIBS="-locr"

CPPFLAGS_BACKUP=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $OCR_CPPFLAGS"
AC_CHECK_HEADERS([ocr.h], [], [AC_MSG_ERROR([
======================================================================
|    ERROR                                                           |
======================================================================
| OCR not found: use --with-ocr to specify an OCR installation       |
======================================================================
])],
[])

CPPFLAGS=$CPPFLAGS_BACKUP
AC_SUBST(OCR_CPPFLAGS)
AC_SUBST(OCR_LDFLAGS)
AC_SUBST(OCR_LIBS)

# Variable substitutions.
AC_SUBST([ac_aux_dir])
AC_SUBST([abs_top_srcdir])

#
# Config file to process
#
AC_CONFIG_FILES([
Makefile
src/Makefile
])

# Generates and runs config.status, which in turn creates the makefiles 
# and any other files resulting from configuration
AC_OUTPUT
