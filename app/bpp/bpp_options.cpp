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


    CommandLineOption("Output behavior options"),
    CommandLineOption(
        'o',
        "output-filename",
        &Options::SetOutputFilename,
        "    Optionally specifies a file path to write output to.  Default behavior\n"
        "    is to write to stdout.  Specifying \"-\" will also send output to stdout\n"
        "    Warning and error messages are always to stderr."),


    CommandLineOption("Macro-related options"),
    CommandLineOption(
        'D',
        "predefine",
        &OptionsBase::Predefine,
        "    Defines a macro value before parsing the input file.  Macro values\n"
        "    specified in the input file will override values defined via this\n"
        "    commandline option.  The argument is of the form:\n"
        "        <macro_id>=<value>"),


    CommandLineOption("Verbosity options"),
    CommandLineOption(
        'V',
        "enable-verbosity",
        &OptionsBase::EnableVerbosity,
        "    Enables the specified verbosity option.  Valid parameters are\n"
        "        \"execution\" - Print general application activity to stderr.\n"
        "        \"scanner\" - Print scanner activity to stderr.\n"
        "        \"parser\" - Print parser activity to stderr.\n"
        "        \"ast\" - Print the parsed abstract syntax tree to stderr.\n"
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
        "bpp",
        &Options::SetInputFilename,
        ms_option,
        ms_option_count,
        executable_filename,
        "BPP - The Barf Preprocessor.\n"
        "Part of the BARF compiler tool suite - written by Victor Dods.",
        "[options] [input_filename]",
        V_EXECUTION|V_PRIMARY_SOURCE_SCANNER|V_PRIMARY_SOURCE_PARSER|V_PRIMARY_SOURCE_AST)
{ }

void Options::SetOutputFilename (string const &output_filename)
{
    if (!m_output_filename.empty())
        cerr << "warning: multiple output filenames specified; "
                "will use the most recent filename" << endl;
    m_output_filename = output_filename;
}

void Options::Parse (int const argc, char const *const *const argv)
{
    OptionsBase::Parse(argc, argv);
}

} // end of namespace Bpp
