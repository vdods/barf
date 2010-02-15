AC_INIT(BARF, 0.9.20100116, victor.dods@gmail.com, barf)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

# hack to get $prefix to actually contain the real value
test "$prefix" = "NONE" && prefix="$ac_default_prefix"

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
# checks for dot (used to turn .dot graphs into images)
AC_CHECK_TOOL(DOT, dot)

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
    [stable-reflex],
    AC_HELP_STRING(
        [--with-stable-reflex=full/path/to/reflex.stable],
        [specify a stable, external reflex binary to compile BARF's own reflex-based scanners -- make sure not to specify the reflex build target of this package.  This option is really only useful if you're developing BARF itself.]),
    [STABLE_REFLEX=$withval],
    [STABLE_REFLEX=])

AC_ARG_WITH(
    [stable-trison],
    AC_HELP_STRING(
        [--with-stable-trison=full/path/to/trison.stable],
        [specify a stable, external trison.unstable binary to compile BARF's own trison-based parsers -- make sure not to specify the trison build target of this package.  This option is really only useful if you're developing BARF itself.]),
    [STABLE_TRISON=$withval],
    [STABLE_TRISON=])

##############################################################################
# processing for above commandline options and AC_CHECK_TOOL applications
##############################################################################

if test "${ENABLE_DEBUG}" = "yes"; then
    CXXFLAGS="${CXXFLAGS} -g3 -O0"
else
    AC_DEFINE(
        [NDEBUG],
        [],
        [Define if compiling without debug code (asserts, etc), otherwise leave undefined])
    CXXFLAGS="${CXXFLAGS} -g0 -O3"
fi

# if warnings should be interpreted as errors, add -Werror to the compiler flags
if test "${WARNINGS_AS_ERRORS}" = "yes"; then
    CXXFLAGS="${CXXFLAGS} -Werror"
fi

if test "${ARCH_TYPE}" != ""; then
    CXXFLAGS="${CXXFLAGS} -mtune=${ARCH_TYPE}"
fi

AM_CONDITIONAL(DOT_EXISTS, [test "${DOT}" != ""])
AC_SUBST(DOT)

AM_CONDITIONAL(STABLE_REFLEX_EXISTS, [test "${STABLE_REFLEX}" != ""])
AC_SUBST(STABLE_REFLEX)

AM_CONDITIONAL(STABLE_TRISON_EXISTS, [test "${STABLE_TRISON}" != ""])
AC_SUBST(STABLE_TRISON)

##############################################################################
# add BARFDATADIR (the absolute path to where the targets/ dir and other stuff
# is installed) to config.h
##############################################################################

temp=${datadir}/barf
eval temp="\"$temp\""
eval temp="\"$temp\""
AC_DEFINE_UNQUOTED([BARFDATADIR], "$temp", [Define as the absolute path of the place the data will be installed (e.g. "/usr/local/share/barf").  This value will be compiled into the reflex and trison binaries as containing the targets/ directory, which is the default targets search path.  Leave undefined if no default targets search path is desired.])

##############################################################################
# add some good stuff to the compiler flags
##############################################################################

CXXFLAGS="${CXXFLAGS} -Wall"

##############################################################################
# configure script generation
##############################################################################

AC_OUTPUT(
    Doxyfile
    Makefile
)
