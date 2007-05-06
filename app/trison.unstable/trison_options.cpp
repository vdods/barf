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
    // warning and error options header
    CommandLineOption("Warning and error options"),
    // warning and error options
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
    // input behavior options header
    CommandLineOption("Input behavior options"),
    // input behavior options
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
    // output behavior options header
    CommandLineOption("Output behavior options"),
    // output behavior options
    CommandLineOption(
        'o',
        "output-basename",
        &OptionsBase::SetOutputBasename,
        "    REQUIRED -- Specifies the name prefix of the output files, to which the\n"
        "    header, implementation and grammar filename extensions will be appended."),
    /*
    CommandLineOption(
        'S',
        "generate-states-file",
        &Options::GenerateStateMachineFile,
        "    Output a human-readable state machine description to the file specified.\n"
        "    Useful in debugging and resolving conflicts in the grammar.  Specifying\n"
        "    - (hyphen) as the filename indicates that the output should be to stdout.\n"
        "    See also option -s."),
    CommandLineOption(
        's',
        "dont-generate-states-file",
        &Options::DontGenerateStateMachineFile,
        "    Do not output a human-readable state machine description text file.\n"
        "    This is the default behavior.  See also option -S."),
    */
    /*
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
        "    debugging.  See also option -L."),
    */
    CommandLineOption(
        'N',
        "generate-npda-dot-graph",
        &OptionsBase::GenerateNfaDotGraph,
        "    Specifies the filename for a `dot` graph (see http://www.graphviz.org/)\n"
        "    of the generated nondeterministic pushdown automata (NPDA).  Useful as a\n"
        "    diagnostic tool.  Specifying - (hyphen) indicates that the output should\n"
        "    be to stdout.  See also option -n."),
    CommandLineOption(
        'n',
        "dont-generate-npda-dot-graph",
        &OptionsBase::DontGenerateNaDotGraph,
        "    Do not create a `dot` graph file for the NPDA.  This is the default\n"
        "    behavior.  See also option -N."),
    CommandLineOption(
        'D',
        "generate-dpda-dot-graph",
        &OptionsBase::GenerateDfaDotGraph,
        "    Specifies the filename for a `dot` graph (see http://www.graphviz.org/) of\n"
        "    the generated deterministic pushdown automata (DPDA; which is generated\n"
        "    from the NFA).  Useful as a diagnostic tool.  Specifying - (hyphen)\n"
        "    indicates that the output should be to stdout.  See also option -d."),
    CommandLineOption(
        'd',
        "dont-generate-dpda-dot-graph",
        &OptionsBase::DontGenerateDaDotGraph,
        "    Do not create a `dot` graph file for the DPDA.  This is the default\n"
        "    behavior.  See also option -D."),
    // verbosity options header
    CommandLineOption("Verbosity options"),
    // verbosity options
    CommandLineOption(
        'V',
        "enable-verbosity",
        &OptionsBase::EnableVerbosity,
        "    Enables the specified verbosity option.  Valid parameters are\n"
        "        \"scanning-spew\" - Show scanner activity debug spew.\n"
        "        \"parsing-spew\" - Show parser activity debug spew.\n"
        "        \"targetspec-parsing-spew\" - Show targetspec parser activity debug spew.\n"
        "        \"preprocessor-parsing-spew\" - Show preprocessor parser activity debug spew.\n"
        "        \"print-ast\" - Show the parsed abstract syntax tree.\n"
        "        \"targetspec-print-ast\" - Show the parsed targetspec abstract syntax tree.\n"
        "        \"preprocessor-print-ast\" - Show the parsed preprocessor abstract syntax tree.\n"
        "        \"all\" - Enable all verbosity options.\n"
        "    All verbosity options are disabled by default.  See also option -v."),
    CommandLineOption(
        'v',
        "disable-verbosity",
        &OptionsBase::DisableVerbosity,
        "    Disables the specified verbosity option.  See option -V for\n"
        "    valid parameters and their descriptions."),
    // just a space before the help option
    CommandLineOption(""),
    // help option
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

void Options::GenerateStateMachineFile (string const &state_machine_filename)
{
    m_state_machine_filename = state_machine_filename;
}

void Options::DontGenerateStateMachineFile ()
{
    m_state_machine_filename.clear();
}

void Options::Parse (int const argc, char const *const *const argv)
{
    OptionsBase::Parse(argc, argv);

    // only check for commandline option consistency if no output-and-quit
    // option was specified (e.g. help or print-targets-search-path)
    if (!m_abort &&
        !GetIsHelpRequested() &&
        GetPrintTargetsSearchPathRequest() == PTSPR_NONE)
    {
        if (GetInputFilename().empty())
            ReportErrorAndSetAbortFlag("no input filename specified");
    }
}

} // end of namespace Trison
