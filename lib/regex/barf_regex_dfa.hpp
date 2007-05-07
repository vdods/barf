// ///////////////////////////////////////////////////////////////////////////
// barf_regex_dfa.hpp by Victor Dods, created 2006/12/31
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_REGEX_DFA_HPP_)
#define _BARF_REGEX_DFA_HPP_

#include "barf_regex.hpp"

#include <vector>

namespace Barf {

class Graph;

namespace Regex {

void GenerateDfa (
    Graph const &nfa_graph,
    Uint32 nfa_accept_state_count,
    vector<Uint32> const &nfa_start_state_index,
    Graph &dfa_graph,
    vector<Uint32> &dfa_start_state_index);

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(_BARF_REGEX_DFA_HPP_)
