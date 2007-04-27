// ///////////////////////////////////////////////////////////////////////////
// trison_graph.hpp by Victor Dods, created 2007/03/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_GRAPH_HPP_)
#define _TRISON_GRAPH_HPP_

#include "trison.hpp"

#include "barf_graph.hpp"

namespace Trison {

// Barf::TransitionType values
enum
{
    TT_EPSILON = 0,
    TT_REDUCE,
    TT_RETURN,
    TT_SHIFT,

    TT_COUNT
};

string const &GetTransitionTypeString (TransitionType transition_type);

struct ShiftTransition : public Graph::Transition
{
    ShiftTransition (Uint32 input_atom, string const &token_label, Uint32 target_index)
        :
        Graph::Transition(TT_SHIFT, input_atom, input_atom, target_index, token_label)
    { }
}; // end of struct ShiftTransition

struct ReduceTransition : public Graph::Transition
{
    ReduceTransition (Uint32 reduction_rule_index, string const &nonterminal_name)
        :
        Graph::Transition(TT_REDUCE, reduction_rule_index, reduction_rule_index, ms_no_target_index, "reduce: " + nonterminal_name, Graph::Color(0x00AA00))
    { }
}; // end of struct ReduceTransition

struct ReturnTransition : public Graph::Transition
{
    ReturnTransition (string const &nonterminal_name)
        :
        Graph::Transition(TT_RETURN, 0, 0, ms_no_target_index, "return: " + nonterminal_name, Graph::Color(0x0000FF))
    { }
}; // end of struct ReturnTransition

struct EpsilonTransition : public Graph::Transition
{
    EpsilonTransition (Uint32 target_index)
        :
        Graph::Transition(TT_EPSILON, 0, 0, target_index, "(e)", Graph::Color(0xEF280E))
    { }
}; // end of struct EpsilonTransition

} // end of namespace Trison

#endif // !defined(_TRISON_GRAPH_HPP_)
