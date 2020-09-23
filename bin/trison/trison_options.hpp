// 2006.02.19 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(TRISON_OPTIONS_HPP_)
#define TRISON_OPTIONS_HPP_

#include "trison.hpp"

#include "barf_optionsbase.hpp"

namespace Trison {

class Options : public OptionsBase
{
public:

    Options (string const &executable_filename);

    // output behavior option accessors
    inline string NpdaStatesPath () const { return m_npda_states_filename.empty() ? g_empty_string : OutputDirectory() + m_npda_states_filename; }
    inline string DpdaStatesPath () const { return m_dpda_states_filename.empty() ? g_empty_string : OutputDirectory() + m_dpda_states_filename; }

    // output behavior options
    void GenerateNpdaStatesFile (string const &npda_states_filename);
    void DontGenerateNpdaStatesFile ();

    void GenerateDpdaStatesFile (string const &dpda_states_filename);
    void DontGenerateDpdaStatesFile ();

    virtual void Parse (int argc, char const *const *argv);

private:

    // output behavior option values
    string m_npda_states_filename;
    string m_dpda_states_filename;

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

} // end of namespace Trison

inline Trison::Options const &TrisonOptions () { return *Dsc<Trison::Options const *>(&GetOptions()); }

#endif // !defined(TRISON_OPTIONS_HPP_)
