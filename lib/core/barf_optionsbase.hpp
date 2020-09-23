// 2006.10.14 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_OPTIONSBASE_HPP_)
#define BARF_OPTIONSBASE_HPP_

#include "barf.hpp"

#include <vector>

#include "barf_commandlineparser.hpp"
#include "barf_searchpath.hpp"

namespace Barf {

class OptionsBase;

} // end of namespace Barf

// the application must define in the global namespace the following functions:
extern bool OptionsAreInitialized ();
extern Barf::OptionsBase const &GetOptions ();

namespace Barf {

// this is a baseclass for common options for all BARF apps
class OptionsBase : public CommandLineParser
{
public:

    enum PrintSearchPathRequest
    {
        PSPR_NONE = 0,
        PSPR_SHORT,
        PSPR_VERBOSE
    }; // end of enum OptionsBase::PrintSearchPathRequest

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
        string const &program_name,
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
        m_program_name(program_name),
        m_treat_warnings_as_errors(false),
        m_halt_on_first_error(false),
    #if DEBUG
        m_assert_on_error(false),
    #endif
        m_print_search_path_request(PSPR_NONE),
        m_with_line_directives(true),
        m_line_directives_relative_to_path(""), // empty indicates paths are not relative
        m_enabled_verbosity(V_NONE),
        m_is_help_requested(false),
        m_abort_flag(false),
        m_allowed_verbosity(allowed_verbosity)
    {
        assert(!m_program_name.empty());
        assert((m_allowed_verbosity & ~V_ALL) == 0 && "allowed_verbosity contains invalid Verbosity flags");
    }

    // returns the name of this program, e.g. "reflex", "trison", "bpp"
    string const &ProgramName () const { return m_program_name; }
    // indicates if there were problems with the specified options
    // and that the program should not continue executing.
    bool Abort () const { return m_abort_flag || !ParseSucceeded(); }

    // returns a string containing instructions on how to report an error
    // in the application.
    string const &HowtoReportError () const;

    // non-option argument accessor
    string const &InputFilename () const { return m_input_filename; }
    // warning and error option accessors
    bool TreatWarningsAsErrors () const { return m_treat_warnings_as_errors; }
    bool HaltOnFirstError () const { return m_halt_on_first_error; }
#if DEBUG
    bool AssertOnError () const { return m_assert_on_error; }
#endif
    // input options
    SearchPath const &GetSearchPath () const { return m_search_path; }
    PrintSearchPathRequest GetPrintSearchPathRequest () const { return m_print_search_path_request; }
    // output options
    string const &OutputDirectory () const { return m_output_directory; }
    bool WithLineDirectives () const { return m_with_line_directives; }
    string const &LineDirectivesRelativeToPath () const { return m_line_directives_relative_to_path; }
    string NaDotGraphPath () const { return m_na_dot_graph_filename.empty() ? g_empty_string : OutputDirectory() + m_na_dot_graph_filename; }
    string DaDotGraphPath () const { return m_da_dot_graph_filename.empty() ? g_empty_string : OutputDirectory() + m_da_dot_graph_filename; }
    // target-related options
    vector<string>::size_type PredefineCount () const { return m_predefine.size(); }
    string const &Predefine (vector<string>::size_type index) const { assert(index < m_predefine.size()); return m_predefine[index]; }
    vector<string>::size_type PostdefineCount () const { return m_postdefine.size(); }
    string const &Postdefine (vector<string>::size_type index) const { assert(index < m_postdefine.size()); return m_postdefine[index]; }
    // verbosity options
    bool IsVerbose (Verbosity verbosity) const { assert((verbosity & ~V_ALL) == 0); return (m_enabled_verbosity & verbosity) != 0; }
    // help option
    bool IsHelpRequested () const { return m_is_help_requested; }

    // non-option argument handler
    void SetInputFilename (string const &input_filename);
    // warning and error options
    void TreatWarningsAsErrors_Enable ();
    void TreatWarningsAsErrors_Disable ();
    void HaltOnFirstError_Enable ();
    void HaltOnFirstError_Disable ();
#if DEBUG
    void AssertOnError_Enable ();
    void AssertOnError_Disable ();
#endif
    // input options
    void ClearSearchPath ();
    void IncludeSearchPath (string const &search_path);
    void RequestShortPrintSearchPath ();
    void RequestVerbosePrintSearchPath ();
    // output options
    void SetOutputDirectory (string const &output_directory);
    void WithLineDirectives_Enable ();
    void WithLineDirectives_Disable ();
    void SetLineDirectivesRelativeToPath (string const &path);
    void GenerateNaDotGraph (string const &na_dot_graph_filename);
    void GenerateNaDotGraph_Disable ();
    void GenerateDaDotGraph (string const &da_dot_graph_filename);
    void GenerateDaDotGraph_Disable ();
    // target-related options
    void AddPredefine (string const &arg);
    void AddPostdefine (string const &arg);
    // verbosity options
    void EnableVerbosity (string const &verbosity_string);
    void DisableVerbosity (string const &verbosity_string);
    // help option
    void RequestHelp ();

    virtual void Parse (int argc, char const *const *argv);

    void AddDefaultSearchPathEntries ();
    void ProcessSearchPath ();

protected:

    bool AbortFlag () const { return m_abort_flag; }
    void ReportErrorAndSetAbortFlag (string const &error_message);

private:

    struct SearchPathEntry
    {
        string m_search_path;
        string m_set_by;
        bool m_ignore_add_failure;

        SearchPathEntry (string const &search_path, string const &set_by, bool ignore_add_failure)
            :
            m_search_path(search_path),
            m_set_by(set_by),
            m_ignore_add_failure(ignore_add_failure)
        { }
    }; // end of struct OptionsBase::SearchPathEntry
    
    enum { IGNORE_FAILURE = true, ABORT_ON_FAILURE = false };
    void AddSearchPath (string const &search_path, string const &set_by, bool ignore_add_failure);
    Verbosity ParseVerbosityString (string const &verbosity_string);

    // the name of this program, e.g. "reflex"
    string const m_program_name;
    // non-option argument value
    string m_input_filename;
    // warning and error option values
    bool m_treat_warnings_as_errors;
    bool m_halt_on_first_error;
#if DEBUG
    bool m_assert_on_error;
#endif
    // input option values
    SearchPath m_search_path;
    PrintSearchPathRequest m_print_search_path_request;
    // output option values
    string m_output_directory;
    bool m_with_line_directives;
    string m_line_directives_relative_to_path;
    string m_na_dot_graph_filename;
    string m_da_dot_graph_filename;
    // targets search path options
    vector<SearchPathEntry> m_search_path_entry;
    // target-related options
    vector<string> m_predefine;
    vector<string> m_postdefine;
    // verbosity options
    Uint32 m_enabled_verbosity;
    // help option value
    bool m_is_help_requested;
    // indicates program should abort
    bool m_abort_flag;
    // stores the verbosity flags (e.g. show parser debug spew, etc)
    Uint32 const m_allowed_verbosity;
}; // end of class OptionsBase

} // end of namespace Barf

#endif // !defined(BARF_OPTIONSBASE_HPP_)
