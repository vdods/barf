// 2007.05.06 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(REFLEX_CODESPECSYMBOLS_HPP_)
#define REFLEX_CODESPECSYMBOLS_HPP_

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

#endif // !defined(REFLEX_CODESPECSYMBOLS_HPP_)
