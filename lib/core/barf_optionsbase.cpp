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

#include "barf_message.hpp"
#include "barf_util.hpp"

namespace Barf {

string const &OptionsBase::HowtoReportError () const
{
    static string const s_howto_report_error("please make a bug report, containing exact reproduction instructions, in the tracker system at http://sourceforge.net/projects/barf/");
    return s_howto_report_error;
}

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
    m_targets_search_path_entry.push_back(TargetsSearchPathEntry(search_path, "set by commandline option", ABORT_ON_FAILURE));
}

void OptionsBase::RequestShortPrintTargetsSearchPath ()
{
    m_print_targets_search_path_request = PTSPR_SHORT;
}

void OptionsBase::RequestVerbosePrintTargetsSearchPath ()
{
    m_print_targets_search_path_request = PTSPR_VERBOSE;
}

void OptionsBase::SetOutputDirectory (string const &output_directory)
{
    m_output_directory = output_directory;
    assert(!m_output_directory.empty());
    if (*m_output_directory.rbegin() != DIRECTORY_SLASH_CHAR)
        m_output_directory += DIRECTORY_SLASH_CHAR;
}

void OptionsBase::WithLineDirectives ()
{
    m_with_line_directives = true;
}

void OptionsBase::WithoutLineDirectives ()
{
    m_with_line_directives = false;
}

void OptionsBase::GenerateNaDotGraph (string const &na_dot_graph_filename)
{
    m_na_dot_graph_filename = na_dot_graph_filename;
}

void OptionsBase::DontGenerateNaDotGraph ()
{
    m_na_dot_graph_filename.clear();
}

void OptionsBase::GenerateDaDotGraph (string const &da_dot_graph_filename)
{
    m_da_dot_graph_filename = da_dot_graph_filename;
}

void OptionsBase::DontGenerateDaDotGraph ()
{
    m_da_dot_graph_filename.clear();
}

void OptionsBase::Predefine (string const &arg)
{
    m_predefine.push_back(arg);
}

void OptionsBase::Postdefine (string const &arg)
{
    m_postdefine.push_back(arg);
}

void OptionsBase::EnableVerbosity (string const &verbosity_string)
{
    Verbosity verbosity = ParseVerbosityString(verbosity_string);
    if (verbosity == V_NONE)
        ReportErrorAndSetAbortFlag("invalid verbosity option argument \"" + verbosity_string + "\"");
    else
        m_enabled_verbosity |= verbosity;
}

void OptionsBase::DisableVerbosity (string const &verbosity_string)
{
    Verbosity verbosity = ParseVerbosityString(verbosity_string);
    if (verbosity == V_NONE)
        ReportErrorAndSetAbortFlag("invalid verbosity option argument \"" + verbosity_string + "\"");
    else
        m_enabled_verbosity &= ~verbosity;
}

void OptionsBase::RequestHelp ()
{
    m_is_help_requested = true;
}

void OptionsBase::Parse (int argc, char const *const *argv)
{
    CommandLineParser::Parse(argc, argv);
}

void OptionsBase::AddDefaultTargetsSearchPathEntries ()
{
#if defined(BARFDATADIR)
    // add "BARFDATADIR/targets" (i.e. the installed targets data)
    // as the lowest-priority targets search path.
    if (!string(BARFDATADIR).empty())
        m_targets_search_path_entry.push_back(TargetsSearchPathEntry(BARFDATADIR DIRECTORY_SLASH_STRING "targets", "location of installed targets directory", IGNORE_FAILURE));
#endif // defined(BARFDATADIR)

    // if the BARF_TARGETS_SEARCH_PATH environment variable is set,
    // add it as the lowest-priority targets search path.
    char const *search_path = getenv("BARF_TARGETS_SEARCH_PATH");
    if (search_path != NULL)
        m_targets_search_path_entry.push_back(TargetsSearchPathEntry(search_path, "set by BARF_TARGETS_SEARCH_PATH environment variable", ABORT_ON_FAILURE));
}

void OptionsBase::ProcessTargetsSearchPath ()
{
    for (vector<TargetsSearchPathEntry>::const_iterator it = m_targets_search_path_entry.begin(),
                                                        it_end = m_targets_search_path_entry.end();
         it != it_end;
         ++it)
    {
        AddTargetsSearchPath(it->m_search_path, it->m_set_by, it->m_ignore_add_failure);
    }

    if (m_targets_search_path.GetIsEmpty())
        m_targets_search_path.AddPath(string(".") + DIRECTORY_SLASH_CHAR, "set as default targets search path");
}

void OptionsBase::ReportErrorAndSetAbortFlag (string const &error_message)
{
    assert(!error_message.empty());
    cerr << "error: " << error_message << endl;
    m_abort_flag = true;
}

void OptionsBase::AddTargetsSearchPath (string const &search_path, string const &set_by, bool ignore_add_failure)
{
    EmitExecutionMessage("attempting to add " + GetStringLiteral(search_path) + " (" + set_by + ") to targets search path");
    
    switch (m_targets_search_path.AddPath(search_path, set_by))
    {
        case SearchPath::ADD_PATH_SUCCESS:
            EmitExecutionMessage("successfully added " + GetStringLiteral(search_path) + " (" + set_by + ") to targets search path");
            break;

        case SearchPath::ADD_PATH_FAILURE_EMPTY:
            if (!ignore_add_failure)
                ReportErrorAndSetAbortFlag("empty path (" + set_by + ") can't be added to the targets search path");
            else
                EmitExecutionMessage("empty path (" + set_by + ") can't be added to the targets search path");
            break;
            
        case SearchPath::ADD_PATH_FAILURE_INVALID:
            if (!ignore_add_failure)
                ReportErrorAndSetAbortFlag("invalid targets search path " + GetStringLiteral(search_path) + " (" + set_by + ')');
            else
                EmitExecutionMessage("invalid targets search path " + GetStringLiteral(search_path) + " (" + set_by + ')');
            break;

        default:
            assert(false && "unhandled case");
            break;
    }
}

OptionsBase::Verbosity OptionsBase::ParseVerbosityString (string const &verbosity_string)
{
    Verbosity verbosity = V_NONE;

    if      (verbosity_string == "execution")            verbosity = V_EXECUTION;
    else if (verbosity_string == "scanner")              verbosity = V_PRIMARY_SOURCE_SCANNER;
    else if (verbosity_string == "parser")               verbosity = V_PRIMARY_SOURCE_PARSER;
    else if (verbosity_string == "ast")                  verbosity = V_PRIMARY_SOURCE_AST;
    else if (verbosity_string == "targetspec-scanner")   verbosity = V_TARGETSPEC_SCANNER;
    else if (verbosity_string == "targetspec-parser")    verbosity = V_TARGETSPEC_PARSER;
    else if (verbosity_string == "targetspec-ast")       verbosity = V_TARGETSPEC_AST;
    else if (verbosity_string == "codespec-scanner")     verbosity = V_CODESPEC_SCANNER;
    else if (verbosity_string == "codespec-parser")      verbosity = V_CODESPEC_PARSER;
    else if (verbosity_string == "codespec-ast")         verbosity = V_CODESPEC_AST;
    else if (verbosity_string == "codespec-symbols")     verbosity = V_CODESPEC_SYMBOLS;
    else if (verbosity_string == "regex-scanner")        verbosity = V_REGEX_SCANNER;
    else if (verbosity_string == "regex-parser")         verbosity = V_REGEX_PARSER;
    else if (verbosity_string == "regex-ast")            verbosity = V_REGEX_AST;
    else if (verbosity_string == "all")                  verbosity = V_ALL;

    return Verbosity(verbosity & m_allowed_verbosity);
}

} // end of namespace Barf
