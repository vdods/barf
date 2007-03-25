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
        Graph::Transition(TT_INPUT_ATOM, input_atom, input_atom, target_index, GetCharacterLiteral(input_atom))
    { }
}; // end of struct InputAtomTransition

struct InputAtomRangeTransition : public Graph::Transition
{
    InputAtomRangeTransition (Uint8 range_lower, Uint8 range_upper, Uint32 target_index)
        :
        Graph::Transition(
            TT_INPUT_ATOM_RANGE,
            range_lower,
            range_upper,
            target_index,
            string("[") + GetCharacterLiteral(m_data_0, false) + "-" + GetCharacterLiteral(m_data_1, false) + "]")
    { }
}; // end of struct InputAtomRangeTransition

struct EpsilonTransition : public Graph::Transition
{
    EpsilonTransition (Uint32 target_index)
        :
        Graph::Transition(TT_EPSILON, 0, 0, target_index, "(e)", 0xEF280E)
    { }
}; // end of struct EpsilonTransition

struct DfaConditionalTransition : public Graph::Transition
{
    DfaConditionalTransition (Uint8 mask, Uint8 flags, Uint32 target_index)
        :
        Graph::Transition(TT_CONDITIONAL, mask, flags, target_index, gs_empty_string, 0x0000FF)
    {
        m_label = GetDfaTransitionString(*this);
    }
}; // end of struct ConditionalTransition

struct NfaConditionalTransition : public Graph::Transition
{
    NfaConditionalTransition (ConditionalType type, Uint32 target_index)
        :
        Graph::Transition(TT_CONDITIONAL, type, type, target_index, gs_empty_string, 0x0000FF)
    {
        m_label = GetNfaTransitionString(*this);
    }
}; // end of struct ConditionalTransition

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(_BARF_REGEX_GRAPH_HPP_)
