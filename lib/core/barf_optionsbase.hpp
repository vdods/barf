// ///////////////////////////////////////////////////////////////////////////
// barf_optionsbase.hpp by Victor Dods, created 2006/10/14
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_OPTIONSBASE_HPP_)
#define _BARF_OPTIONSBASE_HPP_

#include "barf.hpp"

#include <vector>

#include "barf_commandlineparser.hpp"
#include "barf_searchpath.hpp"

namespace Barf {

class OptionsBase;

} // end of namespace Barf

// the application must define in the global namespace the following symbol(s):
extern Barf::OptionsBase *g_options;

namespace Barf {

// this is a baseclass for common options for all BARF apps
class OptionsBase : public CommandLineParser
{
public:

    enum PrintTargetsSearchPathRequest
    {
        PTSPR_NONE = 0,
        PTSPR_SHORT,
        PTSPR_VERBOSE
    }; // end of enum OptionsBase::PrintTargetsSearchPathRequest

    enum Verbosity
    {
        V_NONE                      = 0x0000,

        V_EXECUTION                 = 0x0001,
        V_PRIMARY_SOURCE_SCANNER    = 0x0002,
        V_PRIMARY_SOURCE_PARSER     = 0x0004,
        V_PRIMARY_SOURCE_AST        = 0x0008,
        V_TARGETSPEC_SCANNER        = 0x0010,
        V_TARGETSPEC_PARSER         = 0x0020,
        V_TARGETSPEC_AST            = 0x0040,
        V_CODESPEC_SCANNER          = 0x0080,
        V_CODESPEC_PARSER           = 0x0100,
        V_CODESPEC_AST              = 0x0200,
        V_CODESPEC_SYMBOLS          = 0x0400,
        V_REGEX_SCANNER             = 0x0800,
        V_REGEX_PARSER              = 0x1000,
        V_REGEX_AST                 = 0x2000,

        V_ALL                       = 0x3FFF
    }; // end of enum OptionsBase::Verbosity

    template <typename OptionsBaseSubclass>
    OptionsBase (
        void (OptionsBaseSubclass::*non_option_argument_handler_method)(string const &),
        CommandLineOption const *option,
        Uint32 option_count,
        string const &executable_filename,
        string const &program_description,
        string const &usage_message,
        Uint32 allowed_verbosity = V_ALL)
        :
        CommandLineParser(
            non_option_argument_handler_method,
            option,
            option_count,
            executable_filename,
            program_description,
            usage_message),
        m_treat_warnings_as_errors(false),
        m_halt_on_first_error(false),
    #if DEBUG
        m_assert_on_error(false),
    #endif
        m_print_targets_search_path_request(PTSPR_NONE),
        m_with_line_directives(false),
        m_enabled_verbosity(V_NONE),
        m_is_help_requested(false),
        m_abort(false),
        m_allowed_verbosity(allowed_verbosity)
    {
        assert((m_allowed_verbosity & ~V_ALL) == 0 && "allowed_verbosity contains invalid Verbosity flags");

        // if the BARF_TARGETS_SEARCH_PATH environment variable is set,
        // add it as the lowest-priority targets search path.
        // TODO: add config.h-specified path
        char const *search_path = getenv("BARF_TARGETS_SEARCH_PATH");
        if (search_path != NULL)
            AddTargetsSearchPath(search_path, "set by BARF_TARGETS_SEARCH_PATH environment variable");
    }

    // indicates if there were problems with the specified options
    // and that the program should not continue executing.
    bool GetAbort () const { return m_abort || !GetParseSucceeded(); }

    // non-option argument accessor
    string const &GetInputFilename () const { return m_input_filename; }
    // warning and error option accessors
    bool GetTreatWarningsAsErrors () const { return m_treat_warnings_as_errors; }
    bool GetHaltOnFirstError () const { return m_halt_on_first_error; }
#if DEBUG
    bool GetAssertOnError () const { return m_assert_on_error; }
#endif
    // input options
    SearchPath const &GetTargetsSearchPath () const { return m_targets_search_path; }
    PrintTargetsSearchPathRequest GetPrintTargetsSearchPathRequest () const { return m_print_targets_search_path_request; }
    // output options
    string GetOutputDirectory () const { return m_output_directory; }
    bool GetWithLineDirectives () const { return m_with_line_directives; }
    string GetNaDotGraphPath () const { return GetOutputDirectory() + m_na_dot_graph_filename; }
    string GetDaDotGraphPath () const { return GetOutputDirectory() + m_da_dot_graph_filename; }
    // target-related options
    vector<string>::size_type GetPredefineCount () const { return m_predefine.size(); }
    string const &GetPredefine (vector<string>::size_type index) const { assert(index < m_predefine.size()); return m_predefine[index]; }
    vector<string>::size_type GetPostdefineCount () const { return m_postdefine.size(); }
    string const &GetPostdefine (vector<string>::size_type index) const { assert(index < m_postdefine.size()); return m_postdefine[index]; }
    // verbosity options
    bool GetIsVerbose (Verbosity verbosity) const { assert((verbosity & ~V_ALL) == 0); return (m_enabled_verbosity & verbosity) != 0; }
    // help option
    bool GetIsHelpRequested () const { return m_is_help_requested; }

    // non-option argument handler
    void SetInputFilename (string const &input_filename);
    // warning and error options
    void TreatWarningsAsErrors ();
    void DontTreatWarningsAsErrors ();
    void HaltOnFirstError ();
    void DontHaltOnFirstError ();
#if DEBUG
    void AssertOnError ();
    void DontAssertOnError ();
#endif
    // input options
    void IncludeTargetsSearchPath (string const &search_path);
    void RequestShortPrintTargetsSearchPath ();
    void RequestVerbosePrintTargetsSearchPath ();
    // output options
    void SetOutputDirectory (string const &output_directory);
    void WithLineDirectives ();
    void WithoutLineDirectives ();
    void GenerateNaDotGraph (string const &na_dot_graph_filename);
    void DontGenerateNaDotGraph ();
    void GenerateDaDotGraph (string const &da_dot_graph_filename);
    void DontGenerateDaDotGraph ();
    // target-related options
    void Predefine (string const &arg);
    void Postdefine (string const &arg);
    // verbosity options
    void EnableVerbosity (string const &verbosity_string);
    void DisableVerbosity (string const &verbosity_string);
    // help option
    void RequestHelp ();

    virtual void Parse (int argc, char const *const *argv);

protected:

    void ReportErrorAndSetAbortFlag (string const &error_message);

    // non-option argument value
    string m_input_filename;
    // warning and error option values
    bool m_treat_warnings_as_errors;
    bool m_halt_on_first_error;
#if DEBUG
    bool m_assert_on_error;
#endif
    // input option values
    SearchPath m_targets_search_path;
    PrintTargetsSearchPathRequest m_print_targets_search_path_request;
    // output option values
    string m_output_directory;
    bool m_with_line_directives;
    string m_na_dot_graph_filename;
    string m_da_dot_graph_filename;
    // target-related options
    vector<string> m_predefine;
    vector<string> m_postdefine;
    // verbosity options
    Uint32 m_enabled_verbosity;
    // help option value
    bool m_is_help_requested;
    // indicates program should abort
    bool m_abort;

private:

    void AddTargetsSearchPath (string const &search_path, string const &set_by);
    Verbosity ParseVerbosityString (string const &verbosity_string);

    Uint32 const m_allowed_verbosity;
}; // end of class OptionsBase

} // end of namespace Barf

#endif // !defined(_BARF_OPTIONSBASE_HPP_)
