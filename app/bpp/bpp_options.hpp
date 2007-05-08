// ///////////////////////////////////////////////////////////////////////////
// bpp_options.hpp by Victor Dods, created 2006/11/04
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BPP_OPTIONS_HPP_)
#define _BPP_OPTIONS_HPP_

#include "bpp.hpp"

#include "barf_optionsbase.hpp"

namespace Bpp {

class Options : public OptionsBase
{
public:

    Options (string const &executable_filename);

    string const &GetOutputFilename () const { return m_output_filename; }

    void Parse (int argc, char const *const *argv);

    // output options
    void SetOutputFilename (string const &output_path);

private:

    // output option values
    string m_output_filename;

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

} // end of namespace Bpp

inline Bpp::Options &GetOptions ()
{
    assert(g_options != NULL && "g_options has not been initialized");
    return *Dsc<Bpp::Options *>(g_options);
}

#endif // !defined(_BPP_OPTIONS_HPP_)
