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
        Conditional conditional(transition.Data(0), transition.Data(1));
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
        return GetConditionalTypeString(transition.Data(0));
    else
        return GetTransitionTypeString(transition.Type());
}

Graph::Transition InputAtomTransition (Uint8 input_atom, Uint32 target_index)
{
    Graph::Transition transition(TT_INPUT_ATOM, 1, target_index, GetCharLiteral(input_atom));
    transition.SetData(0, input_atom);
    return transition;
}

Graph::Transition InputAtomRangeTransition (Uint8 range_lower, Uint8 range_upper, Uint32 target_index)
{
    Graph::Transition
        transition(
            TT_INPUT_ATOM_RANGE,
            2,
            target_index,
            string("[") + GetCharLiteral(range_lower, false) + "-" + GetCharLiteral(range_upper, false) + "]");
    transition.SetData(0, range_lower);
    transition.SetData(1, range_upper);
    return transition;
}

Graph::Transition EpsilonTransition (Uint32 target_index)
{
    return Graph::Transition(TT_EPSILON, 0, target_index, "(e)", Graph::Color::ms_red);
}

Graph::Transition DfaConditionalTransition (Uint8 mask, Uint8 flags, Uint32 target_index)
{
    Graph::Transition transition(TT_CONDITIONAL, 2, target_index, g_empty_string, Graph::Color::ms_blue);
    transition.SetData(0, mask);
    transition.SetData(1, flags);
    transition.SetLabel(GetDfaTransitionString(transition));
    return transition;
}

Graph::Transition NfaConditionalTransition (ConditionalType type, Uint32 target_index)
{
    Graph::Transition transition(TT_CONDITIONAL, 1, target_index, g_empty_string, Graph::Color::ms_blue);
    transition.SetData(0, type);
    transition.SetLabel(GetNfaTransitionString(transition));
    return transition;
}

} // end of namespace Regex
} // end of namespace Barf
