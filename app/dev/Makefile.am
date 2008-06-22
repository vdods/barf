##############################################################################
# subdir-objects causes object files to be created in the corresponding subdir
# foreign indicates this is not a GNU package so it won't halt if any
#         GNU-required files (e.g. NEWS) are not found
# 1.4 indicates the minimum automake version to proceed
##############################################################################

AUTOMAKE_OPTIONS = subdir-objects foreign 1.4

##############################################################################
# build targets and extra files to add to the distribution
##############################################################################

.PHONY: \
	dumblang-parser \
	clean-dumblang-parser \
	clean-dumblang-parser-extras \
	force-dumblang-parser \
	\
	dumblang-scanner \
	clean-dumblang-scanner \
	clean-dumblang-scanner-extras \
	force-dumblang-scanner \
	\
	spotless

spotless: clean clean-dumblang-parser-extras clean-dumblang-scanner-extras

clean-dumblang-parser: clean-dumblang-parser-extras
	rm -f $(top_srcdir)/dumblang_parser.cpp
	rm -f $(top_srcdir)/dumblang_parser.hpp

clean-dumblang-parser-extras:
	rm -f $(top_srcdir)/dumblang_parser.states

clean-dumblang-scanner: clean-dumblang-scanner-extras
	rm -f $(top_srcdir)/dumblang_scanner.cpp
	rm -f $(top_srcdir)/dumblang_scanner.hpp

clean-scanner-extras:


bin_PROGRAMS = dumblang

EXTRA_DIST = \
    bootstrap.sh \
	configure.ac

##############################################################################
# general flags
##############################################################################

AM_CPPFLAGS =
LDADD = $(LIBOBJS)

##############################################################################
# dumblang (test app for parser algorithm development)
##############################################################################

dumblang_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/

dumblang_LDADD = \
    $(LDADD) \
    -lcln

dumblang_SOURCES = \
	dumblang_parser.cpp \
	dumblang_parser.trison \
	dumblang_scanner.cpp \
	dumblang_scanner.reflex

if REFLEX_EXISTS
dumblang-scanner: $(top_srcdir)/dumblang_scanner.hpp
$(top_srcdir)/dumblang_scanner.hpp: $(top_srcdir)/dumblang_scanner.cpp
$(top_srcdir)/dumblang_scanner.cpp: $(top_srcdir)/dumblang_scanner.reflex $(REFLEX)
	$(REFLEX) $(top_srcdir)/dumblang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/

force-dumblang-scanner: $(top_srcdir)/dumblang_scanner.reflex $(REFLEX)
	$(REFLEX) $(top_srcdir)/dumblang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/
else # !REFLEX_EXISTS
dumblang-scanner force-dumblang-scanner:
	@echo "!!!"
	@echo "!!! Can't build dumblang scanner without reflex binary."
	@echo "!!! Use --with-reflex configure option."
	@echo "!!!"
endif

if TRISON_EXISTS
dumblang-parser: $(top_srcdir)/dumblang_parser.hpp
$(top_srcdir)/dumblang_parser.hpp: $(top_srcdir)/dumblang_parser.cpp
$(top_srcdir)/dumblang_parser.cpp: $(top_srcdir)/dumblang_parser.trison $(TRISON)
	$(TRISON) $(top_srcdir)/dumblang_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-npda-dot-graph dumblang_parser.dot

force-dumblang-parser: $(top_srcdir)/dumblang_parser.trison $(TRISON)
	$(TRISON) $(top_srcdir)/dumblang_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-npda-dot-graph dumblang_parser.dot
else # !TRISON_EXISTS
dumblang-parser force-dumblang-parser:
	@echo "!!!"
	@echo "!!! Can't build dumblang parser without stable trison binary."
	@echo "!!! Use --with-trison configure option."
	@echo "!!!"
endif

##############################################################################
# all the no-install header files for the applications (is this necessary?)
##############################################################################

noinst_HEADERS = \
	dumblang_parser.hpp \
	dumblang_scanner.hpp
