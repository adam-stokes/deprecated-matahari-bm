# Process this file with autoconf to produce a configure script.
AC_INIT(matahari, 0.0.4)
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h sys/ioctl.h sys/socket.h unistd.h libudev.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT32_T

# Checks for libraries.
PKG_CHECK_MODULES(LIBVIRT, libvirt >= 0.6.2)
PKG_CHECK_MODULES(PCRE, libpcre >= 7.8)
PKG_CHECK_MODULES(UDEV, libudev >= 145)

# Checks for library functions.
AC_CHECK_FUNCS([gethostname inet_ntoa socket])

AC_OUTPUT(Makefile src/Makefile)
