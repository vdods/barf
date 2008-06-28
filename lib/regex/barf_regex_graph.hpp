// ///////////////////////////////////////////////////////////////////////////
// barf_regex_graph.hpp by Victor Dods, created 2007/03/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_REGEX_GRAPH_HPP_)
#define _BARF_REGEX_GRAPH_HPP_

#include "barf_regex.hpp"

#include "barf_graph.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Regex {

// Barf::TransitionType values
enum
{
    TT_INPUT_ATOM = 0,
    TT_INPUT_ATOM_RANGE,
    TT_CONDITIONAL,
    TT_EPSILON,

    TT_COUNT
};

string const &GetTransitionTypeString (TransitionType transition_type);

string GetDfaTransitionString (Graph::Transition const &transition);
string GetNfaTransitionString (Graph::Transition const &transition);

struct InputAtomTransition : public Graph::Transition
{
    InputAtomTransition (Uint8 input_atom, Uint32 target_index)
        :
        Graph::Transition(TT_INPUT_ATOM, 1, target_index, GetCharLiteral(input_atom))
    {
        SetData(0, input_atom);
    }
}; // end of struct InputAtomTransition

struct InputAtomRangeTransition : public Graph::Transition
{
    InputAtomRangeTransition (Uint8 range_lower, Uint8 range_upper, Uint32 target_index)
        :
        Graph::Transition(
            TT_INPUT_ATOM_RANGE,
            2,
            target_index,
            string("[") + GetCharLiteral(range_lower, false) + "-" + GetCharLiteral(range_upper, false) + "]")
    {
        SetData(0, range_lower);
        SetData(1, range_upper);
    }
}; // end of struct InputAtomRangeTransition

struct EpsilonTransition : public Graph::Transition
{
    EpsilonTransition (Uint32 target_index)
        :
        Graph::Transition(TT_EPSILON, 0, target_index, "(e)", Graph::Color(0xEF280E))
    { }
}; // end of struct EpsilonTransition

struct DfaConditionalTransition : public Graph::Transition
{
    DfaConditionalTransition (Uint8 mask, Uint8 flags, Uint32 target_index)
        :
        Graph::Transition(TT_CONDITIONAL, 2, target_index, g_empty_string, Graph::Color(0x0000FF))
    {
        SetData(0, mask);
        SetData(1, flags);
        SetLabel(GetDfaTransitionString(*this));
    }
}; // end of struct ConditionalTransition

struct NfaConditionalTransition : public Graph::Transition
{
    NfaConditionalTransition (ConditionalType type, Uint32 target_index)
        :
        Graph::Transition(TT_CONDITIONAL, 1, target_index, g_empty_string, Graph::Color(0x0000FF))
    {
        SetData(0, type);
        SetLabel(GetNfaTransitionString(*this));
    }
}; // end of struct ConditionalTransition

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(_BARF_REGEX_GRAPH_HPP_)
