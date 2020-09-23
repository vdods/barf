// 2007.05.06 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(REFLEX_AUTOMATON_HPP_)
#define REFLEX_AUTOMATON_HPP_

#include "reflex.hpp"

#include <vector>

namespace Barf {

struct Automaton;
class Graph;

} // end of namespace Barf

namespace Reflex {

struct PrimarySource;

void GenerateNfa (PrimarySource const &primary_source, Automaton &nfa);
void GenerateDfa (PrimarySource const &primary_source, Automaton const &nfa, Uint32 nfa_accept_state_count, Automaton &dfa);

} // end of namespace Reflex

#endif // !defined(REFLEX_AUTOMATON_HPP_)
