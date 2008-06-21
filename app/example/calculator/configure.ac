AC_INIT(BARF, 0.5.20070121, victor.dods@gmail.com, barf)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

# C++ stuff
AC_LANG_CPLUSPLUS
AC_PROG_CXX
# this enables libtool usage for creating libraries
AM_PROG_LIBTOOL
# this determines the machine's byte order
AC_C_BIGENDIAN
# checks for strip for `make install-strip`
AC_CHECK_TOOL(STRIP, strip)
# checks for doxygen for `make doc`
AC_CHECK_TOOL(DOXYGEN, doxygen)
# checks for perl (which seems to be used by doxygen)
AC_CHECK_TOOL(PERL, perl)

##############################################################################
# configure script commandline options
##############################################################################

AC_ARG_ENABLE(
    [debug],
    AC_HELP_STRING(
        [--enable-debug],
        [enable debugging information and disable optimizations (disabled by default)]),
    [ENABLE_DEBUG=$withval],
    [ENABLE_DEBUG="no"])

AC_ARG_ENABLE(
    [warnings-as-errors],
    AC_HELP_STRING(
        [--disable-warnings-as-errors],
        [disables the default behavior of interpreting warnings as errors]),
    [WARNINGS_AS_ERRORS=$enableval],
    [WARNINGS_AS_ERRORS="yes"])

AC_ARG_WITH(
    [arch],
    AC_HELP_STRING(
        [--with-arch=type],
        [compile for a specific architecture type (e.g. i686, athlon, pentium4)]),
    [ARCH_TYPE=$withval],
    [ARCH_TYPE=])

AC_ARG_WITH(
    [reflex],
    AC_HELP_STRING(
        [--with-reflex=full/path/to/reflex],
        [specify a stable, external reflex binary.]),
    [REFLEX=$withval],
    [REFLEX=])

AC_ARG_WITH(
    [trison],
    AC_HELP_STRING(
        [--with-trison=full/path/to/trison],
        [specify a stable, external trison binary.]),
    [TRISON=$withval],
    [TRISON=])

AC_ARG_WITH(
    [targets-path],
    AC_HELP_STRING(
        [--with-targets-path=full/path/to/targets],
        [specify the directory containing reflex and trison targetspec/codespec files (default is ./).]),
    [TARGETS_PATH=$withval],
    [TARGETS_PATH=.])

##############################################################################
# processing for above commandline options
##############################################################################

if test "${ENABLE_DEBUG}" = "yes"; then
    CXXFLAGS="${CXXFLAGS} -g3 -O0"
else
    AC_DEFINE([NDEBUG], [], [Define if compiling without debug code (asserts, etc), otherwise leave undefined])
    CXXFLAGS="${CXXFLAGS} -g0 -O3"
fi

# if warnings should be interpreted as errors, add -Wall to the compiler flags
if test "${WARNINGS_AS_ERRORS}" = "yes"; then
    CXXFLAGS="${CXXFLAGS} -Werror"
fi

if test "${ARCH_TYPE}" != ""; then
    CXXFLAGS="${CXXFLAGS} -march=${ARCH_TYPE}"
fi

AM_CONDITIONAL(REFLEX_EXISTS, [test "${REFLEX}" != ""])
AC_SUBST(REFLEX)

AM_CONDITIONAL(TRISON_EXISTS, [test "${TRISON}" != ""])
AC_SUBST(TRISON)

AC_SUBST(TARGETS_PATH)

##############################################################################
# add some good stuff to the compiler flags
##############################################################################

CXXFLAGS="${CXXFLAGS} -Wall"

##############################################################################
# configure script generation
##############################################################################

AC_OUTPUT(
    Makefile
)
