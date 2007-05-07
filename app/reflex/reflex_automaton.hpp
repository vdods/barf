// ///////////////////////////////////////////////////////////////////////////
// reflex_automaton.hpp by Victor Dods, created 2007/05/06
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_REFLEX_AUTOMATON_HPP_)
#define _REFLEX_AUTOMATON_HPP_

#include "reflex.hpp"

#include <vector>

namespace Barf {

class Graph;

} // end of namespace Barf

namespace Reflex {

class Representation;

void GenerateNfa (Representation const &representation, Graph &nfa_graph, vector<Uint32> &nfa_start_state_index);
void GenerateDfa (Graph const &nfa_graph, Uint32 nfa_accept_state_count, vector<Uint32> const &nfa_start_state_index, Graph &dfa_graph, vector<Uint32> &dfa_start_state_index);

void PrintDotGraph (Graph const &graph, string const &filename, string const &graph_name);

} // end of namespace Reflex

#endif // !defined(_REFLEX_AUTOMATON_HPP_)
