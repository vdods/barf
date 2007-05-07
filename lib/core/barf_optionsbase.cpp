// ///////////////////////////////////////////////////////////////////////////
// barf_optionsbase.cpp by Victor Dods, created 2006/10/14
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_optionsbase.hpp"

#include "barf_util.hpp"

namespace Barf {

void OptionsBase::SetInputFilename (string const &input_filename)
{
    if (!m_input_filename.empty())
        cerr << "warning: multiple input filenames specified; "
                "will use the most recent filename" << endl;
    m_input_filename = input_filename;
}

void OptionsBase::TreatWarningsAsErrors ()
{
    m_treat_warnings_as_errors = true;
}

void OptionsBase::DontTreatWarningsAsErrors ()
{
    m_treat_warnings_as_errors = false;
}

void OptionsBase::HaltOnFirstError ()
{
    m_halt_on_first_error = true;
}

void OptionsBase::DontHaltOnFirstError ()
{
    m_halt_on_first_error = false;
}

#if DEBUG
void OptionsBase::AssertOnError ()
{
    m_assert_on_error = true;
}

void OptionsBase::DontAssertOnError ()
{
    m_assert_on_error = false;
}
#endif

void OptionsBase::IncludeTargetsSearchPath (string const &search_path)
{
    AddTargetsSearchPath(search_path, "set by commandline option");
}

void OptionsBase::RequestShortPrintTargetsSearchPath ()
{
    m_print_targets_search_path_request = PTSPR_SHORT;
}

void OptionsBase::RequestVerbosePrintTargetsSearchPath ()
{
    m_print_targets_search_path_request = PTSPR_VERBOSE;
}

void OptionsBase::SetOutputBasename (string const &output_basename)
{
    if (!m_output_directory.empty() || !m_output_filename_base.empty())
        cerr << "warning: multiple output basenames specified; "
                "will use the most recent output basename" << endl;
    m_output_directory = GetDirectoryPortion(output_basename);
    m_output_filename_base = GetFilenamePortion(output_basename);
}

void OptionsBase::SetOutputDir (string const &output_dir)
{
    m_output_dir = output_dir;
    assert(!m_output_dir.empty());
    if (*m_output_dir.rbegin() != DIRECTORY_SLASH_CHAR)
        m_output_dir += DIRECTORY_SLASH_CHAR;
}

void OptionsBase::WithLineDirectives ()
{
    m_with_line_directives = true;
}

void OptionsBase::WithoutLineDirectives ()
{
    m_with_line_directives = false;
}

void OptionsBase::GenerateNaDotGraph (string const &na_dot_graph_path)
{
    m_na_dot_graph_path = na_dot_graph_path;
}

void OptionsBase::DontGenerateNaDotGraph ()
{
    m_na_dot_graph_path.clear();
}

void OptionsBase::GenerateDaDotGraph (string const &da_dot_graph_path)
{
    m_da_dot_graph_path = da_dot_graph_path;
}

void OptionsBase::DontGenerateDaDotGraph ()
{
    m_da_dot_graph_path.clear();
}

void OptionsBase::EnableVerbosity (string const &verbosity_option)
{
    if (verbosity_option == "scanning-spew")
        m_enabled_verbosity |= V_SCANNING_SPEW;
    else if (verbosity_option == "parsing-spew")
        m_enabled_verbosity |= V_PARSING_SPEW;
    else if (verbosity_option == "targetspec-parsing-spew")
        m_enabled_verbosity |= V_TARGET_SPEC_PARSING_SPEW;
    else if (verbosity_option == "preprocessor-parsing-spew")
        m_enabled_verbosity |= V_PREPROCESSOR_PARSING_SPEW;
    else if (verbosity_option == "print-ast")
        m_enabled_verbosity |= V_SYNTAX_TREE;
    else if (verbosity_option == "targetspec-print-ast")
        m_enabled_verbosity |= V_TARGET_SPEC_SYNTAX_TREE;
    else if (verbosity_option == "preprocessor-print-ast")
        m_enabled_verbosity |= V_PREPROCESSOR_SYNTAX_TREE;
    else if (verbosity_option == "all")
        m_enabled_verbosity = V_ALL;
    else
        ReportErrorAndSetAbortFlag("invalid verbosity option argument \"" + verbosity_option + "\"");
}

void OptionsBase::DisableVerbosity (string const &verbosity_option)
{
    if (verbosity_option == "scanning-spew")
        m_enabled_verbosity &= ~V_SCANNING_SPEW;
    else if (verbosity_option == "parsing-spew")
        m_enabled_verbosity &= ~V_PARSING_SPEW;
    else if (verbosity_option == "targetspec-parsing-spew")
        m_enabled_verbosity &= ~V_TARGET_SPEC_PARSING_SPEW;
    else if (verbosity_option == "preprocessor-parsing-spew")
        m_enabled_verbosity &= ~V_PREPROCESSOR_PARSING_SPEW;
    else if (verbosity_option == "print-ast")
        m_enabled_verbosity &= ~V_SYNTAX_TREE;
    else if (verbosity_option == "targetspec-print-ast")
        m_enabled_verbosity &= ~V_TARGET_SPEC_SYNTAX_TREE;
    else if (verbosity_option == "preprocessor-print-ast")
        m_enabled_verbosity &= ~V_PREPROCESSOR_SYNTAX_TREE;
    else if (verbosity_option == "all")
        m_enabled_verbosity = V_NONE;
    else
        ReportErrorAndSetAbortFlag("invalid verbosity option argument \"" + verbosity_option + "\"");
}

void OptionsBase::RequestHelp ()
{
    m_is_help_requested = true;
}

void OptionsBase::Parse (int argc, char const *const *argv)
{
    CommandLineParser::Parse(argc, argv);

    if (m_targets_search_path.GetIsEmpty())
        m_targets_search_path.AddPath(string(".") + DIRECTORY_SLASH_CHAR, "set as default targets search path");
}

void OptionsBase::ReportErrorAndSetAbortFlag (string const &error_message)
{
    assert(!error_message.empty());
    cerr << "error: " << error_message << endl;
    m_abort = true;
}

void OptionsBase::AddTargetsSearchPath (string const &search_path, string const &set_by)
{
    switch (m_targets_search_path.AddPath(search_path, set_by))
    {
        case SearchPath::ADD_PATH_SUCCESS:
            break;

        case SearchPath::ADD_PATH_FAILURE_EMPTY:
            ReportErrorAndSetAbortFlag("empty path (" + set_by + ") can't be added to the targets search path");
            break;

        case SearchPath::ADD_PATH_FAILURE_INVALID:
            ReportErrorAndSetAbortFlag("invalid path " + GetStringLiteral(search_path) + ' ' + set_by);
            break;
    }
}

} // end of namespace Barf
