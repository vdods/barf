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

#include <sstream>

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

// ///////////////////////////////////////////////////////////////////////////
// NPDA transitions
// ///////////////////////////////////////////////////////////////////////////

Graph::Transition NpdaReduceTransition (Uint32 reduction_rule_index)
{
    Graph::Transition transition(TT_REDUCE, 1, Graph::Transition::ms_no_target_index, FORMAT("REDUCE: rule " << reduction_rule_index), Graph::Color::ms_green);
    transition.SetData(0, reduction_rule_index);
    return transition;
}

Graph::Transition NpdaReturnTransition (string const &nonterminal_name, Uint32 nonterminal_token_index)
{
    Graph::Transition transition(TT_RETURN, 1, Graph::Transition::ms_no_target_index, "RETURN: " + nonterminal_name, Graph::Color::ms_blue);
    transition.SetData(0, nonterminal_token_index);
    return transition;
}

Graph::Transition NpdaShiftTransition (Uint32 transition_token_id, string const &token_label, Uint32 target_index)
{
    Graph::Transition transition(TT_SHIFT, 1, target_index, token_label);
    transition.SetData(0, transition_token_id);
    return transition;
}

Graph::Transition NpdaEpsilonTransition (Uint32 target_index)
{
    return Graph::Transition(TT_EPSILON, 0, target_index, "(e)", Graph::Color::ms_red);
}

// ///////////////////////////////////////////////////////////////////////////
// DPDA transitions
// ///////////////////////////////////////////////////////////////////////////

Graph::Transition DpdaReduceTransition (Uint32 reduction_rule_index, bool is_default_transition)
{
    Graph::Transition transition(TT_REDUCE, 1, Graph::Transition::ms_no_target_index, FORMAT("REDUCE: rule " << reduction_rule_index), Graph::Color::ms_green);
    transition.SetData(0, reduction_rule_index);
    if (is_default_transition)
        transition.SetOrderPriority(Graph::Transition::ORDER_PRIORITY_FIRST);

    return transition;
}

Graph::Transition DpdaReduceTransition (Graph::Transition::DataArray const &lookahead_sequence, string const &lookahead_sequence_string, Uint32 reduction_rule_index, bool is_default_transition)
{
    assert(false && "i think the default transition will preclude this from ever happening.  TODO -- take out sometime");
    Graph::Transition transition(TT_REDUCE, 1+lookahead_sequence.size(), Graph::Transition::ms_no_target_index, FORMAT(lookahead_sequence_string << ": REDUCE: rule " << reduction_rule_index), Graph::Color::ms_green);
    transition.SetData(0, reduction_rule_index);
    for (Uint32 i = 0; i < lookahead_sequence.size(); ++i)
        transition.SetData(i+1, lookahead_sequence[i]);
    if (is_default_transition)
        transition.SetOrderPriority(Graph::Transition::ORDER_PRIORITY_FIRST);
    return transition;
}

Graph::Transition DpdaReturnTransition (string const &nonterminal_name, Uint32 nonterminal_token_index, bool is_default_transition)
{
    Graph::Transition transition(TT_RETURN, 1, Graph::Transition::ms_no_target_index, "RETURN: " + nonterminal_name, Graph::Color::ms_blue);
    transition.SetData(0, nonterminal_token_index);
    if (is_default_transition)
        transition.SetOrderPriority(Graph::Transition::ORDER_PRIORITY_FIRST);
    return transition;
}

Graph::Transition DpdaShiftTransition (Graph::Transition::DataArray const &lookahead_sequence, string const &lookahead_sequence_string, Uint32 target_index)
{
    return Graph::Transition(TT_SHIFT, lookahead_sequence, target_index, lookahead_sequence_string);
}

Graph::Transition DpdaErrorPanicTransition (bool is_default_transition)
{
    Graph::Transition transition(TT_ERROR_PANIC, 0, Graph::Transition::ms_no_target_index, "ERROR PANIC", Graph::Color::ms_red);
    if (is_default_transition)
        transition.SetOrderPriority(Graph::Transition::ORDER_PRIORITY_FIRST);
    return transition;
}

} // end of namespace Trison
