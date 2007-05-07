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

    template <typename OptionsBaseSubclass>
    OptionsBase (
        void (OptionsBaseSubclass::*non_option_argument_handler_method)(string const &),
        CommandLineOption const *option,
        Uint32 option_count,
        string const &executable_filename,
        string const &program_description,
        string const &usage_message)
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
        m_abort(false)
    {
        // if the BARF_TARGETS_SEARCH_PATH environment variable is set,
        // add it as the lowest-priority targets search path.
        // TODO: config.h-specified path
        char const *search_path = getenv("BARF_TARGETS_SEARCH_PATH");
        if (search_path != NULL)
            AddTargetsSearchPath(search_path, "set by BARF_TARGETS_SEARCH_PATH environment variable");
    }

    // indicates if there were problems with the specified options
    // and that the program should not continue executing.
    inline bool GetAbort () const { return m_abort || !GetParseSucceeded(); }

    // non-option argument accessor
    inline string const &GetInputFilename () const { return m_input_filename; }
    // warning and error option accessors
    inline bool GetTreatWarningsAsErrors () const { return m_treat_warnings_as_errors; }
    inline bool GetHaltOnFirstError () const { return m_halt_on_first_error; }
#if DEBUG
    inline bool GetAssertOnError () const { return m_assert_on_error; }
#endif
    // input options
    inline SearchPath const &GetTargetsSearchPath () const { return m_targets_search_path; }
    inline PrintTargetsSearchPathRequest GetPrintTargetsSearchPathRequest () const { return m_print_targets_search_path_request; }
    // output options
    inline string const &GetOutputDirectory () const { return m_output_directory; } // TODO: deprecate
    inline string const &GetOutputFilenameBase () const { return m_output_filename_base; } // TODO: deprecate
    inline string GetOutputPathBase () const { return m_output_directory + m_output_filename_base; } // TODO: deprecate
    inline string GetOutputDir () const { return m_output_dir; }
    inline bool GetWithLineDirectives () const { return m_with_line_directives; }
    inline string const &GetNaDotGraphPath () const { return m_na_dot_graph_path; }
    inline string const &GetDaDotGraphPath () const { return m_da_dot_graph_path; }
    // verbosity options
    inline bool GetShowScanningSpew () const { return m_enabled_verbosity & V_SCANNING_SPEW; }
    inline bool GetShowParsingSpew () const { return m_enabled_verbosity & V_PARSING_SPEW; }
    inline bool GetShowTargetSpecParsingSpew () const { return m_enabled_verbosity & V_TARGET_SPEC_PARSING_SPEW; }
    inline bool GetShowPreprocessorParsingSpew () const { return m_enabled_verbosity & V_PREPROCESSOR_PARSING_SPEW; }
    inline bool GetShowSyntaxTree () const { return m_enabled_verbosity & V_SYNTAX_TREE; }
    inline bool GetShowTargetSpecSyntaxTree () const { return m_enabled_verbosity & V_TARGET_SPEC_SYNTAX_TREE; }
    inline bool GetShowPreprocessorSyntaxTree () const { return m_enabled_verbosity & V_PREPROCESSOR_SYNTAX_TREE; }
    // help option
    inline bool GetIsHelpRequested () const { return m_is_help_requested; }

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
    void SetOutputBasename (string const &output_basename);
    void SetOutputDir (string const &output_dir);
    void WithLineDirectives ();
    void WithoutLineDirectives ();
    void GenerateNaDotGraph (string const &na_dot_graph_path);
    void DontGenerateNaDotGraph ();
    void GenerateDaDotGraph (string const &da_dot_graph_path);
    void DontGenerateDaDotGraph ();
    // verbosity options
    void EnableVerbosity (string const &verbosity_option);
    void DisableVerbosity (string const &verbosity_option);
    // help option
    void RequestHelp ();

    virtual void Parse (int argc, char const *const *argv);

protected:

    void ReportErrorAndSetAbortFlag (string const &error_message);

    enum
    {
        V_NONE                      = 0x00,

        V_SCANNING_SPEW             = 0x01,
        V_PARSING_SPEW              = 0x02,
        V_TARGET_SPEC_PARSING_SPEW  = 0x04,
        V_PREPROCESSOR_PARSING_SPEW = 0x08,
        V_SYNTAX_TREE               = 0x10,
        V_TARGET_SPEC_SYNTAX_TREE   = 0x20,
        V_PREPROCESSOR_SYNTAX_TREE  = 0x40,

        V_ALL                       = 0x7F
    };

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
    string m_output_filename_base;
    string m_output_dir;
    bool m_with_line_directives;
    string m_na_dot_graph_path;
    string m_da_dot_graph_path;
    // verbosity options
    Uint8 m_enabled_verbosity;
    // help option value
    bool m_is_help_requested;
    // indicates program should abort
    bool m_abort;

private:

    void AddTargetsSearchPath (string const &search_path, string const &set_by);
}; // end of class OptionsBase

} // end of namespace Barf

#endif // !defined(_BARF_OPTIONSBASE_HPP_)
