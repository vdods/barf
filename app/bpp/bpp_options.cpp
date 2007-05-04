// ///////////////////////////////////////////////////////////////////////////
// bpp_options.cpp by Victor Dods, created 2006/11/04
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "bpp_options.hpp"

namespace Bpp {

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
    // output behavior options header
    CommandLineOption("Output behavior options"),
    // output behavior options
    CommandLineOption(
        'o',
        "output-filename",
        &Options::SetOutputFilename,
        "    Optionally specifies a file path to write output to.  Default behavior\n"
        "    is to write to stdout.  Specifying \"-\" will also send output to stdout\n"
        "    Warning and error messages are always to stderr."),
    // verbosity options header
    CommandLineOption("Verbosity options"),
    // verbosity options
    CommandLineOption(
        'V',
        "enable-verbosity",
        &Options::EnableVerbosity, // this is NOT &OptionsBase::EnableVerbosity
        "    Enables the specified verbosity option.  Valid parameters are\n"
        "        \"parsing-spew\" - Show parser activity debug spew.\n"
        "        \"print-ast\" - Show the parsed abstract syntax tree.\n"
        "        \"all\" - Enable all verbosity options.\n"
        "    All verbosity options are disabled by default.  See also option -v."),
    CommandLineOption(
        'v',
        "disable-verbosity",
        &Options::DisableVerbosity, // this is NOT &OptionsBase::DisableVerbosity
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
        "BPP - The Barf Preprocessor.\n"
        "Part of the BARF compiler tool suite - written by Victor Dods.",
        "[options] [input_filename]")
{ }

void Options::SetOutputFilename (string const &output_filename)
{
    if (!m_output_filename.empty())
        cerr << "warning: multiple output filenames specified; "
                "will use the most recent filename" << endl;
    m_output_filename = output_filename;
}

void Options::EnableVerbosity (string const &verbosity_option)
{
    if (verbosity_option == "parsing-spew")
        m_enabled_verbosity |= V_PARSING_SPEW;
    else if (verbosity_option == "print-ast")
        m_enabled_verbosity |= V_SYNTAX_TREE;
    else if (verbosity_option == "all")
        m_enabled_verbosity = V_ALL;
    else
        ReportErrorAndSetAbortFlag("invalid verbosity option argument \"" + verbosity_option + "\"");
}

void Options::DisableVerbosity (string const &verbosity_option)
{
    if (verbosity_option == "parsing-spew")
        m_enabled_verbosity &= ~V_PARSING_SPEW;
    else if (verbosity_option == "print-ast")
        m_enabled_verbosity &= ~V_SYNTAX_TREE;
    else if (verbosity_option == "all")
        m_enabled_verbosity = V_NONE;
    else
        ReportErrorAndSetAbortFlag("invalid verbosity option argument \"" + verbosity_option + "\"");
}

void Options::Parse (int const argc, char const *const *const argv)
{
    OptionsBase::Parse(argc, argv);
}

} // end of namespace Bpp
