#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([sqtd], [0.1.0], [www.i_have_no_adress])

AC_CONFIG_SRCDIR([src/sqtd/sqtd.cpp])

AC_CONFIG_FILES([Makefile])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC



# Checks for libraries.

# Checks for header files.
AC_CONFIG_HEADERS([config.h])
AC_CHECK_HEADERS([unistd.h])

AC_CHECK_LIB(pthread, pthread_create)

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T


# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MKTIME

AC_OUTPUT

