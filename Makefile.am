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
	barf-commonlang-scanner \
	force-barf-commonlang-scanner \
	barf-preprocessor-parser \
	force-barf-preprocessor-parser \
	barf-preprocessor-scanner \
	force-barf-preprocessor-scanner \
	barf-regex-parser \
	force-barf-regex-parser \
	barf-targetspec-parser \
	force-barf-targetspec-parser \
	clean-all-parser-extras \
	clean-all-parsers \
	clean-all-scanner-extras \
	clean-all-scanners \
	doc \
	force-all-parsers \
	force-all-scanners \
	libbarf-all \
	libbarf-commonlang \
	libbarf-core \
	libbarf-preprocessor \
	libbarf-regex \
	libbarf-targetspec \
	reflex-parser \
	force-reflex-parser \
	spotless \
	trison-parser \
	force-trison-parser \
	\
	all-test-scanners \
	test-barf-commonlang-scanner \
	test-barf-preprocessor-scanner \
	diff-all-test-scanners \
	diff-test-barf-commonlang-scanner \
	diff-test-barf-preprocessor-scanner \
	clean-all-test-scanners \
	\
	all-test-parsers \
	test-barf-preprocessor-parser \
	test-barf-regex-parser \
	test-barf-targetspec-parser \
	test-reflex-parser \
	test-trison-parser \
	diff-all-test-parsers \
	diff-test-barf-preprocessor-parser \
	diff-test-barf-regex-parser \
	diff-test-barf-targetspec-parser \
	diff-test-reflex-parser \
	diff-test-trison-parser \
	clean-all-test-parsers

libbarf-all: libbarf-commonlang libbarf-core libbarf-targetspec libbarf-preprocessor libbarf-regex
libbarf-commonlang: lib/libbarf-commonlang.la
libbarf-core: lib/libbarf-core.la
libbarf-targetspec: lib/libbarf-targetspec.la
libbarf-preprocessor: lib/libbarf-preprocessor.la
libbarf-regex: lib/libbarf-regex.la

doc:
	$(DOXYGEN) Doxyfile

spotless: clean clean-all-parser-extras clean-all-scanner-extras

clean-all-parser-extras:
	rm -f $(top_srcdir)/app/reflex/reflex_parser.dpda.dot
	rm -f $(top_srcdir)/app/reflex/reflex_parser.dpda.png
	rm -f $(top_srcdir)/app/reflex/reflex_parser.dpda.states
	rm -f $(top_srcdir)/app/reflex/reflex_parser.npda.dot
	rm -f $(top_srcdir)/app/reflex/reflex_parser.npda.png
	rm -f $(top_srcdir)/app/trison/trison_parser.dpda.dot
	rm -f $(top_srcdir)/app/trison/trison_parser.dpda.png
	rm -f $(top_srcdir)/app/trison/trison_parser.dpda.states
	rm -f $(top_srcdir)/app/trison/trison_parser.npda.dot
	rm -f $(top_srcdir)/app/trison/trison_parser.npda.png
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.dpda.dot
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.dpda.png
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.dpda.states
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.npda.dot
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.npda.png
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.dpda.dot
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.dpda.png
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.dpda.states
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.npda.dot
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.npda.png
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.dpda.dot
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.dpda.png
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.dpda.states
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.npda.dot
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.npda.png

clean-all-parsers: clean-all-parser-extras
	rm -f $(top_srcdir)/app/reflex/reflex_parser.cpp
	rm -f $(top_srcdir)/app/reflex/reflex_parser.hpp
	rm -f $(top_srcdir)/app/trison/trison_parser.cpp
	rm -f $(top_srcdir)/app/trison/trison_parser.hpp
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.cpp
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.hpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.cpp
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.hpp

clean-all-scanner-extras:
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.dot
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.png
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.dot
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.png
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.dot
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.png
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.dot
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.png

clean-all-scanners: clean-all-scanner-extras
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.cpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.hpp

force-all-parsers: \
	force-barf-targetspec-parser \
	force-barf-preprocessor-parser \
	force-barf-regex-parser \
	force-reflex-parser \
	force-trison-parser

force-all-scanners: \
	force-barf-commonlang-scanner \
	force-barf-preprocessor-scanner

all-test-scanners: \
	test-barf-commonlang-scanner \
	test-barf-preprocessor-scanner

diff-all-test-scanners: \
	diff-test-barf-commonlang-scanner \
	diff-test-barf-preprocessor-scanner

clean-all-test-scanners:
	rm -f $(top_builddir)/test/barf_commonlang_scanner.*
	rm -f $(top_builddir)/test/barf_preprocessor_scanner.*

all-test-parsers: \
	test-barf-preprocessor-parser \
	test-barf-regex-parser \
	test-barf-targetspec-parser \
	test-reflex-parser \
	test-trison-parser

diff-all-test-parsers: \
	diff-test-barf-preprocessor-parser \
	diff-test-barf-regex-parser \
	diff-test-barf-targetspec-parser \
	diff-test-reflex-parser \
	diff-test-trison-parser

clean-all-test-parsers:
	rm -f $(top_builddir)/test/barf_preprocessor_parser.*
	rm -f $(top_builddir)/test/barf_regex_parser.*
	rm -f $(top_builddir)/test/barf_targetspec_parser.*
	rm -f $(top_builddir)/test/reflex_parser.*
	rm -f $(top_builddir)/test/trison_parser.*

noinst_LTLIBRARIES = \
	lib/libbarf-commonlang.la \
	lib/libbarf-core.la \
	lib/libbarf-targetspec.la \
	lib/libbarf-preprocessor.la \
    lib/libbarf-regex.la

# installed into $(bindir)
bin_PROGRAMS = \
	bpp \
	playground \
	reflex \
	trison

# installed into $(pkgdatadir) as executable
nobase_pkgdata_SCRIPTS = \
	app/example/calculator/bootstrap.sh

# installed into $(pkgdatadir)
nobase_pkgdata_DATA = \
	app/example/calculator/calculator.hpp \
	app/example/calculator/calculator_main.cpp \
	app/example/calculator/calculator_parser.cpp \
	app/example/calculator/calculator_parser.hpp \
	app/example/calculator/calculator_parser.trison \
	app/example/calculator/calculator_scanner.cpp \
	app/example/calculator/calculator_scanner.hpp \
	app/example/calculator/calculator_scanner.reflex \
	app/example/calculator/configure.ac \
	app/example/calculator/Makefile.am \
	LICENSE \
	README \
	targets/barf.cpp.include.codespec \
	targets/reflex.cpp.header.codespec \
	targets/reflex.cpp.implementation.codespec \
	targets/reflex.cpp.targetspec \
	targets/trison.cpp.header.codespec \
	targets/trison.cpp.implementation.codespec \
	targets/trison.cpp.targetspec \
	TODO

# non-source files which are added to the distribution
EXTRA_DIST = \
    bootstrap.sh \
	configure.ac \
	Doxyfile.in \
	INSTALL \
    LICENSE \
    README \
    TODO \
    app/example/calculator/bootstrap.sh \
    app/example/calculator/calculator.hpp \
    app/example/calculator/calculator_main.cpp \
    app/example/calculator/calculator_parser.cpp \
    app/example/calculator/calculator_parser.hpp \
    app/example/calculator/calculator_parser.trison \
    app/example/calculator/calculator_scanner.cpp \
    app/example/calculator/calculator_scanner.hpp \
    app/example/calculator/calculator_scanner.reflex \
    app/example/calculator/configure.ac \
    app/example/calculator/Makefile.am \
    doc/doxygen_pages/acknowledgements.dox \
    doc/doxygen_pages/footer.html \
    doc/doxygen_pages/generic_regexes.dox \
    doc/doxygen_pages/language_theory.dox \
    doc/doxygen_pages/main.dox \
    doc/doxygen_pages/references.dox \
    doc/doxygen_pages/reflex.dox \
    doc/doxygen_pages/reflex_guts.dox \
    doc/doxygen_pages/regexes_in_barf.dox \
    doc/doxygen_pages/targetspec_and_codespec.dox \
    doc/doxygen_pages/trison_guts.dox \
    extra/barf.xml \
    targets/barf.cpp.include.codespec \
    targets/reflex.cpp.header.codespec \
    targets/reflex.cpp.implementation.codespec \
    targets/reflex.cpp.targetspec \
    targets/trison.cpp.header.codespec \
    targets/trison.cpp.implementation.codespec \
    targets/trison.cpp.targetspec

TARGETS_PATH = $(top_srcdir)/targets

##############################################################################
# general flags
##############################################################################

AM_CPPFLAGS =
LDADD = $(LIBOBJS)

##############################################################################
# BARF libraries
##############################################################################
# commonlang depends on
#	core
#	targetspec
#	preprocessor
# core depends on
#	nothing
# targetspec depends on
#   commonlang
# 	core
# preprocessor depends on
#	core
# regex depends on
#	core
##############################################################################

lib_libbarf_commonlang_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/targetspec \
    -I$(top_srcdir)/lib/preprocessor
lib_libbarf_commonlang_la_SOURCES = \
    lib/commonlang/barf_commonlang_ast.cpp \
    lib/commonlang/barf_commonlang_scanner.cpp \
    lib/commonlang/barf_commonlang_scanner.reflex

lib_libbarf_core_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/core
lib_libbarf_core_la_SOURCES = \
    lib/core/barf.cpp \
    lib/core/barf_ast.cpp \
    lib/core/barf_commandlineparser.cpp \
    lib/core/barf_compiletimeasserts.cpp \
    lib/core/barf_enums.cpp \
    lib/core/barf_filoc.cpp \
    lib/core/barf_graph.cpp \
    lib/core/barf_inputbase.cpp \
    lib/core/barf_message.cpp \
    lib/core/barf_optionsbase.cpp \
    lib/core/barf_searchpath.cpp \
    lib/core/barf_util.cpp

lib_libbarf_preprocessor_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/preprocessor
lib_libbarf_preprocessor_la_SOURCES = \
    lib/preprocessor/barf_preprocessor_ast.cpp \
    lib/preprocessor/barf_preprocessor_ast_execute.cpp \
    lib/preprocessor/barf_preprocessor_ast_print.cpp \
    lib/preprocessor/barf_preprocessor_parser.cpp \
    lib/preprocessor/barf_preprocessor_parser.trison \
    lib/preprocessor/barf_preprocessor_scanner.cpp \
    lib/preprocessor/barf_preprocessor_scanner.reflex \
    lib/preprocessor/barf_preprocessor_symboltable.cpp \
    lib/preprocessor/barf_preprocessor_textifier.cpp

lib_libbarf_regex_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/regex
lib_libbarf_regex_la_SOURCES = \
    lib/regex/barf_regex.cpp \
    lib/regex/barf_regex_ast.cpp \
    lib/regex/barf_regex_dfa.cpp \
    lib/regex/barf_regex_graph.cpp \
    lib/regex/barf_regex_nfa.cpp \
    lib/regex/barf_regex_parser.cpp \
    lib/regex/barf_regex_parser.trison

lib_libbarf_targetspec_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/targetspec
lib_libbarf_targetspec_la_SOURCES = \
    lib/targetspec/barf_targetspec_ast.cpp \
    lib/targetspec/barf_targetspec_parser.cpp \
    lib/targetspec/barf_targetspec_parser.trison

if STABLE_REFLEX_EXISTS
barf-commonlang-scanner: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp
$(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp
$(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex
	$(STABLE_REFLEX) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/commonlang/ --generate-nfa-dot-graph barf_commonlang_scanner.nfa.dot --generate-dfa-dot-graph barf_commonlang_scanner.dfa.dot

force-barf-commonlang-scanner:
	$(STABLE_REFLEX) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/commonlang/ --generate-nfa-dot-graph barf_commonlang_scanner.nfa.dot --generate-dfa-dot-graph barf_commonlang_scanner.dfa.dot

barf-preprocessor-scanner: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.hpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.hpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.cpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.cpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.reflex
	$(STABLE_REFLEX) $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/preprocessor/ --generate-nfa-dot-graph barf_preprocessor_scanner.nfa.dot --generate-dfa-dot-graph barf_preprocessor_scanner.dfa.dot

force-barf-preprocessor-scanner:
	$(STABLE_REFLEX) $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/preprocessor/ --generate-nfa-dot-graph barf_preprocessor_scanner.nfa.dot --generate-dfa-dot-graph barf_preprocessor_scanner.dfa.dot

test-barf-commonlang-scanner:
	mkdir -p test
	./reflex $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_builddir)/test/ --generate-nfa-dot-graph barf_commonlang_scanner.nfa.dot --generate-dfa-dot-graph barf_commonlang_scanner.dfa.dot

test-barf-preprocessor-scanner:
	mkdir -p test
	./reflex $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_builddir)/test/ --generate-nfa-dot-graph barf_preprocessor_scanner.nfa.dot --generate-dfa-dot-graph barf_preprocessor_scanner.dfa.dot
else # !STABLE_REFLEX_EXISTS
barf-commonlang-scanner force-barf-commonlang-scanner:
	@echo "!!!"
	@echo "!!! Can't build barf commonlang scanner without stable reflex binary."
	@echo "!!! Use --with-stable-reflex configure option."
	@echo "!!!"

barf-preprocessor-scanner force-barf-preprocessor-scanner:
	@echo "!!!"
	@echo "!!! Can't build barf preprocessor scanner without stable reflex binary."
	@echo "!!! Use --with-stable-reflex configure option."
	@echo "!!!"

test-barf-commonlang-scanner:
	@echo "!!!"
	@echo "!!! Can't make test-barf-commonlang-scanner without stable reflex binary."
	@echo "!!! Use --with-stable-reflex configure option."
	@echo "!!!"

test-barf-preprocessor-scanner:
	@echo "!!!"
	@echo "!!! Can't make test-barf-preprocessor-scanner without stable reflex binary."
	@echo "!!! Use --with-stable-reflex configure option."
	@echo "!!!"
endif

if DOT_EXISTS
$(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.png: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.dot
	$(DOT) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.dot -Tpng -o $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.png

$(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.png: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.dot
	$(DOT) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.dot -Tpng -o $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.png

$(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.png: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.dot
	$(DOT) $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.dot -Tpng -o $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.png

$(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.png: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.dot
	$(DOT) $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.dot -Tpng -o $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.png
endif

diff-test-barf-commonlang-scanner:
	diff -q $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp $(top_builddir)/test/barf_commonlang_scanner.hpp
	diff -q $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp $(top_builddir)/test/barf_commonlang_scanner.cpp

diff-test-barf-preprocessor-scanner:
	diff -q $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.hpp $(top_builddir)/test/barf_preprocessor_scanner.hpp
	diff -q $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.cpp $(top_builddir)/test/barf_preprocessor_scanner.cpp

if STABLE_TRISON_EXISTS
barf-preprocessor-parser: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison
	$(STABLE_TRISON) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/preprocessor --generate-npda-dot-graph barf_preprocessor_parser.npda.dot --generate-dpda-dot-graph barf_preprocessor_parser.dpda.dot --generate-dpda-states-file barf_preprocessor_parser.dpda.states

force-barf-preprocessor-parser:
	$(STABLE_TRISON) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/preprocessor --generate-npda-dot-graph barf_preprocessor_parser.npda.dot --generate-dpda-dot-graph barf_preprocessor_parser.dpda.dot --generate-dpda-states-file barf_preprocessor_parser.dpda.states

barf-regex-parser: $(top_srcdir)/lib/regex/barf_regex_parser.hpp
$(top_srcdir)/lib/regex/barf_regex_parser.hpp: $(top_srcdir)/lib/regex/barf_regex_parser.cpp
$(top_srcdir)/lib/regex/barf_regex_parser.cpp: $(top_srcdir)/lib/regex/barf_regex_parser.trison
	$(STABLE_TRISON) $(top_srcdir)/lib/regex/barf_regex_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/regex --generate-npda-dot-graph barf_regex_parser.npda.dot --generate-dpda-dot-graph barf_regex_parser.dpda.dot --generate-dpda-states-file barf_regex_parser.dpda.states

force-barf-regex-parser:
	$(STABLE_TRISON) $(top_srcdir)/lib/regex/barf_regex_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/regex --generate-npda-dot-graph barf_regex_parser.npda.dot --generate-dpda-dot-graph barf_regex_parser.dpda.dot --generate-dpda-states-file barf_regex_parser.dpda.states

barf-targetspec-parser: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.hpp
$(top_srcdir)/lib/targetspec/barf_targetspec_parser.hpp: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.cpp
$(top_srcdir)/lib/targetspec/barf_targetspec_parser.cpp: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.trison
	$(STABLE_TRISON) $(top_srcdir)/lib/targetspec/barf_targetspec_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/targetspec --generate-npda-dot-graph barf_targetspec_parser.npda.dot --generate-dpda-dot-graph barf_targetspec_parser.dpda.dot --generate-dpda-states-file barf_targetspec_parser.dpda.states

force-barf-targetspec-parser:
	$(STABLE_TRISON) $(top_srcdir)/lib/targetspec/barf_targetspec_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/targetspec --generate-npda-dot-graph barf_targetspec_parser.npda.dot --generate-dpda-dot-graph barf_targetspec_parser.dpda.dot --generate-dpda-states-file barf_targetspec_parser.dpda.states

test-barf-preprocessor-parser:
	mkdir -p test
	./trison $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_builddir)/test/ --generate-npda-dot-graph barf_preprocessor_parser.npda.dot --generate-dpda-dot-graph barf_preprocessor_parser.dpda.dot --generate-dpda-states-file barf_preprocessor_parser.dpda.states

test-barf-regex-parser:
	mkdir -p test
	./trison $(top_srcdir)/lib/regex/barf_regex_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_builddir)/test/ --generate-npda-dot-graph barf_regex_parser.npda.dot --generate-dpda-dot-graph barf_regex_parser.dpda.dot --generate-dpda-states-file barf_regex_parser.dpda.states

test-barf-targetspec-parser:
	mkdir -p test
	./trison $(top_srcdir)/lib/targetspec/barf_targetspec_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_builddir)/test/ --generate-npda-dot-graph barf_targetspec_parser.npda.dot --generate-dpda-dot-graph barf_targetspec_parser.dpda.dot --generate-dpda-states-file barf_targetspec_parser.dpda.states
else # !STABLE_TRISON_EXISTS
barf-preprocessor-parser force-barf-preprocessor-parser:
	@echo "!!!"
	@echo "!!! Can't build barf preprocessor parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

barf-regex-parser force-barf-regex-parser:
	@echo "!!!"
	@echo "!!! Can't build barf regex parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

barf-targetspec-parser force-barf-targetspec-parser:
	@echo "!!!"
	@echo "!!! Can't build barf targetspec parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

test-barf-preprocessor-parser:
	@echo "!!!"
	@echo "!!! Can't make test-barf-preprocessor-parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

test-barf-regex-parser:
	@echo "!!!"
	@echo "!!! Can't make test-barf-regex-parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

test-barf-targetspec-parser:
	@echo "!!!"
	@echo "!!! Can't make test-barf-targetspec-parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"
endif

if DOT_EXISTS
$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.dpda.png: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.dpda.dot
	$(DOT) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.dpda.dot -Tpng -o $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.dpda.png

$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.npda.png: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.npda.dot
	$(DOT) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.npda.dot -Tpng -o $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.npda.png

$(top_srcdir)/lib/regex/barf_regex_parser.dpda.png: $(top_srcdir)/lib/regex/barf_regex_parser.dpda.dot
	$(DOT) $(top_srcdir)/lib/regex/barf_regex_parser.dpda.dot -Tpng -o $(top_srcdir)/lib/regex/barf_regex_parser.dpda.png

$(top_srcdir)/lib/regex/barf_regex_parser.npda.png: $(top_srcdir)/lib/regex/barf_regex_parser.npda.dot
	$(DOT) $(top_srcdir)/lib/regex/barf_regex_parser.npda.dot -Tpng -o $(top_srcdir)/lib/regex/barf_regex_parser.npda.png

$(top_srcdir)/lib/targetspec/barf_targetspec_parser.dpda.png: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.dpda.dot
	$(DOT) $(top_srcdir)/lib/targetspec/barf_targetspec_parser.dpda.dot -Tpng -o $(top_srcdir)/lib/targetspec/barf_targetspec_parser.dpda.png

$(top_srcdir)/lib/targetspec/barf_targetspec_parser.npda.png: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.npda.dot
	$(DOT) $(top_srcdir)/lib/targetspec/barf_targetspec_parser.npda.dot -Tpng -o $(top_srcdir)/lib/targetspec/barf_targetspec_parser.npda.png
endif

diff-test-barf-preprocessor-parser:
	diff -q $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp $(top_builddir)/test/barf_preprocessor_parser.hpp
	diff -q $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp $(top_builddir)/test/barf_preprocessor_parser.cpp

diff-test-barf-regex-parser:
	diff -q $(top_srcdir)/lib/regex/barf_regex_parser.hpp $(top_builddir)/test/barf_regex_parser.hpp
	diff -q $(top_srcdir)/lib/regex/barf_regex_parser.cpp $(top_builddir)/test/barf_regex_parser.cpp

diff-test-barf-targetspec-parser:
	diff -q $(top_srcdir)/lib/targetspec/barf_targetspec_parser.hpp $(top_builddir)/test/barf_targetspec_parser.hpp
	diff -q $(top_srcdir)/lib/targetspec/barf_targetspec_parser.cpp $(top_builddir)/test/barf_targetspec_parser.cpp

##############################################################################
# bpp (BARF preprocessor)
##############################################################################

bpp_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/app/bpp \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/preprocessor

bpp_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-core.la

bpp_SOURCES = \
	app/bpp/bpp_main.cpp \
	app/bpp/bpp_options.cpp

##############################################################################
# playground (testing area)
##############################################################################

playground_CPPFLAGS = \
	$(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/preprocessor \
    -I$(top_srcdir)/lib/regex \
    -I$(top_srcdir)/lib/targetspec

playground_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-regex.la \
    $(top_builddir)/lib/libbarf-commonlang.la \
    $(top_builddir)/lib/libbarf-core.la \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-targetspec.la

playground_SOURCES = \
	app/playground.cpp

##############################################################################
# reflex
##############################################################################

reflex_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/app/reflex \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/targetspec \
    -I$(top_srcdir)/lib/preprocessor \
    -I$(top_srcdir)/lib/regex

reflex_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-targetspec.la \
    $(top_builddir)/lib/libbarf-commonlang.la \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-regex.la \
    $(top_builddir)/lib/libbarf-core.la

reflex_SOURCES = \
	app/reflex/reflex_ast.cpp \
	app/reflex/reflex_automaton.cpp \
	app/reflex/reflex_codespecsymbols.cpp \
	app/reflex/reflex_main.cpp \
	app/reflex/reflex_options.cpp \
	app/reflex/reflex_parser.cpp \
	app/reflex/reflex_parser.trison


if STABLE_TRISON_EXISTS
reflex-parser: $(top_srcdir)/app/reflex/reflex_parser.hpp
$(top_srcdir)/app/reflex/reflex_parser.hpp: $(top_srcdir)/app/reflex/reflex_parser.cpp
$(top_srcdir)/app/reflex/reflex_parser.cpp: $(top_srcdir)/app/reflex/reflex_parser.trison
	$(STABLE_TRISON) $(top_srcdir)/app/reflex/reflex_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/app/reflex --generate-npda-dot-graph reflex_parser.npda.dot --generate-dpda-dot-graph reflex_parser.dpda.dot --generate-dpda-states-file reflex_parser.dpda.states

force-reflex-parser:
	$(STABLE_TRISON) $(top_srcdir)/app/reflex/reflex_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/app/reflex --generate-npda-dot-graph reflex_parser.npda.dot --generate-dpda-dot-graph reflex_parser.dpda.dot --generate-dpda-states-file reflex_parser.dpda.states

test-reflex-parser:
	mkdir -p test
	./trison $(top_srcdir)/app/reflex/reflex_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_builddir)/test/ --generate-npda-dot-graph reflex_parser.npda.dot --generate-dpda-dot-graph reflex_parser.dpda.dot --generate-dpda-states-file reflex_parser.dpda.states
else # !STABLE_TRISON_EXISTS
reflex-parser force-reflex-parser:
	@echo "!!!"
	@echo "!!! Can't build reflex parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

test-reflex-parser:
	@echo "!!!"
	@echo "!!! Can't make test-reflex-parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"
endif

if DOT_EXISTS
$(top_srcdir)/app/reflex/reflex_parser.dpda.png: $(top_srcdir)/app/reflex/reflex_parser.dpda.dot
	$(DOT) $(top_srcdir)/app/reflex/reflex_parser.dpda.dot -Tpng -o $(top_srcdir)/app/reflex/reflex_parser.dpda.png

$(top_srcdir)/app/reflex/reflex_parser.npda.png: $(top_srcdir)/app/reflex/reflex_parser.npda.dot
	$(DOT) $(top_srcdir)/app/reflex/reflex_parser.npda.dot -Tpng -o $(top_srcdir)/app/reflex/reflex_parser.npda.png
endif

diff-test-reflex-parser:
	diff -q $(top_srcdir)/app/reflex/reflex_parser.hpp $(top_builddir)/test/reflex_parser.hpp
	diff -q $(top_srcdir)/app/reflex/reflex_parser.cpp $(top_builddir)/test/reflex_parser.cpp

##############################################################################
# trison
##############################################################################

trison_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/app/trison \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/targetspec \
    -I$(top_srcdir)/lib/preprocessor

trison_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-commonlang.la \
    $(top_builddir)/lib/libbarf-targetspec.la \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-core.la

trison_SOURCES = \
	app/trison/trison_ast.cpp \
	app/trison/trison_codespecsymbols.cpp \
	app/trison/trison_dpda.cpp \
	app/trison/trison_enums.cpp \
	app/trison/trison_graph.cpp \
	app/trison/trison_main.cpp \
	app/trison/trison_message.cpp \
	app/trison/trison_npda.cpp \
	app/trison/trison_options.cpp \
	app/trison/trison_parser.cpp \
	app/trison/trison_parser.trison

if STABLE_TRISON_EXISTS
trison-parser: $(top_srcdir)/app/trison/trison_parser.hpp
$(top_srcdir)/app/trison/trison_parser.hpp: $(top_srcdir)/app/trison/trison_parser.cpp
$(top_srcdir)/app/trison/trison_parser.cpp: $(top_srcdir)/app/trison/trison_parser.trison
	$(STABLE_TRISON) $(top_srcdir)/app/trison/trison_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/app/trison --generate-npda-dot-graph trison_parser.npda.dot --generate-dpda-dot-graph trison_parser.dpda.dot --generate-dpda-states-file trison_parser.dpda.states

force-trison-parser:
	$(STABLE_TRISON) $(top_srcdir)/app/trison/trison_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/app/trison --generate-npda-dot-graph trison_parser.npda.dot --generate-dpda-dot-graph trison_parser.dpda.dot --generate-dpda-states-file trison_parser.dpda.states

test-trison-parser:
	mkdir -p test
	./trison $(top_srcdir)/app/trison/trison_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_builddir)/test/ --generate-npda-dot-graph trison_parser.npda.dot --generate-dpda-dot-graph trison_parser.dpda.dot --generate-dpda-states-file trison_parser.dpda.states
else # !STABLE_TRISON_EXISTS
trison-parser force-trison-parser:
	@echo "!!!"
	@echo "!!! Can't build trison parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

test-trison-parser:
	@echo "!!!"
	@echo "!!! Can't make test-trison-parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"
endif

if DOT_EXISTS
$(top_srcdir)/app/trison/trison_parser.dpda.png: $(top_srcdir)/app/trison/trison_parser.dpda.dot
	$(DOT) $(top_srcdir)/app/trison/trison_parser.dpda.dot -Tpng -o $(top_srcdir)/app/trison/trison_parser.dpda.png

$(top_srcdir)/app/trison/trison_parser.npda.png: $(top_srcdir)/app/trison/trison_parser.npda.dot
	$(DOT) $(top_srcdir)/app/trison/trison_parser.npda.dot -Tpng -o $(top_srcdir)/app/trison/trison_parser.npda.png
endif

diff-test-trison-parser:
	diff -q $(top_srcdir)/app/trison/trison_parser.hpp $(top_builddir)/test/trison_parser.hpp
	diff -q $(top_srcdir)/app/trison/trison_parser.cpp $(top_builddir)/test/trison_parser.cpp

##############################################################################
# all the no-install header files for the applications (is this necessary?)
##############################################################################

noinst_HEADERS = \
	app/bpp/bpp.hpp \
	app/bpp/bpp_options.hpp \
	\
	app/reflex/reflex.hpp \
	app/reflex/reflex_ast.hpp \
	app/reflex/reflex_automaton.hpp \
	app/reflex/reflex_codespecsymbols.hpp \
	app/reflex/reflex_options.hpp \
	app/reflex/reflex_parser.hpp \
	\
	app/trison/trison.hpp \
	app/trison/trison_ast.hpp \
	app/trison/trison_codespecsymbols.hpp \
	app/trison/trison_dpda.hpp \
	app/trison/trison_enums.hpp \
	app/trison/trison_graph.hpp \
	app/trison/trison_message.hpp \
	app/trison/trison_npda.hpp \
	app/trison/trison_options.hpp \
	app/trison/trison_parser.hpp \
	\
	lib/commonlang/barf_commonlang.hpp \
	lib/commonlang/barf_commonlang_ast.hpp \
	lib/commonlang/barf_commonlang_scanner.hpp \
	\
	lib/core/barf.hpp \
	lib/core/barf_ast.hpp \
	lib/core/barf_commandlineparser.hpp \
	lib/core/barf_compiletimeasserts.hpp \
	lib/core/barf_enums.hpp \
	lib/core/barf_filoc.hpp \
	lib/core/barf_graph.hpp \
	lib/core/barf_inputbase.hpp \
	lib/core/barf_list.hpp \
	lib/core/barf_message.hpp \
	lib/core/barf_optionsbase.hpp \
	lib/core/barf_pointer.hpp \
	lib/core/barf_searchpath.hpp \
	lib/core/barf_types.hpp \
	lib/core/barf_util.hpp \
	lib/core/barf_weakreference.hpp \
	\
	lib/preprocessor/barf_preprocessor.hpp \
	lib/preprocessor/barf_preprocessor_ast.hpp \
	lib/preprocessor/barf_preprocessor_parser.hpp \
	lib/preprocessor/barf_preprocessor_scanner.hpp \
	lib/preprocessor/barf_preprocessor_symboltable.hpp \
	lib/preprocessor/barf_preprocessor_textifier.hpp \
	\
	lib/regex/barf_regex.hpp \
	lib/regex/barf_regex_ast.hpp \
	lib/regex/barf_regex_dfa.hpp \
	lib/regex/barf_regex_graph.hpp \
	lib/regex/barf_regex_nfa.hpp \
	lib/regex/barf_regex_parser.hpp \
	\
	lib/targetspec/barf_targetspec.hpp \
	lib/targetspec/barf_targetspec_ast.hpp \
	lib/targetspec/barf_targetspec_parser.hpp
