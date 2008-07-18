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
    inline string GetDpdaStatesPath () const { return m_dpda_states_filename.empty() ? g_empty_string : GetOutputDirectory() + m_dpda_states_filename; }

    // output behavior options
    void GenerateDpdaStatesFile (string const &dpda_states_filename);
    void DontGenerateDpdaStatesFile ();

    virtual void Parse (int argc, char const *const *argv);

private:

    // output behavior option values
    string m_dpda_states_filename;

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

} // end of namespace Trison

inline Trison::Options const &GetTrisonOptions () { return *Dsc<Trison::Options const *>(&GetOptions()); }

#endif // !defined(_TRISON_OPTIONS_HPP_)
