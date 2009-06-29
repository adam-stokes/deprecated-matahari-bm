# Process this file with autoconf to produce a configure script.
AC_INIT(nodereporter, 0.0.1)
AC_CONFIG_SRCDIR([src/nic.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h sys/ioctl.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT32_T

# Checks for libraries.
PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.2.12)
PKG_CHECK_MODULES(HAL, hal >= 0.5.12)
PKG_CHECK_MODULES(LIBVIRT, libvirt >= 0.6.2)
AX_BOOST_BASE([1.37.0])
AX_BOOST_REGEX

# Checks for library functions.
AC_CHECK_FUNCS([gethostname inet_ntoa socket])

AC_OUTPUT(Makefile src/Makefile)
