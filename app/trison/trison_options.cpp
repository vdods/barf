// ///////////////////////////////////////////////////////////////////////////
// trison_options.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_options.hpp"

namespace Trison {

CommandLineOption const Options::ms_option[] =
{
    CommandLineOption("Warning and error options"),
    CommandLineOption(
        'W',
        "warnings-as-errors",
        &OptionsBase::TreatWarningsAsErrors,
        "    Treat warnings as errors.  See also option -w."),
    CommandLineOption(
        'w',
        "warnings-not-as-errors",
        &OptionsBase::TreatWarningsAsErrors,
        "    Warnings will not be treated as errors, and thus not abort the execution\n"
        "    of the program.  This is the default behavior.  See also option -W."),
    CommandLineOption(
        'E',
        "halt-on-first-error",
        &OptionsBase::HaltOnFirstError,
        "    All errors (and warnings, if option -W is specified) will be considered\n"
        "    fatal, and will abort execution immediately.  See also option -e."),
    CommandLineOption(
        'e',
        "dont-halt-on-first-error",
        &OptionsBase::DontHaltOnFirstError,
        "    The program will continue executing as long as possible before aborting\n"
        "    when errors occur.  This is the default behavior.  See also option -E."),
#if DEBUG
    CommandLineOption(
        'A',
        "assert-on-error",
        &OptionsBase::AssertOnError,
        "    All errors (and warnings, if option -W is specified) will cause an assert\n"
        "    which is only useful if you're developing BARF tools (in fact you should\n"
        "    only see this option if the binaries were compiled in debug mode).  See\n"
        "    also option -a."),
    CommandLineOption(
        'a',
        "dont-assert-on-error",
        &OptionsBase::DontAssertOnError,
        "    This negates the effect of option -A, and is the default behavior."),
#endif


    CommandLineOption("Input behavior options"),
    CommandLineOption(
        'I',
        "include-targets-search-path",
        &OptionsBase::IncludeTargetsSearchPath,
        "    Adds another entry to the search path for data files (e.g. targetspec and\n"
        "    codespec files) with higher search priority than the existing entries.\n"
        "    By default the search path is specified by the BARF_TARGETS_SEARCH_PATH\n"
        "    enviroment variable.  If no valid directories are specified, the search\n"
        "    path will default to the current working directory."),
    CommandLineOption(
        'p',
        "print-targets-search-path",
        &OptionsBase::RequestShortPrintTargetsSearchPath,
        "    Prints to stdout, from highest priority to lowest, the targets search path\n"
        "    once all -I options have been processed, then exits.  Each path entry is\n"
        "    delimited by a newline."),
    CommandLineOption(
        'P',
        "print-verbose-targets-search-path",
        &OptionsBase::RequestVerbosePrintTargetsSearchPath,
        "    Prints to stdout, from highest priority to lowest, the targets search path\n"
        "    once all -I options have been processed, then exits.  Each path entry is\n"
        "    followed by a description of how each was added.  Each entry is delimited\n"
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
        &Options::WithLineDirectives,
        "    Use #line directives in the generated source, so that compile errors and\n"
        "    debugging will use the parser source file when appropriate.  This is the\n"
        "    default behavior.  See also option -l."),
    CommandLineOption(
        'l',
        "without-line-directives",
        &Options::WithoutLineDirectives,
        "    Do not use #line directives in the generated source.  This might be helpful\n"
        "    when the original parser source will not be available during compilation or\n"
        "    debugging, or if the target language doesn't support #line directives to\n"
        "    begin with.  See also option -L."),
    CommandLineOption(
        "generate-npda-dot-graph",
        &OptionsBase::GenerateNaDotGraph,
        "    Specifies the filename for a `dot` graph (see http://www.graphviz.org/)\n"
        "    of the generated nondeterministic pushdown automata (NPDA).  Useful as a\n"
        "    diagnostic tool.  Specifying - (hyphen) indicates that the output should\n"
        "    be to stdout.  See also option --dont-generate-npda-dot-graph."),
    CommandLineOption(
        "dont-generate-npda-dot-graph",
        &OptionsBase::DontGenerateNaDotGraph,
        "    Do not create a `dot` graph file for the NPDA.  This is the default\n"
        "    behavior.  See also option --generate-npda-dot-graph."),
    CommandLineOption(
        "generate-dpda-dot-graph",
        &OptionsBase::GenerateDaDotGraph,
        "    Specifies the filename for a `dot` graph (see http://www.graphviz.org/) of\n"
        "    the generated deterministic pushdown automata (DPDA; which is generated\n"
        "    from the NFA).  Useful as a diagnostic tool.  Specifying - (hyphen)\n"
        "    indicates that the output should be to stdout.  See also option\n"
        "    --dont-generate-dpda-dot-graph."),
    CommandLineOption(
        "dont-generate-dpda-dot-graph",
        &OptionsBase::DontGenerateDaDotGraph,
        "    Do not create a `dot` graph file for the DPDA.  This is the default\n"
        "    behavior.  See also option --generate-dpda-dot-graph."),
    CommandLineOption(
        'S',
        "generate-dpda-states-file",
        &Options::GenerateDpdaStatesFile,
        "    Output a human-readable specification of the DPDA state machine to the file\n"
        "    specified.  Useful in debugging and resolving conflicts in the grammar.\n"
        "    Specifying - (hyphen) as the filename indicates that the output should be\n"
        "    to stdout.  See also option -s."),
    CommandLineOption(
        's',
        "dont-generate-dpda-states-file",
        &Options::DontGenerateDpdaStatesFile,
        "    Do not output a human-readable specification of the DPDA state machine text\n"
        "    file.  This is the default behavior.  See also option -S."),

/* TODO: implement
    CommandLineOption("Target-related options"),
    CommandLineOption(
        'd',
        "predefine",
        &OptionsBase::Predefine,
        "    Defines a targetspec directive value for a given target before parsing\n"
        "    the primary source.  Directive values specified in the primary source will\n"
        "    override values defined via this commandline option.  The argument is of\n"
        "    the form:\n"
        "        target.<target_id>.<directive_id>=<value>\n"
        "    The <value> portion of the argument must include the delimiters appropriate\n"
        "    to the targetspec directive (e.g. %string directives require a double-quote-\n"
        "    delimited <value>, etc).  See also option -D."),
    CommandLineOption(
        'D',
        "postdefine",
        &OptionsBase::Postdefine,
        "    Defines a targetspec directive value for a given target after parsing\n"
        "    the primary source.  Directive values specified in the primary source will\n"
        "    be overridden by values defined via this commandline option.  See option\n"
        "    -d for details on the form of the argument."),
*/

    CommandLineOption("Verbosity options"),
    CommandLineOption(
        'V',
        "enable-verbosity",
        &OptionsBase::EnableVerbosity,
        "    Enables the specified verbosity option.  Valid parameters are\n"
        "        \"execution\" - Print general application activity to stderr. (not currently implemented)\n" // TODO: implement
        "        \"scanner\" - Print primary source (*.trison) scanner activity to stderr.\n"
        "        \"parser\" - Print primary source (*.trison) parser activity to stderr.\n"
        "        \"ast\" - Print the parsed primary source abstract syntax tree to stderr.\n"
        "        \"targetspec-scanner\" - Print targetspec scanner activity to stderr.\n"
        "        \"targetspec-parser\" - Print targetspec parser activity to stderr.\n"
        "        \"targetspec-ast\" - Print the parsed targetspec abstract syntax tree(s) to stderr.\n"
        "        \"codespec-scanner\" - Print codespec scanner activity to stderr. (not currently implemented)\n" // TODO: implement
        "        \"codespec-parser\" - Print codespec parser activity to stderr.\n"
        "        \"codespec-ast\" - Print the parsed codespec abstract syntax tree(s) to stderr.\n"
        "        \"codespec-symbols\" - Print the preprocessor symbols available to the codespec to stderr.\n"
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
        &Options::SetInputFilename,
        ms_option,
        ms_option_count,
        executable_filename,
        "Trison - A context-free-grammar-parser generator.\n"
        "Part of the BARF compiler tool suite - written by Victor Dods.",
        "[options] <input_filename>")
{ }

void Options::GenerateDpdaStatesFile (string const &dpda_states_filename)
{
    m_dpda_states_filename = dpda_states_filename;
}

void Options::DontGenerateDpdaStatesFile ()
{
    m_dpda_states_filename.clear();
}

void Options::Parse (int const argc, char const *const *const argv)
{
    OptionsBase::Parse(argc, argv);

    // only check for commandline option consistency if no output-and-quit
    // option was specified (e.g. help or print-targets-search-path)
    if (!AbortFlag() &&
        !GetIsHelpRequested() &&
        GetPrintTargetsSearchPathRequest() == PTSPR_NONE)
    {
        if (GetInputFilename().empty())
            ReportErrorAndSetAbortFlag("no input filename specified");
    }
}

} // end of namespace Trison
