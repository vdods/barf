// 2007.05.06 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(TRISON_CODESPECSYMBOLS_HPP_)
#define TRISON_CODESPECSYMBOLS_HPP_

#include "trison.hpp"

namespace Barf {

class Graph;

namespace Preprocessor {

class SymbolTable;

} // end of namespace Preprocessor
} // end of namespace Barf

namespace Trison {

struct PrimarySource;

void GenerateGeneralAutomatonSymbols (PrimarySource const &primary_source, Preprocessor::SymbolTable &symbol_table);
void GenerateNpdaSymbols (PrimarySource const &primary_source, Graph const &npda_graph, Preprocessor::SymbolTable &symbol_table);
void GenerateDpdaSymbols (PrimarySource const &primary_source, Graph const &dpda_graph, Preprocessor::SymbolTable &symbol_table);
void GenerateTargetDependentSymbols(PrimarySource const &primary_source, string const &target_id, Preprocessor::SymbolTable &symbol_table);

} // end of namespace Trison

#endif // !defined(TRISON_CODESPECSYMBOLS_HPP_)
