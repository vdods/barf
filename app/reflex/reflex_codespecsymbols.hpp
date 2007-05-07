// ///////////////////////////////////////////////////////////////////////////
// reflex_codespecsymbols.hpp by Victor Dods, created 2007/05/06
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_REFLEX_CODESPECSYMBOLS_HPP_)
#define _REFLEX_CODESPECSYMBOLS_HPP_

#include "reflex.hpp"

#include <vector>

namespace Barf {

class Graph;

namespace Preprocessor {

class SymbolTable;

} // end of namespace Preprocessor
} // end of namespace Barf

namespace Reflex {

struct PrimarySource;

void GenerateGeneralAutomatonSymbols (PrimarySource const &primary_source, Preprocessor::SymbolTable &symbol_table);
void GenerateNfaSymbols (PrimarySource const &primary_source, Graph const &nfa_graph, vector<Uint32> const &nfa_start_state_index, Preprocessor::SymbolTable &symbol_table);
void GenerateDfaSymbols (PrimarySource const &primary_source, Graph const &dfa_graph, vector<Uint32> const &dfa_start_state_index, Preprocessor::SymbolTable &symbol_table);
void GenerateTargetDependentSymbols (PrimarySource const &primary_source, string const &target_id, Preprocessor::SymbolTable &symbol_table);

} // end of namespace Reflex

#endif // !defined(_REFLEX_CODESPECSYMBOLS_HPP_)
