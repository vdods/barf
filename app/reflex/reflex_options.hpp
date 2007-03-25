// ///////////////////////////////////////////////////////////////////////////
// reflex_options.hpp by Victor Dods, created 2006/10/20
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_REFLEX_OPTIONS_HPP_)
#define _REFLEX_OPTIONS_HPP_

#include "reflex.hpp"

#include "barf_optionsbase.hpp"

extern OptionsBase *g_options;

namespace Reflex {

class Options : public OptionsBase
{
public:

    Options (string const &executable_filename);

    virtual void Parse (int argc, char const *const *argv);

private:

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

} // end of namespace Reflex

inline Reflex::Options *GetOptions ()
{
    return DStaticCast<Reflex::Options *>(g_options);
}

#endif // !defined(_REFLEX_OPTIONS_HPP_)
