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

struct Automaton;
class Graph;

} // end of namespace Barf

namespace Reflex {

struct PrimarySource;

void GenerateNfa (PrimarySource const &primary_source, Automaton &nfa);
void GenerateDfa (Automaton const &nfa, Uint32 nfa_accept_state_count, Automaton &dfa);

} // end of namespace Reflex

#endif // !defined(_REFLEX_AUTOMATON_HPP_)
