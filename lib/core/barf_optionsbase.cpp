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

void OptionsBase::IncludeDataPath (string const &data_path)
{
    m_search_path.AddPath(data_path, "set by commandline option");
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

void OptionsBase::GenerateNfaDotGraph (string const &nfa_dot_graph_path)
{
    m_na_dot_graph_path = nfa_dot_graph_path;
}

void OptionsBase::DontGenerateNaDotGraph ()
{
    m_na_dot_graph_path.clear();
}

void OptionsBase::GenerateDfaDotGraph (string const &dfa_dot_graph_path)
{
    m_da_dot_graph_path = dfa_dot_graph_path;
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
    else if (verbosity_option == "langspec-parsing-spew")
        m_enabled_verbosity |= V_LANG_SPEC_PARSING_SPEW;
    else if (verbosity_option == "preprocessor-parsing-spew")
        m_enabled_verbosity |= V_PREPROCESSOR_PARSING_SPEW;
    else if (verbosity_option == "print-ast")
        m_enabled_verbosity |= V_SYNTAX_TREE;
    else if (verbosity_option == "langspec-print-ast")
        m_enabled_verbosity |= V_LANG_SPEC_SYNTAX_TREE;
    else if (verbosity_option == "preprocessor-print-ast")
        m_enabled_verbosity |= V_PREPROCESSOR_SYNTAX_TREE;
    else if (verbosity_option == "all")
        m_enabled_verbosity = V_ALL;
    else
    {
        cerr << "error: invalid verbosity option argument \"" << verbosity_option << "\"" << endl;
        m_abort = true;
    }
}

void OptionsBase::DisableVerbosity (string const &verbosity_option)
{
    if (verbosity_option == "scanning-spew")
        m_enabled_verbosity &= ~V_SCANNING_SPEW;
    else if (verbosity_option == "parsing-spew")
        m_enabled_verbosity &= ~V_PARSING_SPEW;
    else if (verbosity_option == "langspec-parsing-spew")
        m_enabled_verbosity &= ~V_LANG_SPEC_PARSING_SPEW;
    else if (verbosity_option == "preprocessor-parsing-spew")
        m_enabled_verbosity &= ~V_PREPROCESSOR_PARSING_SPEW;
    else if (verbosity_option == "print-ast")
        m_enabled_verbosity &= ~V_SYNTAX_TREE;
    else if (verbosity_option == "langspec-print-ast")
        m_enabled_verbosity &= ~V_LANG_SPEC_SYNTAX_TREE;
    else if (verbosity_option == "preprocessor-print-ast")
        m_enabled_verbosity &= ~V_PREPROCESSOR_SYNTAX_TREE;
    else if (verbosity_option == "all")
        m_enabled_verbosity = V_NONE;
    else
    {
        cerr << "error: invalid verbosity option argument \"" << verbosity_option << "\"" << endl;
        m_abort = true;
    }
}

void OptionsBase::RequestHelp ()
{
    m_is_help_requested = true;
}

void OptionsBase::UnrequestHelp ()
{
    m_is_help_requested = false;
}

void OptionsBase::Parse (int argc, char const *const *argv)
{
    CommandLineParser::Parse(argc, argv);

    if (m_search_path.GetIsEmpty())
        m_search_path.AddPath(string(".") + DIRECTORY_SLASH_CHAR, "set as default data path");
}

} // end of namespace Barf
