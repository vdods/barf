// 2006.12.31 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_REGEX_DFA_HPP_)
#define BARF_REGEX_DFA_HPP_

#include "barf_regex.hpp"

#include <vector>

namespace Barf {

class Graph;

namespace Regex {

void GenerateDfa (Automaton const &nfa, Uint32 nfa_accept_state_count, Automaton &dfa);

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(BARF_REGEX_DFA_HPP_)
