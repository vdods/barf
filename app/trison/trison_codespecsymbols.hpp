// ///////////////////////////////////////////////////////////////////////////
// trison_codespecsymbols.hpp by Victor Dods, created 2007/05/06
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_CODESPECSYMBOLS_HPP_)
#define _TRISON_CODESPECSYMBOLS_HPP_

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

#endif // !defined(_TRISON_CODESPECSYMBOLS_HPP_)
