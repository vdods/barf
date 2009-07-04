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
    StateMachine const &state_machine,
    Automaton &nfa,
//     Graph &nfa_graph,
//     vector<Uint32> &nfa_start_state_index,
    Uint32 &next_accept_handler_index)
{
    Uint32 master_start_state_index =
        nfa.m_graph.AddNode(
            new Regex::NodeData(
                Regex::IS_START_NODE,
                Regex::NOT_ACCEPT_NODE,
                state_machine.m_state_machine_id->GetText()));
    nfa.m_start_state_index.push_back(master_start_state_index);

    // each rule is effectively or'ed together (i.e. using regex operator '|')
    // except that each rule ends alone at its own accept state.
    for (RuleList::const_iterator it = state_machine.m_rule_list->begin(),
                                  it_end = state_machine.m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        GenerateNfa(*rule, nfa.m_graph, master_start_state_index, next_accept_handler_index++);
    }
}

void GenerateNfa (PrimarySource const &primary_source, Automaton &nfa)
{
    assert(nfa.m_graph.NodeCount() == 0);
    assert(nfa.m_start_state_index.empty());

    // pre-allocate all the accept handler state nodes, because the first
    // contiguous block of N nodes are reserved for accept states (where N
    // is the number of accept states).
    for (Uint32 i = 0, accept_handler_count = primary_source.RuleCount();
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
    for (StateMachineMap::const_iterator it = primary_source.m_state_machine_map->begin(),
                                        it_end = primary_source.m_state_machine_map->end();
         it != it_end;
         ++it)
    {
        StateMachine const *state_machine = it->second;
        assert(state_machine != NULL);
        GenerateNfa(*state_machine, nfa, next_accept_handler_index);
    }

    assert(nfa.m_graph.NodeCount() >= primary_source.m_state_machine_map->size());
    assert(nfa.m_start_state_index.size() == primary_source.m_state_machine_map->size());
}

void GenerateDfa (PrimarySource const &primary_source, Automaton const &nfa, Uint32 nfa_accept_state_count, Automaton &dfa)
{
    assert(dfa.m_graph.NodeCount() == 0);
    assert(dfa.m_start_state_index.empty());
    Regex::GenerateDfa(nfa, nfa_accept_state_count, dfa);
    assert(dfa.m_graph.NodeCount() >= primary_source.m_state_machine_map->size());
    assert(dfa.m_start_state_index.size() == primary_source.m_state_machine_map->size());
}

} // end of namespace Reflex
