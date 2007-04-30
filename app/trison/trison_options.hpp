// ///////////////////////////////////////////////////////////////////////////
// trison_options.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_OPTIONS_HPP_)
#define _TRISON_OPTIONS_HPP_

#include "trison.hpp"

#include "barf_optionsbase.hpp"

extern OptionsBase *g_options;

namespace Trison {

class Options : public OptionsBase
{
public:

    Options (string const &executable_filename);

    // output behavior option accessors
    inline bool GetIsOutputBasenameSpecified () const { return !m_output_basename.empty(); }
    string GetHeaderFilename () const;
    string GetImplementationFilename () const;
    inline string GetStateMachineFilename () const { return m_state_machine_filename; }
    // verbosity options
    inline bool GetShowParsingSpew () const { return m_enabled_verbosity & V_PARSING_SPEW; }
    inline bool GetShowSyntaxTree () const { return m_enabled_verbosity & V_SYNTAX_TREE; }

    // output behavior options
    void SetOutputBasename (string const &output_basename);
    void SetHeaderExtension (string const &header_extension);
    void SetImplementationExtension (string const &implementation_extension);
    void GenerateStateMachineFile (string const &state_machine_filename);
    void DontGenerateStateMachineFile ();
    // verbosity options
    void EnableVerbosity (string const &verbosity_option);
    void DisableVerbosity (string const &verbosity_option);

    void Parse (int argc, char const *const *argv);

private:

    enum
    {
        V_NONE          = 0x00,

        V_PARSING_SPEW  = 0x01,
        V_SYNTAX_TREE   = 0x02,

        V_ALL           = 0x03
    };

    // output behavior option values
    string m_output_basename;
    string m_header_extension;
    string m_implementation_extension;
    string m_state_machine_filename;

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

inline Options *GetOptions ()
{
    return Dsc<Options *>(g_options);
}

} // end of namespace Trison

#endif // !defined(_TRISON_OPTIONS_HPP_)
