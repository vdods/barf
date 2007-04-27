// ///////////////////////////////////////////////////////////////////////////
// barf_regex_graph.cpp by Victor Dods, created 2007/03/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_regex_graph.hpp"

namespace Barf {
namespace Regex {

string const &GetTransitionTypeString (TransitionType transition_type)
{
    static string const s_transition_type_string[TT_COUNT] =
    {
        "TT_INPUT_ATOM",
        "TT_INPUT_ATOM_RANGE",
        "TT_CONDITIONAL",
        "TT_EPSILON"
    };
    assert(transition_type < TT_COUNT);
    return s_transition_type_string[transition_type];
}

string GetDfaTransitionString (Graph::Transition const &transition)
{
    if (transition.Type() == TT_CONDITIONAL)
    {
        string retval;
        Conditional conditional(transition.Data0(), transition.Data1());
        while (conditional.m_mask != 0)
        {
            ConditionalType conditional_type = GetConditionalTypeFromConditional(conditional);
            retval += GetConditionalTypeString(conditional_type);
            if (conditional.m_mask != 0)
                retval += '\n';
        }
        return retval;
    }
    else
        return GetTransitionTypeString(transition.Type());
}

string GetNfaTransitionString (Graph::Transition const &transition)
{
    if (transition.Type() == TT_CONDITIONAL)
        return GetConditionalTypeString(transition.Data0());
    else
        return GetTransitionTypeString(transition.Type());
}

} // end of namespace Regex
} // end of namespace Barf
