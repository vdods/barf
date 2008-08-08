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

spotless: clean clean-calculator-parser clean-calculator-scanner clean-calculator-parser-extras clean-calculator-scanner-extras

clean-calculator-parser: clean-calculator-parser-extras
	rm -f $(top_srcdir)/calculator_parser.cpp
	rm -f $(top_srcdir)/calculator_parser.hpp

clean-calculator-parser-extras:
	rm -f $(top_srcdir)/calculator_parser.dpda.dot
	rm -f $(top_srcdir)/calculator_parser.dpda.png
	rm -f $(top_srcdir)/calculator_parser.dpda.states
	rm -f $(top_srcdir)/calculator_parser.npda.dot
	rm -f $(top_srcdir)/calculator_parser.npda.png

clean-calculator-scanner: clean-calculator-scanner-extras
	rm -f $(top_srcdir)/calculator_scanner.cpp
	rm -f $(top_srcdir)/calculator_scanner.hpp

clean-calculator-scanner-extras:
	rm -f $(top_srcdir)/calculator_scanner.dfa.dot
	rm -f $(top_srcdir)/calculator_scanner.dfa.png
	rm -f $(top_srcdir)/calculator_scanner.nfa.dot
	rm -f $(top_srcdir)/calculator_scanner.nfa.png

# installed into $(bindir)
bin_PROGRAMS = calculator

EXTRA_DIST = \
    bootstrap.sh \
	configure.ac \
	INSTALL \
	LICENSE \
	README

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
$(top_srcdir)/calculator_scanner.cpp: $(top_srcdir)/calculator_scanner.reflex
	$(REFLEX) $(top_srcdir)/calculator_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-nfa-dot-graph calculator_scanner.nfa.dot --generate-dfa-dot-graph calculator_scanner.dfa.dot

force-calculator-scanner:
	$(REFLEX) $(top_srcdir)/calculator_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-nfa-dot-graph calculator_scanner.nfa.dot --generate-dfa-dot-graph calculator_scanner.dfa.dot
else # !REFLEX_EXISTS
calculator-scanner force-calculator-scanner:
	@echo "!!!"
	@echo "!!! Can't build calculator scanner without reflex binary."
	@echo "!!! Use --with-reflex configure option."
	@echo "!!!"
endif

if DOT_EXISTS
$(top_srcdir)/calculator_scanner.dfa.png: $(top_srcdir)/calculator_scanner.dfa.dot
	$(DOT) $(top_srcdir)/calculator_scanner.dfa.dot -Tpng -o $(top_srcdir)/calculator_scanner.dfa.png

$(top_srcdir)/calculator_scanner.nfa.png: $(top_srcdir)/calculator_scanner.nfa.dot
	$(DOT) $(top_srcdir)/calculator_scanner.nfa.dot -Tpng -o $(top_srcdir)/calculator_scanner.nfa.png
endif

if TRISON_EXISTS
calculator-parser: $(top_srcdir)/calculator_parser.hpp
$(top_srcdir)/calculator_parser.hpp: $(top_srcdir)/calculator_parser.cpp
$(top_srcdir)/calculator_parser.cpp: $(top_srcdir)/calculator_parser.trison
	$(TRISON) $(top_srcdir)/calculator_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-npda-dot-graph calculator_parser.npda.dot --generate-dpda-dot-graph calculator_parser.dpda.dot --generate-dpda-states-file calculator_parser.dpda.states

force-calculator-parser:
	$(TRISON) $(top_srcdir)/calculator_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-npda-dot-graph calculator_parser.npda.dot --generate-dpda-dot-graph calculator_parser.dpda.dot --generate-dpda-states-file calculator_parser.dpda.states
else # !TRISON_EXISTS
calculator-parser force-calculator-parser:
	@echo "!!!"
	@echo "!!! Can't build calculator parser without stable trison binary."
	@echo "!!! Use --with-trison configure option."
	@echo "!!!"
endif

if DOT_EXISTS
$(top_srcdir)/calculator_parser.dpda.png: $(top_srcdir)/calculator_parser.dpda.dot
	$(DOT) $(top_srcdir)/calculator_parser.dpda.dot -Tpng -o $(top_srcdir)/calculator_parser.dpda.png

$(top_srcdir)/calculator_parser.npda.png: $(top_srcdir)/calculator_parser.npda.dot
	$(DOT) $(top_srcdir)/calculator_parser.npda.dot -Tpng -o $(top_srcdir)/calculator_parser.npda.png
endif

##############################################################################
# all the no-install header files for the applications (is this necessary?)
##############################################################################

noinst_HEADERS = \
	calculator.hpp \
	calculator_parser.hpp \
	calculator_scanner.hpp
