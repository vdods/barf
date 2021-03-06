// 2007.03.19 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_REGEX_GRAPH_HPP_)
#define BARF_REGEX_GRAPH_HPP_

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

string const &TransitionTypeString (TransitionType transition_type);

string DfaTransitionString (Graph::Transition const &transition);
string NfaTransitionString (Graph::Transition const &transition);

Graph::Transition InputAtomTransition (Uint8 input_atom, Uint32 target_index);
Graph::Transition InputAtomRangeTransition (Uint8 range_lower, Uint8 range_upper, Uint32 target_index);
Graph::Transition EpsilonTransition (Uint32 target_index);
Graph::Transition DfaConditionalTransition (Uint8 mask, Uint8 flags, Uint32 target_index);
Graph::Transition NfaConditionalTransition (ConditionalType type, Uint32 target_index);

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(BARF_REGEX_GRAPH_HPP_)
