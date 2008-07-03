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
    TT_ERROR_PANIC,

    TT_COUNT
};

string const &GetTransitionTypeString (TransitionType transition_type);

struct ShiftTransition : public Graph::Transition
{
    ShiftTransition (Uint32 transition_token_id, string const &token_label, Uint32 target_index)
        :
        Graph::Transition(TT_SHIFT, 1, target_index, token_label)
    {
        SetData(0, transition_token_id);
    }
    ShiftTransition (DataArray const &data_array, string const &token_label, Uint32 target_index)
        :
        Graph::Transition(TT_SHIFT, data_array, target_index, token_label)
    { }
}; // end of struct ShiftTransition

struct ReduceTransition : public Graph::Transition
{
    ReduceTransition (Uint32 reduction_rule_index, string const &nonterminal_name, string const &text_prefix = g_empty_string)
        :
        Graph::Transition(TT_REDUCE, 1, ms_no_target_index, text_prefix + "reduce " + nonterminal_name, Graph::Color(0x008800))
    {
        SetData(0, reduction_rule_index);
    }
}; // end of struct ReduceTransition

struct ReturnTransition : public Graph::Transition
{
    ReturnTransition (string const &nonterminal_name, Uint32 nonterminal_token_index, string const &text_prefix = g_empty_string)
        :
        Graph::Transition(TT_RETURN, 1, ms_no_target_index, text_prefix + "return " + nonterminal_name, Graph::Color(0x0000FF))
    {
        SetData(0, nonterminal_token_index);
    }
}; // end of struct ReturnTransition

struct EpsilonTransition : public Graph::Transition
{
    EpsilonTransition (Uint32 target_index)
        :
        Graph::Transition(TT_EPSILON, 0, target_index, "(e)", Graph::Color(0xEF280E))
    { }
}; // end of struct EpsilonTransition

struct ErrorPanicTransition : public Graph::Transition
{
    ErrorPanicTransition (string const &text_prefix = g_empty_string)
        :
        Graph::Transition(TT_ERROR_PANIC, 0, ms_no_target_index, text_prefix + "error panic", Graph::Color(0xEF280E))
    { }
}; // end of struct ErrorPanicTransition

} // end of namespace Trison

#endif // !defined(_TRISON_GRAPH_HPP_)
