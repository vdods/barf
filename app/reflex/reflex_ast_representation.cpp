// ///////////////////////////////////////////////////////////////////////////
// reflex_ast_representation.cpp by Victor Dods, created 2006/10/21
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "reflex_ast.hpp"

#include <fstream>
#include <sstream>

#include "barf_graph.hpp"
#include "barf_message.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "barf_regex.hpp"
#include "barf_regex_dfa.hpp"
#include "barf_regex_graph.hpp"
#include "barf_regex_nfa.hpp"

#include "reflex_automaton.hpp" // TEMP
#include "reflex_codespecsymbols.hpp" // TEMP

namespace Reflex {

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Uint32 Representation::GetRuleCount () const
{
    Uint32 accept_handler_count = 0;
    for (ScannerModeMap::const_iterator it = m_scanner_mode_map->begin(),
                                         it_end = m_scanner_mode_map->end();
         it != it_end;
         ++it)
    {
        ScannerMode const *scanner_mode = it->second;
        assert(scanner_mode != NULL);
        accept_handler_count += scanner_mode->GetRuleCount();
    }
    return accept_handler_count;
}

Rule const *Representation::GetRule (Uint32 rule_index) const
{
    assert(rule_index < GetRuleCount());
    for (ScannerModeMap::const_iterator it = m_scanner_mode_map->begin(),
                                         it_end = m_scanner_mode_map->end();
         it != it_end;
         ++it)
    {
        ScannerMode const *scanner_mode = it->second;
        assert(scanner_mode != NULL);
        if (rule_index < scanner_mode->GetRuleCount())
            return scanner_mode->m_rule_list->GetElement(rule_index);
        else
            rule_index -= scanner_mode->GetRuleCount();
    }
    assert(false && "GetRuleCount() doesn't match reality");
    return NULL;
}

void Representation::GenerateNfaAndDfa () const
{
    GenerateNfa(*this, m_nfa_graph, m_nfa_start_state_index);
    GenerateDfa(m_nfa_graph, GetRuleCount(), m_nfa_start_state_index, m_dfa_graph, m_dfa_start_state_index);
}

void Representation::PrintNfaDotGraph (string const &filename, string const &graph_name) const
{
    PrintDotGraph(m_nfa_graph, filename, graph_name);
}

void Representation::PrintDfaDotGraph (string const &filename, string const &graph_name) const
{
    PrintDotGraph(m_dfa_graph, filename, graph_name);
}

void Representation::GenerateAutomatonSymbols (
    Preprocessor::SymbolTable &symbol_table) const
{
    GenerateGeneralAutomatonSymbols(*this, symbol_table);
    GenerateNfaSymbols(*this, m_nfa_graph, m_nfa_start_state_index, symbol_table);
    GenerateDfaSymbols(*this, m_dfa_graph, m_dfa_start_state_index, symbol_table);
}

void Representation::GenerateTargetDependentSymbols (
    string const &target_id,
    Preprocessor::SymbolTable &symbol_table) const
{
    Reflex::GenerateTargetDependentSymbols(*this, target_id, symbol_table);
}

} // end of namespace Reflex
