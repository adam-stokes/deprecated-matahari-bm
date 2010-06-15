dnl ---------------------------------------------------------------------
dnl Matahari autoconf.
dnl ---------------------------------------------------------------------

AC_PREREQ([2.65])
AC_INIT([Matahari], [0.1.0], [matahari@lists.fedorahosted.org])
AC_CONFIG_SRCDIR([src/linux_platform.cpp])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
dnl Make automake keep quiet about wildcards & other GNUmake-isms
AM_INIT_AUTOMAKE([-Wno-portability])

dnl ---------------------------------------------------------------------
dnl check for necessary programs
dnl ---------------------------------------------------------------------

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
CFLAGS="$cflags_save"

AC_PROG_CC_C_O

AC_CHECK_PROG(HASQMFGEN, qmf-gen, "yes")
if test x"$HASQMFGEN" = "x"; then
   AC_MSG_ERROR([Missing QMF schema processor.])
fi

dnl ---------------------------------------------------------------------
dnl Checks for libraries.
dnl ---------------------------------------------------------------------

AC_CHECK_LIB([check], [suite_create], ,
             AC_MSG_ERROR([Missing check libraries.]))
AC_CHECK_LIB([virt], [virConnectOpen], ,
             AC_MSG_ERROR([Missing libvirt libraries.]))
AC_CHECK_LIB([pcre], [pcre_compile], ,
             AC_MSG_ERROR([Missing PCRE libraries.]))
AC_CHECK_LIB([udev], [udev_new], ,
             AC_MSG_ERROR([Missing udev libraries.]))

dnl ---------------------------------------------------------------------
dnl Checks for header files.
dnl ---------------------------------------------------------------------

AC_CHECK_HEADERS([string.h sys/ioctl.h])

dnl ---------------------------------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
dnl ---------------------------------------------------------------------

AC_HEADER_STDBOOL
AC_TYPE_UINT32_T

dnl ---------------------------------------------------------------------
dnl Checks for library functions.
dnl ---------------------------------------------------------------------

AC_CHECK_FUNCS([strdup strerror sysinfo uname])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/tests/Makefile])
AC_OUTPUT
