// ///////////////////////////////////////////////////////////////////////////
// reflex_options.cpp by Victor Dods, created 2006/10/20
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "reflex_options.hpp"

namespace Reflex {

CommandLineOption const Options::ms_option[] =
{
    CommandLineOption("Warning and error options"),
    CommandLineOption(
        'W',
        "warnings-as-errors",
        &OptionsBase::TreatWarningsAsErrors_Enable,
        "    Treat warnings as errors.  See also option -w."),
    CommandLineOption(
        'w',
        "warnings-not-as-errors",
        &OptionsBase::TreatWarningsAsErrors_Enable,
        "    Warnings will not be treated as errors, and thus not abort the execution\n"
        "    of the program.  This is the default behavior.  See also option -W."),
    CommandLineOption(
        'E',
        "halt-on-first-error",
        &OptionsBase::HaltOnFirstError_Enable,
        "    All errors (and warnings, if option -W is specified) will be considered\n"
        "    fatal, and will abort execution immediately.  See also option -e."),
    CommandLineOption(
        'e',
        "dont-halt-on-first-error",
        &OptionsBase::HaltOnFirstError_Disable,
        "    The program will continue executing as long as possible before aborting\n"
        "    when errors occur.  This is the default behavior.  See also option -E."),
#if DEBUG
    CommandLineOption(
        'A',
        "assert-on-error",
        &OptionsBase::AssertOnError_Enable,
        "    All errors (and warnings, if option -W is specified) will cause an assert\n"
        "    which is only useful if you're developing BARF tools (in fact you should\n"
        "    only see this option if the binaries were compiled in debug mode).  See\n"
        "    also option -a."),
    CommandLineOption(
        'a',
        "dont-assert-on-error",
        &OptionsBase::AssertOnError_Disable,
        "    This negates the effect of option -A, and is the default behavior."),
#endif


    CommandLineOption("Input behavior options"),
    CommandLineOption(
        'c',
        "clear-targets-search-path",
        &OptionsBase::ClearSearchPath,
        "    Clears the existing search path."),
    CommandLineOption(
        'I',
        "include-targets-search-path",
        &OptionsBase::IncludeSearchPath,
        "    Adds an entry to the search path for data files (e.g. targetspec and\n"
        "    codespec files), with higher search priority than the existing entries.\n"
        "    By default the search path is specified by the BARF_TARGETS_SEARCH_PATH\n"
        "    enviroment variable.  If no valid directories are specified, the search\n"
        "    path will default to the current working directory."),
    CommandLineOption(
        'p',
        "print-targets-search-path",
        &OptionsBase::RequestShortPrintSearchPath,
        "    Prints to stdout, from highest priority to lowest, the targets search path\n"
        "    once all -c and -I options have been processed, then exits.  Each path entry\n"
        "    is delimited by a newline."),
    CommandLineOption(
        'P',
        "print-verbose-targets-search-path",
        &OptionsBase::RequestVerbosePrintSearchPath,
        "    Prints to stdout, from highest priority to lowest, the targets search path\n"
        "    once all -c and -I options have been processed, then exits.  Each path entry\n"
        "    is followed by a description of how each was added.  Each entry is delimited\n"
        "    by a newline.  This is identical to option -p except for the description."),


    CommandLineOption("Output behavior options"),
    CommandLineOption(
        'o',
        "output-directory",
        &OptionsBase::SetOutputDirectory,
        "    Specifies the target directory for generated code.  The default is the\n"
        "    current working directory."),
    CommandLineOption(
        'L',
        "with-line-directives",
        &Options::WithLineDirectives_Enable,
        "    Use #line directives in the generated source, so that compile errors and\n"
        "    debugging will use the parser source file when appropriate.  This is the\n"
        "    default behavior.  See also option -l."),
    CommandLineOption(
        'l',
        "without-line-directives",
        &Options::WithLineDirectives_Disable,
        "    Do not use #line directives in the generated source.  This might be helpful\n"
        "    when the original parser source will not be available during compilation or\n"
        "    debugging, or if the target language doesn't support #line directives to\n"
        "    begin with.  See also option -L."),
    CommandLineOption(
        'r',
        "line-directives-relative-to-path",
        &Options::SetLineDirectivesRelativeToPath,
        "    If #line directives are to be used in the generated source, then make the\n"
        "    paths relative to the given path.  The default is the empty string, meaning\n"
        "    #line directives will not be made relative."),
    CommandLineOption(
        "generate-nfa-dot-graph",
        &OptionsBase::GenerateNaDotGraph,
        "    Specifies the filename for a `dot` graph (see http://www.graphviz.org/)\n"
        "    of the generated nondeterministic finite automata (NFA).  Useful as a\n"
        "    diagnostic tool.  Specifying - (hyphen) indicates that the output should\n"
        "    be to stdout.  See also option --dont-generate-nfa-dot-graph."),
    CommandLineOption(
        "dont-generate-nfa-dot-graph",
        &OptionsBase::GenerateNaDotGraph_Disable,
        "    Do not create a `dot` graph file for the NFA.  This is the default\n"
        "    behavior.  See also option --generate-nfa-dot-graph."),
    CommandLineOption(
        "generate-dfa-dot-graph",
        &OptionsBase::GenerateDaDotGraph,
        "    Specifies the filename for a `dot` graph (see http://www.graphviz.org/) of\n"
        "    the generated deterministic finite automata (DFA; which is generated from\n"
        "    the NFA).  Useful as a diagnostic tool.  Specifying - (hyphen) indicates\n"
        "    that the output should be to stdout.  See also option\n"
        "    --dont-generate-dfa-got-graph."),
    CommandLineOption(
        "dont-generate-dfa-dot-graph",
        &OptionsBase::GenerateDaDotGraph_Disable,
        "    Do not create a `dot` graph file for the DFA.  This is the default\n"
        "    behavior.  See also option --generate-dfa-dot-graph."),


    CommandLineOption("Target-related options"),
    CommandLineOption(
        'D',
        "predefine",
        &OptionsBase::AddPredefine,
        "    Defines a targetspec directive value for a given target before parsing\n"
        "    the primary source.  Directive values specified in the primary source will\n"
        "    override values defined via this commandline option.  The argument is of\n"
        "    the form:\n"
        "        %target.<target_id>.<directive_id> <value>\n"
        "    The <value> portion of the argument must include the delimiters appropriate\n"
        "    to the targetspec directive (e.g. %string directives require a double-quote-\n"
        "    delimited <value>, etc).  See also option -d."),
    CommandLineOption(
        'd',
        "postdefine",
        &OptionsBase::AddPostdefine,
        "    Defines a targetspec directive value for a given target after parsing\n"
        "    the primary source.  Directive values specified in the primary source will\n"
        "    be overridden by values defined via this commandline option.  See option\n"
        "    -D for details on the form of the argument."),


    CommandLineOption("Verbosity options"),
    CommandLineOption(
        'V',
        "enable-verbosity",
        &OptionsBase::EnableVerbosity,
        "    Enables the specified verbosity option.  Valid parameters are\n"
        "        \"execution\" - Print general application activity to stderr.\n"
        "        \"scanner\" - Print primary source (*.reflex) scanner activity to stderr.\n"
        "        \"parser\" - Print primary source (*.reflex) parser activity to stderr.\n"
        "        \"ast\" - Print the parsed primary source abstract syntax tree to stderr.\n"
        "        \"targetspec-scanner\" - Print targetspec scanner activity to stderr.\n"
        "        \"targetspec-parser\" - Print targetspec parser activity to stderr.\n"
        "        \"targetspec-ast\" - Print the parsed targetspec abstract syntax tree(s) to stderr.\n"
        "        \"codespec-scanner\" - Print codespec scanner activity to stderr.\n"
        "        \"codespec-parser\" - Print codespec parser activity to stderr.\n"
        "        \"codespec-ast\" - Print the parsed codespec abstract syntax tree(s) to stderr.\n"
        "        \"codespec-symbols\" - Print the preprocessor symbols available to the codespec to stderr.\n"
//         "        \"regex-scanner\" - Print regex scanner activity to stderr. (not currently implemented)\n"
        "        \"regex-parser\" - Print regex parser activity to stderr.\n"
        "        \"regex-ast\" - Print the parsed regex abstract syntax tree(s) to stderr.\n"
        "        \"all\" - Enable all above verbosity options.\n"
        "    All verbosity options are disabled by default.  See also option -v."),
    CommandLineOption(
        'v',
        "disable-verbosity",
        &OptionsBase::DisableVerbosity,
        "    Disables the specified verbosity option.  See option -V for\n"
        "    valid parameters and their descriptions."),


    CommandLineOption(""),
    CommandLineOption(
        'h',
        "help",
        &OptionsBase::RequestHelp,
        "    Prints this help message.")
};
Uint32 const Options::ms_option_count = sizeof(Options::ms_option) / sizeof(CommandLineOption);

Options::Options (string const &executable_filename)
    :
    OptionsBase(
        "reflex",
        &Options::SetInputFilename,
        ms_option,
        ms_option_count,
        executable_filename,
        "Reflex - a lexical scanner generator - version " PACKAGE_VERSION "\n"
        "Part of the BARF compiler tool suite - written by Victor Dods",
        "[options] <input_filename>")
{ }

void Options::Parse (int const argc, char const *const *const argv)
{
    OptionsBase::Parse(argc, argv);

    // only check for commandline option consistency if no output-and-quit
    // option was specified (e.g. help or print-targets-search-path)
    if (!AbortFlag() &&
        !IsHelpRequested() &&
        GetPrintSearchPathRequest() == PSPR_NONE)
    {
        if (InputFilename().empty())
            ReportErrorAndSetAbortFlag("no input filename specified");
    }
}

} // end of namespace Reflex
