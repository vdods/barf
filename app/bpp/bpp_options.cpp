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
        'I',
        "include-search-path",
        &OptionsBase::IncludeSearchPath,
        "    Adds another entry to the include search path, with higher search priority\n"
        "    than the existing entries.  If no valid directories are specified, the search\n"
        "    path will default to the current working directory."),
    CommandLineOption(
        'p',
        "print-search-path",
        &OptionsBase::RequestShortPrintSearchPath,
        "    Prints to stdout, from highest priority to lowest, the include search path\n"
        "    once all -I options have been processed, then exits.  Each path entry is\n"
        "    delimited by a newline."),
    CommandLineOption(
        'P',
        "print-verbose-search-path",
        &OptionsBase::RequestVerbosePrintSearchPath,
        "    Prints to stdout, from highest priority to lowest, the include search path\n"
        "    once all -I options have been processed, then exits.  Each path entry is\n"
        "    followed by a description of how each was added.  Each entry is delimited\n"
        "    by a newline.  This is identical to option -p except for the description."),


    CommandLineOption("Output behavior options"),
    CommandLineOption(
        'o',
        "output-filename",
        &Options::SetOutputFilename,
        "    Optionally specifies a file path to write output to.  Default behavior\n"
        "    is to write to stdout.  Specifying \"-\" will also send output to stdout\n"
        "    Warning and error messages are always to stderr."),

/* TODO implement
    CommandLineOption("Macro-related options"),
    CommandLineOption(
        'D',
        "define",
        &OptionsBase::AddPredefine,
        "    Defines a macro value before parsing the input file.  Macro values\n"
        "    specified in the input file will override values defined via this\n"
        "    commandline option.  The argument is of the form:\n"
        "        <macro_id>=<value>"),
*/

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
