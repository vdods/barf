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

    // output behavior options
    void SetOutputBasename (string const &output_basename);
    void SetHeaderExtension (string const &header_extension);
    void SetImplementationExtension (string const &implementation_extension);
    void GenerateStateMachineFile (string const &state_machine_filename);
    void DontGenerateStateMachineFile ();

    void Parse (int argc, char const *const *argv);

private:

    // output behavior option values
    string m_output_basename;
    string m_header_extension;
    string m_implementation_extension;
    string m_state_machine_filename;

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

inline Trison::Options &GetOptions ()
{
    assert(g_options != NULL && "g_options has not been initialized");
    return *Dsc<Options *>(g_options);
}

} // end of namespace Trison

#endif // !defined(_TRISON_OPTIONS_HPP_)
