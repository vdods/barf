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
	calculator-parser \
	clean-calculator-parser \
	clean-calculator-parser-extras \
	force-calculator-parser \
	\
	calculator-scanner \
	clean-calculator-scanner \
	clean-calculator-scanner-extras \
	force-calculator-scanner \
	\
	spotless

spotless: clean clean-calculator-parser-extras clean-calculator-scanner-extras

clean-calculator-parser: clean-calculator-parser-extras
	rm -f $(top_srcdir)/calculator_parser.cpp
	rm -f $(top_srcdir)/calculator_parser.hpp

clean-calculator-parser-extras:
	rm -f $(top_srcdir)/calculator_parser.states

clean-calculator-scanner: clean-calculator-scanner-extras
	rm -f $(top_srcdir)/calculator_scanner.cpp
	rm -f $(top_srcdir)/calculator_scanner.hpp

clean-scanner-extras:


bin_PROGRAMS = calculator

EXTRA_DIST = \
    bootstrap.sh \
	configure.ac

##############################################################################
# general flags
##############################################################################

AM_CPPFLAGS =
LDADD = $(LIBOBJS)

##############################################################################
# calculator (example app)
##############################################################################

calculator_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/

calculator_LDADD = \
    $(LDADD) \
    -lcln

calculator_SOURCES = \
	calculator_main.cpp \
	calculator_parser.cpp \
	calculator_parser.trison \
	calculator_scanner.cpp \
	calculator_scanner.reflex

if REFLEX_EXISTS
calculator-scanner: $(top_srcdir)/calculator_scanner.hpp
$(top_srcdir)/calculator_scanner.hpp: $(top_srcdir)/calculator_scanner.cpp
$(top_srcdir)/calculator_scanner.cpp: $(top_srcdir)/calculator_scanner.reflex $(REFLEX)
	$(REFLEX) $(top_srcdir)/calculator_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/

force-calculator-scanner: $(top_srcdir)/calculator_scanner.reflex $(REFLEX)
	$(REFLEX) $(top_srcdir)/calculator_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/
else # !REFLEX_EXISTS
calculator-scanner force-calculator-scanner:
	@echo "!!!"
	@echo "!!! Can't build calculator scanner without reflex binary."
	@echo "!!! Use --with-reflex configure option."
	@echo "!!!"
endif

if TRISON_EXISTS
calculator-parser: $(top_srcdir)/calculator_parser.hpp
$(top_srcdir)/calculator_parser.hpp: $(top_srcdir)/calculator_parser.cpp
$(top_srcdir)/calculator_parser.cpp: $(top_srcdir)/calculator_parser.trison $(TRISON)
	$(TRISON) $(top_srcdir)/calculator_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/calculator_parser --generate-states-file $(top_srcdir)/calculator_parser.states

force-calculator-parser: $(top_srcdir)/calculator_parser.trison $(TRISON)
	$(TRISON) $(top_srcdir)/calculator_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/calculator_parser --generate-states-file $(top_srcdir)/calculator_parser.states
else # !TRISON_EXISTS
calculator-parser force-calculator-parser:
	@echo "!!!"
	@echo "!!! Can't build calculator parser without stable trison binary."
	@echo "!!! Use --with-trison configure option."
	@echo "!!!"
endif

##############################################################################
# all the no-install header files for the applications (is this necessary?)
##############################################################################

noinst_HEADERS = \
	calculator.hpp \
	calculator_parser.hpp \
	calculator_scanner.hpp
