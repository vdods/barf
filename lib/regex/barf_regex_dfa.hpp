// ///////////////////////////////////////////////////////////////////////////
// barf_regex_dfa.hpp by Victor Dods, created 2006/12/31
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

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
