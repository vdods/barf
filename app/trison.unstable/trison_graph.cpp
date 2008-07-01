// ///////////////////////////////////////////////////////////////////////////
// trison_graph.cpp by Victor Dods, created 2007/03/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_graph.hpp"

namespace Trison {

string const &GetTransitionTypeString (TransitionType transition_type)
{
    static string const s_transition_type_string[TT_COUNT] =
    {
        "TT_EPSILON",
        "TT_REDUCE",
        "TT_RETURN",
        "TT_SHIFT",
        "TT_ERROR_PANIC"
    };
    assert(transition_type < TT_COUNT);
    return s_transition_type_string[transition_type];
}

} // end of namespace Trison
