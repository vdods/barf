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

string const &TransitionTypeString (TransitionType transition_type)
{
    static string const s_transition_type_string[TT_COUNT] =
    {
        "ERROR_PANIC",
        "RETURN",
        "REDUCE",
        "SHIFT",
        "INSERT_LOOKAHEAD_ERROR",
        "DISCARD_LOOKAHEAD",
        "POP_STACK",
        "EPSILON"
    };
    assert(transition_type < TT_COUNT);
    return s_transition_type_string[transition_type];
}

// ///////////////////////////////////////////////////////////////////////////
// NPDA transitions
// ///////////////////////////////////////////////////////////////////////////

Graph::Transition NpdaReduceTransition (Uint32 transition_token_id, string const &transition_label, Uint32 reduction_rule_index)
{
    Graph::Transition transition(TT_REDUCE, 2, Graph::Transition::ms_no_target_index, FORMAT(transition_label << ":REDUCE rule " << reduction_rule_index), Graph::Color::ms_blue);
    transition.SetData(0, transition_token_id);
    transition.SetData(1, reduction_rule_index);
    return transition;
}

Graph::Transition NpdaReturnTransition (Uint32 transition_token_id, string const &transition_label)
{
    Graph::Transition transition(TT_RETURN, 2, Graph::Transition::ms_no_target_index, transition_label + ":RETURN", Graph::Color::ms_blue);
    transition.SetData(0, transition_token_id);
    transition.SetData(1, Uint32(-1)); // unused
    return transition;
}

Graph::Transition NpdaShiftTransition (Uint32 transition_token_id, string const &transition_label, Uint32 target_index)
{
    Graph::Transition transition(TT_SHIFT, 2, target_index, FORMAT(transition_label << ":SHIFT then push state " << target_index));
    transition.SetData(0, transition_token_id);
    transition.SetData(1, target_index);
    return transition;
}

Graph::Transition NpdaInsertLookaheadErrorTransition (Uint32 transition_token_id, string const &transition_label)
{
    Graph::Transition transition(TT_INSERT_LOOKAHEAD_ERROR, 2, Graph::Transition::ms_no_target_index, transition_label + ":INSERT_LOOKAHEAD_ERROR", Graph::Color::ms_orange);
    transition.SetData(0, transition_token_id);
    transition.SetData(1, Uint32(-1)); // unused
    return transition;
}

Graph::Transition NpdaDiscardLookaheadTransition (Uint32 transition_token_id, string const &transition_label)
{
    Graph::Transition transition(TT_DISCARD_LOOKAHEAD, 2, Graph::Transition::ms_no_target_index, transition_label + ":DISCARD_LOOKAHEAD", Graph::Color::ms_orange);
    transition.SetData(0, transition_token_id);
    transition.SetData(1, Uint32(-1)); // unused
    return transition;
}

Graph::Transition NpdaPopStackTransition (Uint32 transition_token_id, string const &transition_label, Uint32 pop_count)
{
    Graph::Transition transition(TT_POP_STACK, 2, Graph::Transition::ms_no_target_index, FORMAT(transition_label << ":POP_STACK " << pop_count), Graph::Color::ms_red);
    transition.SetData(0, transition_token_id);
    transition.SetData(1, pop_count);
    return transition;
}

Graph::Transition NpdaEpsilonTransition (Uint32 target_index)
{
    Graph::Transition transition(TT_EPSILON, 2, target_index, FORMAT("(e):go to state " << target_index), Graph::Color::ms_green);
    transition.SetData(0, 0); // default
    transition.SetData(1, target_index);
    return transition;
}

// ///////////////////////////////////////////////////////////////////////////
// DPDA transitions
// ///////////////////////////////////////////////////////////////////////////

Graph::Transition DpdaReduceTransition (Uint32 reduction_rule_index)
{
    Graph::Transition transition(TT_REDUCE, 1, Graph::Transition::ms_no_target_index, FORMAT("REDUCE rule " << reduction_rule_index), Graph::Color::ms_green);
    transition.SetData(0, reduction_rule_index);
    return transition;
}

Graph::Transition DpdaReturnTransition (string const &nonterminal_name, Uint32 nonterminal_token_index)
{
    Graph::Transition transition(TT_RETURN, 1, Graph::Transition::ms_no_target_index, "RETURN " + nonterminal_name, Graph::Color::ms_blue);
    transition.SetData(0, nonterminal_token_index);
    return transition;
}

Graph::Transition DpdaShiftTransition (Graph::Transition::DataArray const &lookahead_sequence, string const &lookahead_sequence_string, Uint32 target_index)
{
    return Graph::Transition(TT_SHIFT, lookahead_sequence, target_index, lookahead_sequence_string);
}

Graph::Transition DpdaErrorPanicTransition ()
{
    return Graph::Transition(TT_ERROR_PANIC, 0, Graph::Transition::ms_no_target_index, "ERROR PANIC", Graph::Color::ms_red);
}

} // end of namespace Trison
