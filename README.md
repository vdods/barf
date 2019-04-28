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

