# BARF - Bison Awesomely Replaced, Flex

A suite of compiler tools by Victor Dods

This suite contains programs to supplant the useful, but dated, tools `bison`
(LALR(1) parser generator) and `flex` (lexical scanner generator):
-   `bpp` - The BARF Preprocessor.  A preprocessor vaguely similar to (but not
    intended to emulate) the C preprocessor, only with formal looping
    constructs, arrays and maps.
-   `reflex` - A lexical scanner generator.  Produces arbitrary-language scanners
    using data-driven specifications.  Currently "only" provides a cpp target.
-   `trison` - An LALR parser generator.  Produces arbitrary-language parsers
    using data-driven specifications.  Currently "only" provides a cpp target
    which is an NPDA parser (General LR parser).

## Release Notes

This lists versions 2 and later only.  Note that BARF follows non-strict semantic versioning;
in particular, the major revision number certainly indicates backward-incompatible changes, and
while the minor revision number usually indicates backward-compatible changes, there may be
minor incompabilities.  The patch revision number indicates bugfixes and minor changes.

-   `2.2.0` : Added a method to the `trison.cpp` target related to the
    `default_max_allowable_lookahead_queue_size` directive that should have been there in
    version `2.1.0`.  This constitutes a minor revision because a new API method was added.
-   `2.1.0` : Addition of `default_max_allowable_lookahead_queue_size` to `trison.cpp` target,
    which provides a tool for preventing infinite loops due to insufficient error-handling rules
    in a grammar.
-   `2.0.0` : Long-awaited release of BARF 2.0!  Mainly involves major improvements to `trison`.
    In particular, trison.cpp target was refactored to be more robust (though it currently only
    uses an NPDA implementation).

### Earlier Releases

While the first version of `trison` came about in 2006, official packages were only
created in 2010.  `reflex` is more or less the same as back then, but `trison` has
improved drastically over time.

-   `0.9.20101002` : http://thedods.com/victor/barf-0.9.20101002.tar.bz2
-   `0.9.20100116` : http://thedods.com/victor/barf-0.9.20100116.tar.bz2

## Building and Installing

From the project root, create a build directory (e.g. `build`), run `cmake` from
within in, then build and install.  Example commands:

    git clone https://github.com/vdods/barf.git
    cd barf
    mkdir build
    cd build
    cmake -D CMAKE_BUILD_TYPE=Release ..

The above invocation of cmake specifies `Release` mode, which will generate Makefiles
to build the tools with compiler optimizations enabled.  Then build and install the
tools as follows.

    make
    make install

On Linux (and probably Mac OS X) this will install the resources to `/usr/local/lib/barf`
which notably includes the barf targets dir, which contains the code templates for
generating code from reflex or trison source.  The tool binaries `bpp`, `reflex`, and
`trison` will be installed to `/usr/local/bin`.  The install location can be modified
in the usual cmake way using the config variable `CMAKE_INSTALL_PREFIX` (which in this
case is `/usr/local`).  To run cmake in interactive mode:

    cd build
    ccmake ..

The `ccmake` tool is a curses interface for cmake which is easy to use.

## Using BARF in Another Project

If `barf` has been properly installed, the files necessary for cmake's `find_package`
to work will be installed, such that an external project can incorporate BARF using
the following code in its `CMakeLists.txt` file.

    find_package(barf REQUIRED)

This defines the functions

    reflex_add_source(SOURCE_FILE TARGET_NAME)
    trison_add_source(SOURCE_FILE TARGET_NAME)

which when invoked create build rules for generating C++ code from reflex code or
trison code respectively.

## Examples

There are several reflex scanners and trison parsers within the BARF codebase itself
which function as realistic examples.
-   [reflex_parser](app/reflex/reflex_parser.trison)
-   [trison parser](app/trison/trison_parser.trison)
-   [barf_commonlang_scanner](lib/commonlang/barf_commonlang_scanner.reflex)
-   [barf_preprocessor_parser](lib/preprocessor/barf_preprocessor_parser.trison)
-   [barf_preprocessor_scanner](lib/preprocessor/barf_preprocessor_scanner.reflex)
-   [barf_regex_parser](lib/regex/barf_regex_parser.trison)
-   [barf_targetspec_parser](lib/targetspec/barf_targetspec_parser.trison)

Additional examples, presented as standalone programs, complete with cmake-based build
systems, are presented under the `examples` directory.
-   [interactive_calculator parser](examples/interactive_calculator/calculator_parser.trison)
-   [interactive_calculator scanner](examples/interactive_calculator/calculator_scanner.reflex)
-   [noninteractive_calculator parser](examples/noninteractive_calculator/CalcParser.trison)
-   [noninteractive_calculator scanner](examples/noninteractive_calculator/Scanner.reflex)

## Licensing

The files generated using this tool suite (bpp, reflex, trison) are the
property of the user of the tools.  The generated code can therefore be
licensed any way the user wishes, and thus the generated code is compatible
with being used in any software (proprietary, open-source, etc).  In other
words, the code generated by the tools (bpp, reflex, trison) is not subject
to the license (GPL version 2) specified in the LICENSE file.

Unless a different license was explicitly granted in writing by the copyright
holder (Victor Dods), this software (BARF; in particular, the codebase that
produces the tools bpp, reflex, trison) is freely distributable under the
terms of the GNU General Public License, version 2. Any works deriving from
this work (BARF) must also be released under the GNU GPL.  See the included
file LICENSE for details.

## Thanks

To Dan Palm for suggesting BARF (as Bison's Awesome Replacement Framework) as
a name, and for being the first other person to attempt to develop anything
using it.  He created a scripting language called
[Steel](https://github.com/Nightwind0/steel) for his game engine
[Stone Ring](https://github.com/Nightwind0/stonering).

## Development Notes

### `dev` and `metadev` targets

To test changes to `reflex` and `trison`, a set of build targets have been created
which run the built `reflex` and `trison` binaries on all the `.reflex` and `.trison`
sources in BARF's own codebase, producing parser and scanner code (this is the content
of the build dir subdir `dev`), compiles versions of those binaries using the scanner
and parser code in `dev` (these are `dev_reflex` and `dev_trison`), and then uses those
binaries on all the `.reflex` and `.trison` sources, producing parser and scanner code
(this is the content of the build dir subdir `metadev`), so that the output of the `dev`
and `metadev` stages can be compared and verified to be identical.

To run these checks, build the `metadev_check` target.  E.g.

    make metadev_check

Success is when the produced `dev` and `metadev` subdirs of the build dir are identical.
This build target should work even from a fresh build dir, and will build `bpp`, `reflex`,
`trison`, and `playground` in the process.

CMake probably handles the dependencies of these targets correctly, but if you want to
make absolutely sure that fresh sources have been generated and targets built, then run
the following command before `make metadev_check`.

    make clean clean_all

## To-dos

Some of these are super old and may no longer apply.

### Bugs

-   trison: `POP_STACK 2` transitions (for when %end is the lookahead and it shouldn't be discarded)
    are not exercised if the stack would be empty after the pop.  But this should happen, because
    that's a pretty crucial error state to handle, which if not handled results in an infinite loop
    in the parser.
-   disallow empty reflex states
-   reflex: an unterminated regex causes a hang (while reflex is parsing the *.reflex source file):

        %state_machine BLAH
        :
            (
        ;

-   bug (in trison) -- lack of a parse error handler for:

        %terminal XXX %type "Blah"

    which should be

        %terminal XXX %type.cpp "Blah"

-   looks like trison doesn't catch EOF before ; in a nonterminal declaration

### BARF General To-dos

-   write man pages for reflex, trison, bpp (hopefully somehow get doxygen to
    do this)
-   figure out how to generate/install docs
-   guarantee that errors will be reported such that the line numbers never
    decrease.
-   bug: getting "file not found" error when a file exists but the permissions
    are such that the user can't read it.
-   add lots of error rules to all parser grammars
-   descriptions should use terms "finite-automaton-based scanners" and
    "pushdown-automaton-based parsers"
-   error when any output file is the same as an input file, or when any two
    output files are the same.
-   change comments to %/ and %* *% in primary sources?
-   move trison/reflex stuff into lib/trison and lib/reflex? and only keep
    main, options and main header file in app dir (so test apps can link the
    libs and run tests)?  put stuff in Barf::Xxx namespace
-   do comprehensive deleting of stuff (this might be hard with all the throwing)
    THEN
    do memory leak checking (valgrind)
-   get rid of AST hierarchy overviews once doxygen docs are to some decent state
-   use Ast::Directive::DirectiveString in error messages instead of
    creating the error messages by hand -- make all directives derive from
    Ast::Directive
-   file locations should be "2d range" -- a starting line/column and an ending
    line/column (or starting line/column and a number of bytes following).
    nonterminal expressions can then be the union of the file locations of their
    component parts.  this technique will be useful later in compiler design for
    debugging info.
-   add filename-sanitizing function -- i.e. to check for newlines and other stuff
-   save off FiLoc where each error panic starts, so that if it's a general parse
    error, at least some info can be printed.
-   Change doxygen comments to all /// style
-   Ensure all the different types of cleanup are performed, verify with valgrind.

### CMake-related To-dos

-   Change shell commands (e.g. invocations of `make`, `rm`, `diff`) over to use
    corresponding cmake commands, such as `file(REMOVE ...)`, etc.

### Documentation To-dos

-   table of contents inside each doxygen_pages page
-   pyramidal documentation
-   simplify documentation (maybe leave out basic regex stuff, keep to essentials)

### Commonlang To-dos

-   target directives (including rule handlers) should accept multiple targets ? like

        %target.cpp_nostl,cpp_withstl,cpp_noexceptions %{ blah %}

### Core To-dos

-   make a Graph::Node::Index type (a struct or an enum with a value
    UINT32_UPPER_BOUND (to force it to be unsigned)) and Graph::Node::IndexArray
    vector type in barf_graph.h (replacing the ad-hoc vector<Uint32> used all
    over the place)
-   check standard on octal escape chars (up to 3 octal digits only?)
-   make EscapeChar (which does full char escaping) and use it in EscapeString?
    this would make the char scanning much easier
-   make repeated identical (or perhaps just very similar) warnings/errors
    suppressed (e.g. in preprocessor, having an error in a loop will cause that
    error to be emitted that many times)

### Targetspec To-dos

-   allow using the value of previously declared directives as default values
    (this would be had for free if targetspec-declared, primary-source-defined
    codespec symbols are allowed to use codespec symbols -- see codespec section)
-   add targetspec versions (target-specific) -- so you can compile/link
    multiple coexisting versions of the same target but at different versions.
    do this by sticking a version number in the namespace of the generated
    baseclasses

### Codespec To-dos

-   add comment/description element to each codespec symbol, so that you can
    print out a list of all available codespec symbols and their descriptions
    from the commandline for easier development (i.e. to the state machine ones
    so that you don't have to look through xxx_codespecsymbols.cpp for these)
-   make better error message (i.e. anything informative at all) when there's
    an error in an included codespec file.

### Preprocessor To-dos

-   add parser error handler for when argument to is_defined is not an identifier
-   just make the preprocessor language better in general -- it's sort of crappy
-   add a printf function
-   add some way to use a different file location for errors/warnings
-   add comments (C++ and C style)
-   make _blah ids not be able to be defined in-file, they're reserved
-   add assignment operator; e.g. <{x=x+3}
-   add eval directive so you can parse and execute a block of text as a
    preprocessor body.
-   add 64 bit ints (better) and add whatever the negative-most 64-bit int is
    to the scanner (because the way negative ints are constructed in the parser
    screws this up)
-   maybe make the Textifier detect if a newline was just printed, so that
    it doesn't print unnecessary newlines when generating line directives
-   add boolean property for wether or not a particular symbol was referenced
    during preprocessing, so that warnings can be emitted if a symbol isn't used
-   Could add "indent" and "unindent" directives in order to better format code.
    Would need this to work inside "if" blocks.
-   Add ability to print list of dependencies (i.e. included files).  This would
    depend on the include path as well as the preprocessor variable values themselves.

### Regex To-dos

-   document all of this regex stuff in detail, since it's pretty complicated,
    and not completely standard
-   in graph printing, TT_INPUT_ATOM_RANGEs which include [ or ] as the ending
    char need to escape them
-   generally fix up the escaping of characters in display strings and graphs
    to escape in the context of the regex.
-   make a parse-string method which returns a Regex::RegularExpression and
    possibly throws to indicate errors.
-   move NodeData into separate file
-   improve the DFA generation so that when throwing an exception when there is
    a conditional conflict, it includes the file location that caused it.
-   add a pass to NFA generation where it optimizes unnecessary epsilon
    transitions out.
-   bracket expression transitions should use single-atom transitions when possible

### BPP To-dos

-   add predefine (-D) option

### Reflex To-dos

-   instead of always extracting the accepted string and passing it in,
    make it available through an accessor, so it can be constructed lazily.
    OR
    somehow just return pointers into the buffer for the accepted string, so
    that there is as little overhead as possible when running the scanner.
    Could do this with std::span (newer C++ standard)
-   maybe consistencize the codespec symbol names

### Reflex/Trison Common To-dos

-   use smallest integer type to store state and transition indices, etc.
    (in C++, this can be done easily with templates)
-   move the --with-line-directives option to be an optional targetspec directive
-   add targetspec directive for line directives referencing codespecs (the
    default should be to not refer to codespecs), making development easier
-   add option which prints the reflex/trison targets available in the current
    search path and which directory each resides in.
-   the rule handler code block type should be specifiable by the targetspec
-   specifying any %target.xxx directive seems to add the target; stop this
-   allow "%targets xxxx" in pre/postdefine
-   also add post-undefine option
-   warn about targetspec directive values that weren't referenced in the
    code generation (preprocessor) stage -- ones that don't appear at all in
    any codespec, not necessarily to indicate they weren't used in one particular
    case.
-   Ensure that commandline-specified values for parser directives override in-source values,
    so that parser generation can be controlled from the build system if needed, e.g. for unit testing.
-   Run preprocessor on code blocks in reflex and trison, so that e.g. stuff like this can work
    in a reflex/trison code block.

        Parser::ParserReturnCode parse (std::string const &s, double &parsed_value)
        {
            std::istringstream in(s);
            Parser parser;
            parser.attach_istream(in);
        <|if(is_defined(generate_debug_spew_code))
            parser.DebugSpew(true);
        <|end_if
            Parser::ParserReturnCode return_code = parser.Parse(&parsed_value);
            return return_code;
        }

-   Ensure that commandline-specified values for scanner/parser directives override in-source values,
    so that scanner/parser generation can be controlled from the build system if needed, e.g. for unit testing.
-   Run preprocessor on code blocks in reflex and trison, so that e.g. stuff like this can work
    in a reflex/trison code block.

        Parser::ParserReturnCode parse (std::string const &s, double &parsed_value)
        {
            std::istringstream in(s);
            Parser parser;
            parser.attach_istream(in);
        <|if(is_defined(generate_debug_spew_code))
            parser.DebugSpew(true);
        <|end_if
            Parser::ParserReturnCode return_code = parser.Parse(&parsed_value);
            return return_code;
        }

-   Probably restructure the `targets` directory to be hierarchical, like

        targets
            reflex
                cpp
                    <associated files>
            trison
                cpp
                    <general use trison cpp files>
                    dpda
                        <associated files
                    npda
                        <associated files>

    and then make the include directive use that directory hierarchy.  Probably require full
    paths in include directive, and disallow relative paths.

### Trison To-dos

-   pass reference instead of double pointer to Parse
-   refactor the DPDA generation to be simpler and faster (difficult)
-   figure out why minimal graphing isn't working (the right lookaheads
    aren't being generated).
-   if possible, make the dpda.states file put the states' rules in order.
-   add a max token stack depth in each parser implementation
-   you shouldn't have to specify %terminal 'X' for ascii chars.  you should
    just be able to use them (or maybe add as a directive)
-   check for reduction rule variable name collisions
-   decouple npda states from trison ASTs -- they should only exist in the
    graph itself.
-   add an unbounded grammar app/dev test case, and deal with unbounded grammars
    (since DPDAs can't be generated for them)
-   compactify lookaheads in dpda
-   make sure lookahead sequences are alphabetized, allowing for a binary search
    instead of a linear search (in the dpda target)
-   unit tests for NPDA
-   add some nice metrics to the NPDA parser -- make this part of the parser API
    -   max lookahead count (done)
    -   max memory used
    -   some measurement of the "leanness" of the grammar, like how much branching
        was encountered, etc
-   in the NPDA dot graph, maybe move the self-transitions to just text lines in the
    state box itself, to avoid the mess that dot creates with self transitions.
-   maybe add some online DPDA generation in NPDA during runtime, optionally saving
    the tables to a file.  The file should be able to be specified as "read-only".
    This would allow a balance between DPDA-generation for commonly used grammar pathways
    and the flexibility of NPDA parsers.
-   port NPDA code to offline DPDA generation (after NPDA has been fully unit tested)
-   NPDA parser test design notes
    -   need to test a bunch of different grammars
        -   grammars with bounded lookahead (1, 2, 3, and 0 if that's possible)
        -   grammars with unbounded lookahead
        -   try to come up with a worst case grammar
        -   look on wikipedia and other websites for grammars
        -   use actual grammars like from barf, steel, etc.
    -   for each grammar, make sure each rule is exercised
    -   for each grammar, make sure each combination of rules (i.e. binary operators) is exercised
    Unit test should be compiled into a single binary.  Each Parser should have a distinct
    name, and the various inputs to the parsers should be stored in files.  The outputs should
    be ASTs, and compared against "expected" ASTs which can be defined in code.
-   Add an error handling rule for trison:

        %nonterminal maybe_assigned_id %type "Ast::Id *"

    caused the generic error `syntax error in %nonterminal directive` because of missing `.cpp`

-   Probably compute epsilon closure of states during trison execution, so that the NPDA implementation
    doesn't have to implement it.  This could also be used for the NPDA states file, so that it requires
    the reader to do less work.
-   Perhaps in NPDA states file, show epsilon closure and transitions from all states in that epsilon
    closure, so that the human reader doesn't have to do as much mental work.
-   Collapse some options together using arguments (e.g. the pairs of "do" and "don't" options).
-   Add [ ] brackets to "symbol" class in kate syntax highlighting file.
-   Add symbols and strings/chars to the body section of codespec kate syntax highlighting file, so that
    at least some syntax highlighting is done in the body.
-   If "indent" and "unindent" directives were present in preprocessor, then Python targets would be
    possible (since they're indentation-sensitive).  Would need this to work inside "if" blocks.
-   Make the npda states file grammar section print out the precedence levels and print the precedence
    for each rule (including default)
-   There appears to be a crash bug in trison where an undeclared terminal is used in the bracketed
    %error[...] syntax.
-   Should re-add the specification of which stage of each rule it is in the npda state file,
    probably as a tuple, like if there is

        rule 123 : blah <- A B C

    then there would be

        rule 123,0 : blah <- . A B C
        rule 123,1 : blah <- A . B C
        rule 123,2 : blah <- A B . C
        rule 123,3 : blah <- A B C .

-   Allow an id to be associated with an %error directive, where the token associated with the
    %error directive is produced by the error handling actions.  This is so the parser can store
    a file location (or range) for the error.  This would support the idea of making ERROR_ into
    a nonterminal, which would then be called error_.
-   Probably just get rid of ResetForNewInput, favoring destroying and recreating the parser/scanner.
    At least examine if there's any reason to keep that feature.
-   Make an option in reflex and trison for printing a list of dependent targetspec and codespec files
    for a given source or target.  I.e. the input file is parser.trison and it specifies targets
    target.cpp and target.fakelang, and then it prints target.cpp.targetspec, target.cpp.header.codespec,
    target.cpp.implementation.codespec, target.cpp.npda.header.codespec, target.cpp.npda.implementation.codespec,
    ..., target.fakelang.targetspec target.fakelang.codespec.  This way, build processes can be more
    automated and require less hand-specification.

    Similarly, make an option where it prints the files that would be generated by a command (i.e.
    thing_parser.trison -> thing_parser.hpp thing_parser.cpp

-   Should probably make NPDA DAG generation forbid an INSERT_ERROR_TOKEN action when the lookahead
    is ERROR_.  This would prevent one class of infinite loop.
