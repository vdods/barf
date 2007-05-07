// ///////////////////////////////////////////////////////////////////////////
// reflex_automaton.cpp by Victor Dods, created 2007/05/06
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "reflex_automaton.hpp"

#include <fstream>

#include "barf_graph.hpp"
#include "barf_regex_dfa.hpp"
#include "barf_regex_nfa.hpp"
#include "reflex_ast.hpp"

namespace Reflex {

void GenerateNfa (Rule const &rule, Graph &graph, Uint32 start_index, Uint32 end_index)
{
    assert(rule.m_rule_regex != NULL);
    Regex::GenerateNfa(*rule.m_rule_regex, graph, start_index, end_index);
}

void GenerateNfa (
    ScannerMode const &scanner_mode,
    Graph &nfa_graph,
    vector<Uint32> &nfa_start_state_index,
    Uint32 &next_accept_handler_index)
{
    Uint32 master_start_state_index =
        nfa_graph.AddNode(
            new Regex::NodeData(
                Regex::IS_START_NODE,
                Regex::NOT_ACCEPT_NODE,
                scanner_mode.m_scanner_mode_id->GetText()));
    nfa_start_state_index.push_back(master_start_state_index);

    // each rule is effectively or'ed together (i.e. using regex operator '|')
    // except that each rule ends alone at its own accept state.
    for (RuleList::const_iterator it = scanner_mode.m_rule_list->begin(),
                                  it_end = scanner_mode.m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        GenerateNfa(*rule, nfa_graph, master_start_state_index, next_accept_handler_index++);
    }
}

void GenerateNfa (PrimarySource const &primary_source, Automaton &nfa)
{
    assert(nfa.m_graph.GetNodeCount() == 0);
    assert(nfa.m_start_state_index.empty());

    // pre-allocate all the accept handler state nodes, because the first
    // contiguous block of N nodes are reserved for accept states (where N
    // is the number of accept states).
    for (Uint32 i = 0, accept_handler_count = primary_source.GetRuleCount();
         i < accept_handler_count;
         ++i)
    {
        nfa.m_graph.AddNode(
            new Regex::NodeData(
                Regex::NOT_START_NODE,
                Regex::IS_ACCEPT_NODE,
                string("(") + primary_source.GetRule(i)->m_rule_regex_string + ")",
                i));
    }

    Uint32 next_accept_handler_index = 0;
    for (ScannerModeMap::const_iterator it = primary_source.m_scanner_mode_map->begin(),
                                        it_end = primary_source.m_scanner_mode_map->end();
         it != it_end;
         ++it)
    {
        ScannerMode const *scanner_mode = it->second;
        assert(scanner_mode != NULL);
        GenerateNfa(*scanner_mode, nfa.m_graph, nfa.m_start_state_index, next_accept_handler_index);
    }

    assert(nfa.m_graph.GetNodeCount() >= primary_source.m_scanner_mode_map->size());
    assert(nfa.m_start_state_index.size() == primary_source.m_scanner_mode_map->size());
}

void GenerateDfa (Automaton const &nfa, Uint32 nfa_accept_state_count, Automaton &dfa)
{
    assert(dfa.m_graph.GetNodeCount() == 0);
    assert(dfa.m_start_state_index.empty());
    Regex::GenerateDfa(nfa, nfa_accept_state_count, dfa);
}

} // end of namespace Reflex