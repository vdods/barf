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

spotless: clean clean-dumblang-parser clean-dumblang-scanner clean-dumblang-parser-extras clean-dumblang-scanner-extras

clean-dumblang-parser: clean-dumblang-parser-extras
	rm -f $(top_srcdir)/dumblang_parser.cpp
	rm -f $(top_srcdir)/dumblang_parser.hpp

clean-dumblang-parser-extras:
	rm -f $(top_srcdir)/dumblang_parser.dpda.dot
	rm -f $(top_srcdir)/dumblang_parser.dpda.png
	rm -f $(top_srcdir)/dumblang_parser.npda.dot
	rm -f $(top_srcdir)/dumblang_parser.npda.png
	rm -f $(top_srcdir)/dumblang_parser.dpda.states

clean-dumblang-scanner: clean-dumblang-scanner-extras
	rm -f $(top_srcdir)/dumblang_scanner.cpp
	rm -f $(top_srcdir)/dumblang_scanner.hpp

clean-dumblang-scanner-extras:
	rm -f $(top_srcdir)/dumblang_scanner.dfa.dot
	rm -f $(top_srcdir)/dumblang_scanner.dfa.png
	rm -f $(top_srcdir)/dumblang_scanner.nfa.dot
	rm -f $(top_srcdir)/dumblang_scanner.nfa.png

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
    $(LDADD)

dumblang_SOURCES = \
	dumblang_parser.cpp \
	dumblang_parser.trison \
	dumblang_scanner.cpp \
	dumblang_scanner.reflex

if REFLEX_EXISTS
dumblang-scanner: $(top_srcdir)/dumblang_scanner.hpp
$(top_srcdir)/dumblang_scanner.hpp: $(top_srcdir)/dumblang_scanner.cpp
$(top_srcdir)/dumblang_scanner.cpp: $(top_srcdir)/dumblang_scanner.reflex $(REFLEX) $(TARGETS_PATH)/reflex.cpp.targetspec $(TARGETS_PATH)/reflex.cpp.header.codespec $(TARGETS_PATH)/reflex.cpp.implementation.codespec
	$(REFLEX) $(top_srcdir)/dumblang_scanner.reflex --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-nfa-dot-graph dumblang_scanner.nfa.dot --generate-dfa-dot-graph dumblang_scanner.dfa.dot

force-dumblang-scanner:
	$(REFLEX) $(top_srcdir)/dumblang_scanner.reflex --include-targets-search-path $(TARGETS_PATH)  --warnings-as-errors --output-directory $(top_srcdir)/ --generate-nfa-dot-graph dumblang_scanner.nfa.dot --generate-dfa-dot-graph dumblang_scanner.dfa.dot
else # !REFLEX_EXISTS
dumblang-scanner force-dumblang-scanner:
	@echo "!!!"
	@echo "!!! Can't build dumblang scanner without reflex binary."
	@echo "!!! Use --with-reflex configure option."
	@echo "!!!"
endif

if DOT_EXISTS
if CONVERT_EXISTS
$(top_srcdir)/dumblang_scanner.dfa.png: $(top_srcdir)/dumblang_scanner.dfa.dot
	$(DOT) $(top_srcdir)/dumblang_scanner.dfa.dot -Tpng -o $(top_srcdir)/dumblang_scanner.dfa.png
	$(CONVERT) $(top_srcdir)/dumblang_scanner.dfa.png -resize 66%x66% $(top_srcdir)/dumblang_scanner.dfa.png

$(top_srcdir)/dumblang_scanner.nfa.png: $(top_srcdir)/dumblang_scanner.nfa.dot
	$(DOT) $(top_srcdir)/dumblang_scanner.nfa.dot -Tpng -o $(top_srcdir)/dumblang_scanner.nfa.png
	$(CONVERT) $(top_srcdir)/dumblang_scanner.nfa.png -resize 66%x66% $(top_srcdir)/dumblang_scanner.nfa.png
else
$(top_srcdir)/dumblang_scanner.dfa.png: $(top_srcdir)/dumblang_scanner.dfa.dot
	$(DOT) $(top_srcdir)/dumblang_scanner.dfa.dot -Tpng -o $(top_srcdir)/dumblang_scanner.dfa.png

$(top_srcdir)/dumblang_scanner.nfa.png: $(top_srcdir)/dumblang_scanner.nfa.dot
	$(DOT) $(top_srcdir)/dumblang_scanner.nfa.dot -Tpng -o $(top_srcdir)/dumblang_scanner.nfa.png
endif
endif

if TRISON_EXISTS
dumblang-parser: $(top_srcdir)/dumblang_parser.hpp
$(top_srcdir)/dumblang_parser.hpp: $(top_srcdir)/dumblang_parser.cpp
$(top_srcdir)/dumblang_parser.cpp: $(top_srcdir)/dumblang_parser.trison $(TRISON) $(TARGETS_PATH)/trison.cpp_npda.targetspec $(TARGETS_PATH)/trison.cpp_npda.header.codespec $(TARGETS_PATH)/trison.cpp_npda.implementation.codespec
	$(TRISON) $(top_srcdir)/dumblang_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-npda-dot-graph dumblang_parser.npda.dot --generate-dpda-dot-graph dumblang_parser.dpda.dot --generate-dpda-states-file dumblang_parser.dpda.states

force-dumblang-parser:
	$(TRISON) $(top_srcdir)/dumblang_parser.trison --include-targets-search-path $(TARGETS_PATH) --warnings-as-errors --output-directory $(top_srcdir)/ --generate-npda-dot-graph dumblang_parser.npda.dot --generate-dpda-dot-graph dumblang_parser.dpda.dot --generate-dpda-states-file dumblang_parser.dpda.states
else # !TRISON_EXISTS
dumblang-parser force-dumblang-parser:
	@echo "!!!"
	@echo "!!! Can't build dumblang parser without stable trison binary."
	@echo "!!! Use --with-trison configure option."
	@echo "!!!"
endif

if DOT_EXISTS
if CONVERT_EXISTS
$(top_srcdir)/dumblang_parser.dpda.png: $(top_srcdir)/dumblang_parser.dpda.dot
	$(DOT) $(top_srcdir)/dumblang_parser.dpda.dot -Tpng -o $(top_srcdir)/dumblang_parser.dpda.png
	$(CONVERT) $(top_srcdir)/dumblang_parser.dpda.png -resize 66%x66% $(top_srcdir)/dumblang_parser.dpda.png

$(top_srcdir)/dumblang_parser.npda.png: $(top_srcdir)/dumblang_parser.npda.dot
	$(DOT) $(top_srcdir)/dumblang_parser.npda.dot -Tpng -o $(top_srcdir)/dumblang_parser.npda.png
	$(CONVERT) $(top_srcdir)/dumblang_parser.npda.png -resize 66%x66% $(top_srcdir)/dumblang_parser.npda.png
else
$(top_srcdir)/dumblang_parser.dpda.png: $(top_srcdir)/dumblang_parser.dpda.dot
	$(DOT) $(top_srcdir)/dumblang_parser.dpda.dot -Tpng -o $(top_srcdir)/dumblang_parser.dpda.png

$(top_srcdir)/dumblang_parser.npda.png: $(top_srcdir)/dumblang_parser.npda.dot
	$(DOT) $(top_srcdir)/dumblang_parser.npda.dot -Tpng -o $(top_srcdir)/dumblang_parser.npda.png
endif
endif

##############################################################################
# all the no-install header files for the applications (is this necessary?)
##############################################################################

noinst_HEADERS = \
	dumblang_parser.hpp \
	dumblang_scanner.hpp
