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
        
        
    CommandLineOption("Output behavior options"),
    CommandLineOption(
        'o',
        "output-basename",
        &Options::SetOutputBasename,
        "    REQUIRED -- Specifies the name prefix of the output files, to which the\n"
        "    header, implementation and grammar filename extensions will be appended."),
    CommandLineOption(
        'H',
        "header-ext",
        &Options::SetHeaderExtension,
        "    Specifies the extension for the generated parser class header file.\n"
        "    The default value is \"h\"."),
    CommandLineOption(
        'I',
        "impl-ext",
        &Options::SetImplementationExtension,
        "    Specifies the filename for the generated parser class implementation file.\n"
        "    The default value is \"cpp\"."),
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
    CommandLineOption(
        'L',
        "with-line-directives",
        &OptionsBase::WithLineDirectives,
        "    Use #line directives in the generated source, so that compile errors and\n"
        "    debugging will use the parser source file when appropriate.  This is the\n"
        "    default behavior.  See also option -l."),
    CommandLineOption(
        'l',
        "without-line-directives",
        &OptionsBase::WithoutLineDirectives,
        "    Do not use #line directives in the generated source.  This might be helpful\n"
        "    when the original parser source will not be available during compilation or\n"
        "    debugging.  See also option -L."),
        
        
    CommandLineOption("Verbosity options"),
    CommandLineOption(
        'V',
        "enable-verbosity",
        &OptionsBase::EnableVerbosity,
        "    Enables the specified verbosity option.  Valid parameters are\n"
        "        \"parser\" - Show parser activity debug spew.\n"
        "        \"ast\" - Show the parsed abstract syntax tree.\n"
        "        \"all\" - Enable all verbosity options.\n"
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
        "Trison - A LALR(1) grammar parser generator.\n"
        "Part of the BARF compiler tool suite - written by Victor Dods.",
        "[options] --output-basename=<basename> <input_filename>",
        V_PRIMARY_SOURCE_PARSER|V_PRIMARY_SOURCE_AST)
{
    m_with_line_directives = true;
}

string Options::GetHeaderFilename () const
{
    if (m_output_basename.empty())
        return "";
    else if (m_header_extension.empty())
        return m_output_basename + ".hpp";
    else
        return m_output_basename + "." + m_header_extension;
}

string Options::GetImplementationFilename () const
{
    if (m_output_basename.empty())
        return "";
    else if (m_implementation_extension.empty())
        return m_output_basename + ".cpp";
    else
        return m_output_basename + "." + m_implementation_extension;
}

void Options::SetOutputBasename (string const &output_basename)
{
    assert(!output_basename.empty());
    if (!m_output_basename.empty())
        cerr << "warning: multiple output basenames specified; "
                "will use the most recent output basename" << endl;
    m_output_basename = output_basename;
}

void Options::SetHeaderExtension (string const &header_extension)
{
    assert(!header_extension.empty());
    if (!m_header_extension.empty())
        cerr << "warning: multiple header extensions specified; "
                "will use the most recent header extension" << endl;
    m_header_extension = header_extension;
}

void Options::SetImplementationExtension (string const &implementation_extension)
{
    assert(!implementation_extension.empty());
    if (!m_implementation_extension.empty())
        cerr << "warning: multiple implementation extensions specified; "
                "will use the most recent implementation extension" << endl;
    m_implementation_extension = implementation_extension;
}

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

    if (!GetIsHelpRequested())
    {
        if (GetInputFilename().empty())
            ReportErrorAndSetAbortFlag("no input filename specified");
    }
}

} // end of namespace Trison
