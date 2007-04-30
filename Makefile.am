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
	libbarf-all \
	libbarf-commonlang \
	libbarf-core \
	libbarf-langspec \
	libbarf-preprocessor \
	libbarf-regex \
	doc \
	spotless \
	clean-parser-extras \
	clean-all-parsers \
	clean-all-scanners \
	force-all-parsers \
	force-all-scanners \
	barf-commonlang-scanner \
	barf-commonlang-scanner-force \
	barf-commonlang-scanner-release \
	barf-langspec-parser \
	barf-langspec-parser-force \
	barf-langspec-parser-release \
	barf-preprocessor-parser \
	barf-preprocessor-parser-force \
	barf-preprocessor-parser-release \
	barf-regex-parser \
	barf-regex-parser-force \
	barf-regex-parser-release \
	reflex-parser \
	reflex-parser-force \
	reflex-parser-release \
	trison-parser \
	trison-parser-force \
	trison-parser-release \
	trison-unstable-parser \
	trison-unstable-parser-force \
	trison-unstable-parser-release

libbarf-all: libbarf-commonlang libbarf-core libbarf-langspec libbarf-preprocessor libbarf-regex
libbarf-commonlang: lib/libbarf-commonlang.la
libbarf-core: lib/libbarf-core.la
libbarf-langspec: lib/libbarf-langspec.la
libbarf-preprocessor: lib/libbarf-preprocessor.la
libbarf-regex: lib/libbarf-regex.la

doc:
	$(DOXYGEN) Doxyfile

spotless: clean clean-parser-extras

clean-parser-extras:
	rm -f $(top_srcdir)/app/reflex/reflex_parser.states
	rm -f $(top_srcdir)/app/trison/trison_parser.states
	rm -f $(top_srcdir)/app/trison/trison_parser_header.cpp
	rm -f $(top_srcdir)/app/trison/trison_parser_implementation.cpp
	rm -f $(top_srcdir)/app/trison.unstable/trison_parser.states
	rm -f $(top_srcdir)/lib/langspec/barf_langspec_parser.states
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.states
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.states

clean-all-parsers: clean-parser-extras
	rm -f $(top_srcdir)/app/reflex/reflex_parser.cpp
	rm -f $(top_srcdir)/app/reflex/reflex_parser.hpp
	rm -f $(top_srcdir)/app/trison/trison_parser.cpp
	rm -f $(top_srcdir)/app/trison/trison_parser.hpp
	rm -f $(top_srcdir)/app/trison.unstable/trison_parser.cpp
	rm -f $(top_srcdir)/app/trison.unstable/trison_parser.hpp
	rm -f $(top_srcdir)/lib/langspec/barf_langspec_parser.cpp
	rm -f $(top_srcdir)/lib/langspec/barf_langspec_parser.hpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp
	rm -f $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.cpp
	rm -f $(top_srcdir)/lib/regex/barf_regex_parser.hpp

clean-all-scanners:
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.cpp
	rm -f $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.hpp

force-all-parsers: \
	barf-langspec-parser-force \
	barf-preprocessor-parser-force \
	barf-regex-parser-force \
	reflex-parser-force \
	trison-parser-force \
	trison-unstable-parser-force

force-all-scanners: \
	barf-commonlang-scanner-force

noinst_LTLIBRARIES = \
	lib/libbarf-commonlang.la \
	lib/libbarf-core.la \
	lib/libbarf-langspec.la \
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
    data/reflex.cpp.header.codespec \
    data/reflex.cpp.implementation.codespec \
    data/reflex.cpp.langspec \
    data/trison.cpp.header.codespec \
    data/trison.cpp.implementation.codespec \
    data/trison.cpp.langspec \
    doc/doxygen_pages/acknowledgements.dox \
    doc/doxygen_pages/generic_regexes.dox \
    doc/doxygen_pages/langspec_and_codespec.dox \
    doc/doxygen_pages/language_theory.dox \
    doc/doxygen_pages/main.dox \
    doc/doxygen_pages/references.dox \
    doc/doxygen_pages/reflex.dox \
    doc/doxygen_pages/reflex_guts.dox \
    doc/doxygen_pages/regexes_in_barf.dox \
    extra/barf.xml

DATA_PATH = $(top_srcdir)/data

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
#	langspec
#	preprocessor
# core depends on
#	nothing
# langspec depends on
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
    -I$(top_srcdir)/lib/langspec \
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

lib_libbarf_langspec_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/lib/commonlang \
    -I$(top_srcdir)/lib/core \
    -I$(top_srcdir)/lib/langspec
lib_libbarf_langspec_la_SOURCES = \
    lib/langspec/barf_langspec_ast.cpp \
    lib/langspec/barf_langspec_enums.cpp \
    lib/langspec/barf_langspec_parser.cpp \
    lib/langspec/barf_langspec_parser.trison

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
	$(STABLE_REFLEX) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-search-path $(DATA_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/commonlang/

barf-commonlang-scanner-force: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex $(STABLE_REFLEX)
	$(STABLE_REFLEX) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-search-path $(DATA_PATH) --warnings-as-errors --output-directory $(top_srcdir)/lib/commonlang/

barf-commonlang-scanner-release: $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex $(STABLE_REFLEX)
	$(STABLE_REFLEX) $(top_srcdir)/lib/commonlang/barf_commonlang_scanner.reflex --include-search-path $(DATA_PATH) --warnings-as-errors --without-line-directives --output-directory $(top_srcdir)/lib/commonlang/
else # !STABLE_REFLEX_EXISTS
barf-commonlang-scanner barf-commonlang-scanner-force barf-commonlang-scanner-release:
	@echo "!!!"
	@echo "!!! Can't build barf commonlang scanner without stable trison binary."
	@echo "!!! Use --with-stable-reflex configure option."
	@echo "!!!"
endif

if STABLE_TRISON_EXISTS
barf-langspec-parser: $(top_srcdir)/lib/langspec/barf_langspec_parser.hpp
$(top_srcdir)/lib/langspec/barf_langspec_parser.hpp: $(top_srcdir)/lib/langspec/barf_langspec_parser.cpp
$(top_srcdir)/lib/langspec/barf_langspec_parser.cpp: $(top_srcdir)/lib/langspec/barf_langspec_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/langspec/barf_langspec_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/langspec/barf_langspec_parser --generate-states-file $(top_srcdir)/lib/langspec/barf_langspec_parser.states

barf-langspec-parser-force: $(top_srcdir)/lib/langspec/barf_langspec_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/langspec/barf_langspec_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/langspec/barf_langspec_parser --generate-states-file $(top_srcdir)/lib/langspec/barf_langspec_parser.states

barf-langspec-parser-release: $(top_srcdir)/lib/langspec/barf_langspec_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/langspec/barf_langspec_parser.trison --warnings-as-errors --without-line-directives --output-basename $(top_srcdir)/lib/langspec/barf_langspec_parser --generate-states-file $(top_srcdir)/lib/langspec/barf_langspec_parser.states

barf-preprocessor-parser: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.hpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp
$(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.cpp: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser --generate-states-file $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.states

barf-preprocessor-parser-force: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser --generate-states-file $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.states

barf-preprocessor-parser-release: $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.trison --warnings-as-errors --without-line-directives --output-basename $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser --generate-states-file $(top_srcdir)/lib/preprocessor/barf_preprocessor_parser.states

barf-regex-parser: $(top_srcdir)/lib/regex/barf_regex_parser.hpp
$(top_srcdir)/lib/regex/barf_regex_parser.hpp: $(top_srcdir)/lib/regex/barf_regex_parser.cpp
$(top_srcdir)/lib/regex/barf_regex_parser.cpp: $(top_srcdir)/lib/regex/barf_regex_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/regex/barf_regex_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/regex/barf_regex_parser --generate-states-file $(top_srcdir)/lib/regex/barf_regex_parser.states

barf-regex-parser-force: $(top_srcdir)/lib/regex/barf_regex_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/regex/barf_regex_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/lib/regex/barf_regex_parser --generate-states-file $(top_srcdir)/lib/regex/barf_regex_parser.states

barf-regex-parser-release: $(top_srcdir)/lib/regex/barf_regex_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/lib/regex/barf_regex_parser.trison --warnings-as-errors --without-line-directives --output-basename $(top_srcdir)/lib/regex/barf_regex_parser --generate-states-file $(top_srcdir)/lib/regex/barf_regex_parser.states
else # !STABLE_TRISON_EXISTS
barf-langspec-parser barf-langspec-parser-force barf-langspec-parser-release:
	@echo "!!!"
	@echo "!!! Can't build barf langspec parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

barf-preprocessor-parser barf-preprocessor-parser-force barf-preprocessor-parser-release:
	@echo "!!!"
	@echo "!!! Can't build barf preprocessor parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"

barf-regex-parser barf-regex-parser-force barf-regex-parser-release:
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
    -I$(top_srcdir)/lib/langspec \
    -I$(top_srcdir)/lib/preprocessor \
    -I$(top_srcdir)/lib/regex

playground_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-langspec.la \
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
    -I$(top_srcdir)/lib/langspec \
    -I$(top_srcdir)/lib/preprocessor \
    -I$(top_srcdir)/lib/regex

reflex_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-langspec.la \
    $(top_builddir)/lib/libbarf-commonlang.la \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-regex.la \
    $(top_builddir)/lib/libbarf-core.la

reflex_SOURCES = \
	app/reflex/reflex_ast.cpp \
	app/reflex/reflex_ast_representation.cpp \
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

reflex-parser-force: $(top_srcdir)/app/reflex/reflex_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/reflex/reflex_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/reflex/reflex_parser --generate-states-file $(top_srcdir)/app/reflex/reflex_parser.states

reflex-parser-release: $(top_srcdir)/app/reflex/reflex_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/reflex/reflex_parser.trison --warnings-as-errors --without-line-directives --output-basename $(top_srcdir)/app/reflex/reflex_parser --generate-states-file $(top_srcdir)/app/reflex/reflex_parser.states
else # !STABLE_TRISON_EXISTS
reflex-parser reflex-parser-force reflex-parser-release:
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

trison-parser-force: $(top_srcdir)/app/trison/trison_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/trison/trison_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/trison/trison_parser --generate-states-file $(top_srcdir)/app/trison/trison_parser.states

trison-parser-release: $(top_srcdir)/app/trison/trison_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/trison/trison_parser.trison --warnings-as-errors --without-line-directives --output-basename $(top_srcdir)/app/trison/trison_parser --generate-states-file $(top_srcdir)/app/trison/trison_parser.states
else # !STABLE_TRISON_EXISTS
trison-parser trison-parser-force trison-parser-release:
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
    -I$(top_srcdir)/lib/langspec \
    -I$(top_srcdir)/lib/preprocessor

trison_unstable_LDADD = \
    $(LDADD) \
    $(top_builddir)/lib/libbarf-commonlang.la \
    $(top_builddir)/lib/libbarf-langspec.la \
    $(top_builddir)/lib/libbarf-preprocessor.la \
    $(top_builddir)/lib/libbarf-core.la

trison_unstable_SOURCES = \
	app/trison.unstable/trison_ast.cpp \
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

trison-unstable-parser-force: $(top_srcdir)/app/trison.unstable/trison_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/trison.unstable/trison_parser.trison --warnings-as-errors --output-basename $(top_srcdir)/app/trison.unstable/trison_parser --generate-states-file $(top_srcdir)/app/trison.unstable/trison_parser.states

trison-unstable-parser-release: $(top_srcdir)/app/trison.unstable/trison_parser.trison $(STABLE_TRISON)
	$(STABLE_TRISON) $(top_srcdir)/app/trison.unstable/trison_parser.trison --warnings-as-errors --without-line-directives --output-basename $(top_srcdir)/app/trison.unstable/trison_parser --generate-states-file $(top_srcdir)/app/trison.unstable/trison_parser.states
else # !STABLE_TRISON_EXISTS
trison-unstable-parser trison-unstable-parser-force trison-unstable-parser-release:
	@echo "!!!"
	@echo "!!! Can't build trison.unstable parser without stable trison binary."
	@echo "!!! Use --with-stable-trison configure option."
	@echo "!!!"
endif

else # !BUILD_TRISON_UNSTABLE

trison-unstable-parser trison-unstable-parser-force trison-unstable-parser-release:
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
	lib/core/barf_message.hpp \
	lib/core/barf_optionsbase.hpp \
	lib/core/barf_pointer.hpp \
	lib/core/barf_searchpath.hpp \
	lib/core/barf_types.hpp \
	lib/core/barf_util.hpp \
	\
	lib/langspec/barf_langspec.hpp \
	lib/langspec/barf_langspec_ast.hpp \
	lib/langspec/barf_langspec_enums.hpp \
	lib/langspec/barf_langspec_parser.hpp \
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
	lib/regex/barf_regex_parser.hpp
