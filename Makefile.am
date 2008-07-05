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
	clean-all-parsers \
	clean-all-scanners \
	clean-parser-extras \
	clean-scanner-extras \
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
	trison-unstable-parser \
	force-trison-unstable-parser

libbarf-all: libbarf-commonlang libbarf-core libbarf-targetspec libbarf-preprocessor libbarf-regex
libbarf-commonlang: lib/libbarf-commonlang.la
libbarf-core: lib/libbarf-core.la
libbarf-targetspec: lib/libbarf-targetspec.la
libbarf-preprocessor: lib/libbarf-preprocessor.la
libbarf-regex: lib/libbarf-regex.la

doc:
	$(DOXYGEN) Doxyfile

spotless: clean clean-parser-extras clean-scanner-extras

clean-parser-extras:
	rm -f $(top_srcdir)/app/reflex/reflex_parser.states
	rm -f $(top_srcdir)/app/trison/trison_parser.states
	rm -f $(top_srcdir)/app/trison/trison_parser_header.cpp
	rm -f $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	rm -f $(top_srcdir)/app/trison.unstable/trison_parser.states
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.states
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.states
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.states

clean-all-parsers: clean-parser-extras
	rm -f $(top_srcdir)/app/reflex/reflex_parser.cpp
	rm -f $(top_srcdir)/app/reflex/reflex_parser.hpp
	rm -f $(top_srcdir)/app/trison/trison_parser.cpp
	rm -f $(top_srcdir)/app/trison/trison_parser.hpp
	rm -f $(top_srcdir)/app/trison.unstable/trison_parser.cpp
	rm -f $(top_srcdir)/app/trison.unstable/trison_parser.hpp
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.cpp
	rm -f $(top_srcdir)/lib/targetspec/barf_targetspec_parser.hpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.cpp
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.hpp

clean-scanner-extras:
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.dot
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.dfa.png
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.dot
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.nfa.png
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.dot
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.dfa.png
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.dot
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.nfa.png

clean-all-scanners:
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner_new.cpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner_new.hpp

force-all-parsers: \
	force-barf-targetspec-parser \
	force-barf-preprocessor-parser \
	force-barf-regex-parser \
	force-reflex-parser \
	force-trison-parser \
	force-trison-unstable-parser

force-all-scanners: \
	force-barf-commonlang-scanner \
	force-barf-preprocessor-scanner

noinst_LTLIBRARIES = \
	lib/libbarf-commonlang.la \
	lib/libbarf-core.la \
	lib/libbarf-targetspec.la \
	lib/libbarf-preprocessor.la \
    lib/libbarf-regex.la

if BUILD_TRISON_UNSTABLE
bin_PROGRAMS = \
	bpp \
	reflex \
	trison \
	trison.unstable
else # !BUILD_TRISON_UNSTABLE
bin_PROGRAMS = \
	bpp \
	reflex \
	trison
endif

noinst_PROGRAMS = \
	playground

EXTRA_DIST = \
    bootstrap.sh \
	configure.ac \
	Doxyfile.in \
	INSTALL \
    LICENSE \
    README \
    TODO \
    doc/doxygen_pages/acknowledgements.dox \
    doc/doxygen_pages/generic_regexes.dox \
    doc/doxygen_pages/targetspec_and_codespec.dox \
    doc/doxygen_pages/language_theory.dox \
    doc/doxygen_pages/main.dox \
    doc/doxygen_pages/references.dox \
    doc/doxygen_pages/reflex.dox \
    doc/doxygen_pages/reflex_guts.dox \
    doc/doxygen_pages/regexes_in_barf.dox \
    extra/barf.xml \
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

lib_libbarf_targetspec_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/targetspec
lib_libbarf_targetspec_la_SOURCES = \
    lib/targetspec/barf_targetspec_ast.cpp \
    lib/targetspec/barf_targetspec_enums.cpp \
    lib/targetspec/barf_targetspec_parser.cpp \
    lib/targetspec/barf_targetspec_parser.trison

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
    lib/preprocessor/barf_preprocessor_scanner_new.cpp \
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

if STABLE_REFLEX_EXISTS
barf-commonlang-scanner: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp
$(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp
$(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex $(STABLE_REFLEX)
	$(STABLE_REFLEX) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/commonlang/ --generate-nfa-dot-graph barf_commonlang_scanner.nfa.dot --generate-dfa-dot-graph barf_commonlang_scanner.dfa.dot

force-barf-commonlang-scanner:
	$(STABLE_REFLEX) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/commonlang/ --generate-nfa-dot-graph barf_commonlang_scanner.nfa.dot --generate-dfa-dot-graph barf_commonlang_scanner.dfa.dot

barf-preprocessor-scanner: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner_new.hpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner_new.hpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner_new.cpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner_new.cpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.reflex $(STABLE_REFLEX)
	$(STABLE_REFLEX) $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/preprocessor/ --generate-nfa-dot-graph barf_preprocessor_scanner.nfa.dot --generate-dfa-dot-graph barf_preprocessor_scanner.dfa.dot

force-barf-preprocessor-scanner:
	$(STABLE_REFLEX) $(top_srcdir)/lib/preprocessor/barf_preprocessor_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/preprocessor/ --generate-nfa-dot-graph barf_preprocessor_scanner.nfa.dot --generate-dfa-dot-graph barf_preprocessor_scanner.dfa.dot
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

if STABLE_TRISON_EXISTS
barf-targetspec-parser: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.hpp
$(top_srcdir)/lib/targetspec/barf_targetspec_parser.hpp: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.cpp
$(top_srcdir)/lib/targetspec/barf_targetspec_parser.cpp: $(top_srcdir)/lib/targetspec/barf_targetspec_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/targetspec/barf_targetspec_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/targetspec/barf_targetspec_parser --generate-states-file $(top_srcdir)/lib/targetspec/barf_targetspec_parser.states

force-barf-targetspec-parser:
	$(STABLE_TRISON) $(top_srcdir)/lib/targetspec/barf_targetspec_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/targetspec/barf_targetspec_parser --generate-states-file $(top_srcdir)/lib/targetspec/barf_targetspec_parser.states

barf-preprocessor-parser: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser --generate-states-file $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.states

force-barf-preprocessor-parser:
	$(STABLE_TRISON) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser --generate-states-file $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.states

barf-regex-parser: $(top_srcdir)/lib/regex/barf_regex_parser.hpp
$(top_srcdir)/lib/regex/barf_regex_parser.hpp: $(top_srcdir)/lib/regex/barf_regex_parser.cpp
$(top_srcdir)/lib/regex/barf_regex_parser.cpp: $(top_srcdir)/lib/regex/barf_regex_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/regex/barf_regex_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/regex/barf_regex_parser --generate-states-file $(top_srcdir)/lib/regex/barf_regex_parser.states

force-barf-regex-parser:
	$(STABLE_TRISON) $(top_srcdir)/lib/regex/barf_regex_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/regex/barf_regex_parser --generate-states-file $(top_srcdir)/lib/regex/barf_regex_parser.states
else # !STABLE_TRISON_EXISTS
barf-targetspec-parser force-barf-targetspec-parser:
	@echo "!!!"
	@echo "!!! Can't build barf targetspec parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

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
endif

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
# playground (app for testing stuff)
##############################################################################

playground_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/app \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/targetspec \
    -I$(top_srcdir)/lib/preprocessor \
    -I$(top_srcdir)/lib/regex

playground_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-targetspec.la \
    $(top_builddir)/lib/libbarf-commonlang.la \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-regex.la \
    $(top_builddir)/lib/libbarf-core.la

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
	app/reflex/reflex_enums.cpp \
	app/reflex/reflex_main.cpp \
	app/reflex/reflex_options.cpp \
	app/reflex/reflex_parser.cpp \
	app/reflex/reflex_parser.trison

if STABLE_TRISON_EXISTS
reflex-parser: $(top_srcdir)/app/reflex/reflex_parser.hpp
$(top_srcdir)/app/reflex/reflex_parser.hpp: $(top_srcdir)/app/reflex/reflex_parser.cpp
$(top_srcdir)/app/reflex/reflex_parser.cpp: $(top_srcdir)/app/reflex/reflex_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/reflex/reflex_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/reflex/reflex_parser --generate-states-file $(top_srcdir)/app/reflex/reflex_parser.states

force-reflex-parser:
	$(STABLE_TRISON) $(top_srcdir)/app/reflex/reflex_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/reflex/reflex_parser --generate-states-file $(top_srcdir)/app/reflex/reflex_parser.states
else # !STABLE_TRISON_EXISTS
reflex-parser force-reflex-parser:
	@echo "!!!"
	@echo "!!! Can't build reflex parser without stable reflex binary."
	@echo "!!! Use --with-stable-reflex configure option."
	@echo "!!!"
endif

##############################################################################
# trison
##############################################################################

trison_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/app/trison \
    -I$(top_srcdir)/lib/core

trison_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-core.la

trison_SOURCES = \
	app/trison/trison_ast.cpp \
	app/trison/trison_enums.cpp \
	app/trison/trison_main.cpp \
	app/trison/trison_message.cpp \
	app/trison/trison_options.cpp \
	app/trison/trison_parser.cpp \
	app/trison/trison_parser.trison \
	app/trison/trison_parser_header.cpp \
	app/trison/trison_parser_header.cpp.template \
	app/trison/trison_parser_implementation.cpp \
	app/trison/trison_parser_implementation.cpp.template \
	app/trison/trison_preprocessor.cpp \
	app/trison/trison_scanner.cpp \
	app/trison/trison_state.cpp \
	app/trison/trison_stateid.cpp \
	app/trison/trison_statemachine.cpp \
	app/trison/trison_transition.cpp

if STABLE_TRISON_EXISTS
trison-parser: $(top_srcdir)/app/trison/trison_parser.hpp
$(top_srcdir)/app/trison/trison_parser.hpp: $(top_srcdir)/app/trison/trison_parser.cpp
$(top_srcdir)/app/trison/trison_parser.cpp: $(top_srcdir)/app/trison/trison_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/trison/trison_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/trison/trison_parser --generate-states-file $(top_srcdir)/app/trison/trison_parser.states

force-trison-parser:
	$(STABLE_TRISON) $(top_srcdir)/app/trison/trison_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/trison/trison_parser --generate-states-file $(top_srcdir)/app/trison/trison_parser.states
else # !STABLE_TRISON_EXISTS
trison-parser force-trison-parser:
	@echo "!!!"
	@echo "!!! Can't build trison parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"
endif

# custom rules for the "template" files (this is going away soon)
$(top_srcdir)/app/trison/trison_parser_header.cpp: $(top_srcdir)/app/trison/trison_parser_header.cpp.template
	echo "#include \"trison_statemachine.hpp\"" > $(top_srcdir)/app/trison/trison_parser_header.cpp
	echo "" >> $(top_srcdir)/app/trison/trison_parser_header.cpp
	echo "string const Trison::StateMachine::ms_header_file_template =" >> $(top_srcdir)/app/trison/trison_parser_header.cpp
	echo "\"\\" >> $(top_srcdir)/app/trison/trison_parser_header.cpp
	cat $(top_srcdir)/app/trison/trison_parser_header.cpp.template | sed "s#\\\\#\\\\\\\\#g" | sed "s#\\\"#\\\\\"#g" | sed "s#.*#&\\\\n\\\\#g" >> $(top_srcdir)/app/trison/trison_parser_header.cpp
	echo "\";" >> $(top_srcdir)/app/trison/trison_parser_header.cpp
	echo "" >> $(top_srcdir)/app/trison/trison_parser_header.cpp

$(top_srcdir)/app/trison/trison_parser_implementation.cpp: $(top_srcdir)/app/trison/trison_parser_implementation.cpp.template
	echo "#include \"trison_statemachine.hpp\"" > $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	echo "" >> $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	echo "string const Trison::StateMachine::ms_implementation_file_template =" >> $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	echo "\"\\" >> $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	cat $(top_srcdir)/app/trison/trison_parser_implementation.cpp.template | sed "s#\\\\#\\\\\\\\#g" | sed "s#\\\"#\\\\\"#g" | sed "s#.*#&\\\\n\\\\#g" >> $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	echo "\";" >> $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	echo "" >> $(top_srcdir)/app/trison/trison_parser_implementation.cpp

##############################################################################
# trison.unstable
##############################################################################

if BUILD_TRISON_UNSTABLE

trison_unstable_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/app/trison.unstable \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/targetspec \
    -I$(top_srcdir)/lib/preprocessor

trison_unstable_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-commonlang.la \
    $(top_builddir)/lib/libbarf-targetspec.la \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-core.la

trison_unstable_SOURCES = \
	app/trison.unstable/trison_ast.cpp \
	app/trison.unstable/trison_codespecsymbols.cpp \
	app/trison.unstable/trison_dpda.cpp \
	app/trison.unstable/trison_enums.cpp \
	app/trison.unstable/trison_graph.cpp \
	app/trison.unstable/trison_main.cpp \
	app/trison.unstable/trison_message.cpp \
	app/trison.unstable/trison_npda.cpp \
	app/trison.unstable/trison_options.cpp \
	app/trison.unstable/trison_parser.cpp \
	app/trison.unstable/trison_parser.trison

if STABLE_TRISON_EXISTS
trison-unstable-parser: $(top_srcdir)/app/trison.unstable/trison_parser.hpp
$(top_srcdir)/app/trison.unstable/trison_parser.hpp: $(top_srcdir)/app/trison.unstable/trison_parser.cpp
$(top_srcdir)/app/trison.unstable/trison_parser.cpp: $(top_srcdir)/app/trison.unstable/trison_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/trison.unstable/trison_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/trison.unstable/trison_parser --generate-states-file $(top_srcdir)/app/trison.unstable/trison_parser.states

force-trison-unstable-parser:
	$(STABLE_TRISON) $(top_srcdir)/app/trison.unstable/trison_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/trison.unstable/trison_parser --generate-states-file $(top_srcdir)/app/trison.unstable/trison_parser.states
else # !STABLE_TRISON_EXISTS
trison-unstable-parser force-trison-unstable-parser:
	@echo "!!!"
	@echo "!!! Can't build trison.unstable parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"
endif

else # !BUILD_TRISON_UNSTABLE

trison-unstable-parser force-trison-unstable-parser:
	@echo "!!!"
	@echo "!!! These targets are disabled without configuring using --enable-trison-unstable."
	@echo "!!!"

endif

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
	app/reflex/reflex_enums.hpp \
	app/reflex/reflex_options.hpp \
	app/reflex/reflex_parser.hpp \
	\
	app/trison/trison.hpp \
	app/trison/trison_ast.hpp \
	app/trison/trison_enums.hpp \
	app/trison/trison_message.hpp \
	app/trison/trison_options.hpp \
	app/trison/trison_parser.hpp \
	app/trison/trison_preprocessor.hpp \
	app/trison/trison_rulephase.hpp \
	app/trison/trison_scanner.hpp \
	app/trison/trison_state.hpp \
	app/trison/trison_stateid.hpp \
	app/trison/trison_statemachine.hpp \
	app/trison/trison_terminal.hpp \
	app/trison/trison_transition.hpp \
	\
	app/trison.unstable/trison.hpp \
	app/trison.unstable/trison_ast.hpp \
	app/trison.unstable/trison_codespecsymbols.hpp \
	app/trison.unstable/trison_dpda.hpp \
	app/trison.unstable/trison_enums.hpp \
	app/trison.unstable/trison_graph.hpp \
	app/trison.unstable/trison_message.hpp \
	app/trison.unstable/trison_npda.hpp \
	app/trison.unstable/trison_options.hpp \
	app/trison.unstable/trison_parser.hpp \
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
	lib/targetspec/barf_targetspec.hpp \
	lib/targetspec/barf_targetspec_ast.hpp \
	lib/targetspec/barf_targetspec_enums.hpp \
	lib/targetspec/barf_targetspec_parser.hpp \
	\
	lib/preprocessor/barf_preprocessor.hpp \
	lib/preprocessor/barf_preprocessor_ast.hpp \
	lib/preprocessor/barf_preprocessor_parser.hpp \
	lib/preprocessor/barf_preprocessor_scanner.hpp \
	lib/preprocessor/barf_preprocessor_scanner_new.hpp \
	lib/preprocessor/barf_preprocessor_symboltable.hpp \
	lib/preprocessor/barf_preprocessor_textifier.hpp \
	\
	lib/regex/barf_regex.hpp \
	lib/regex/barf_regex_ast.hpp \
	lib/regex/barf_regex_dfa.hpp \
	lib/regex/barf_regex_graph.hpp \
	lib/regex/barf_regex_nfa.hpp \
	lib/regex/barf_regex_parser.hpp
