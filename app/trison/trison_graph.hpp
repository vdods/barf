// ///////////////////////////////////////////////////////////////////////////
// trison_graph.hpp by Victor Dods, created 2007/03/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(TRISON_GRAPH_HPP_)
#define TRISON_GRAPH_HPP_

#include "trison.hpp"

#include "barf_graph.hpp"

namespace Trison {

// Barf::TransitionType values.  The order of these is critical to the order
// in Barf::Graph::Transition::Order.  We require that TT_ERROR_PANIC,
// TT_RETURN, and TT_REDUCE come before TT_SHIFT, so that they appear before
// TT_SHIFT, because the default transition must come first (and a transition
// is TT_ERROR_PANIC, TT_RETURN, or TT_REDUCE if and only if it is a default
// transition).
enum
{
    TT_ERROR_PANIC = 0,
    TT_RETURN,
    TT_REDUCE,
    TT_SHIFT,
    TT_EPSILON,

    TT_COUNT
};

string const &TransitionTypeString (TransitionType transition_type);

// ///////////////////////////////////////////////////////////////////////////
// NPDA transitions
// ///////////////////////////////////////////////////////////////////////////

Graph::Transition NpdaReduceTransition (Uint32 reduction_rule_index);
Graph::Transition NpdaReturnTransition (string const &nonterminal_name, Uint32 nonterminal_token_index);
Graph::Transition NpdaShiftTransition (Uint32 transition_token_id, string const &token_label, Uint32 target_index);
Graph::Transition NpdaEpsilonTransition (Uint32 target_index);

// ///////////////////////////////////////////////////////////////////////////
// DPDA transitions
// ///////////////////////////////////////////////////////////////////////////

enum
{
    IS_DEFAULT_TRANSITION = true,
    NOT_DEFAULT_TRANSITION = false
};

Graph::Transition DpdaReduceTransition (Uint32 reduction_rule_index);
Graph::Transition DpdaReturnTransition (string const &nonterminal_name, Uint32 nonterminal_token_index);
Graph::Transition DpdaShiftTransition (Graph::Transition::DataArray const &lookahead_sequence, string const &lookahead_sequence_string, Uint32 target_index);
Graph::Transition DpdaErrorPanicTransition ();

} // end of namespace Trison

#endif // !defined(TRISON_GRAPH_HPP_)
