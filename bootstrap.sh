#!/bin/bash

ARGUMENT=${1}
if [ "x${ARGUMENT}" = "x" ]
then
	ARGUMENT=bootstrap
fi

case ${ARGUMENT} in
(bootstrap)
	echo "Auto-generating files necessary to build..."
	echo ""

	# generate aclocal.m4
	aclocal
	# generate config.h.in
	autoheader
	# generate ltmain.sh and others
	libtoolize
	# recursively generate Makefile.in from all Makefile.am
	automake --add-missing
	# generate configure
	autoconf
	;;

(purge)
	echo "Deleting files that can be auto-generated..."
	echo ""
	
	rm -frv aclocal.m4 \
	        autom4te.cache \
	        config.guess \
	        config.h.in \
	        config.sub \
	        configure \
	        depcomp \
	        install-sh \
	        ltmain.sh \
	        missing \
            config.h.in~ \
            ylwrap
	find -name "Makefile.in" -type f -exec rm -fv {} \;
	;;
	
(help)
	echo "Usage: ${0} [bootstrap|purge|help]"
	echo ""
	echo "Commands:"
	echo ""
	echo " bootstrap - Auto-generate files necessary to build."
	echo "     purge - Delete files that can be auto-generated."
	echo "      help - This help message."
	;;
esac

exit 0

