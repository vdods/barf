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

namespace Reflex {

void Rule::GenerateNfa (Graph &graph, Uint32 start_index, Uint32 end_index) const
{
    assert(m_rule_regex != NULL);
    Regex::GenerateNfa(*m_rule_regex, graph, start_index, end_index);
}

void Rule::PopulateAcceptHandlerCodeArraySymbol (
    string const &target_language_id,
    Preprocessor::ArraySymbol *accept_handler_code_symbol) const
{
    assert(accept_handler_code_symbol != NULL);
    CommonLang::RuleHandler const *rule_handler = m_rule_handler_map->GetElement(target_language_id);
    assert(rule_handler != NULL);
    Ast::CodeBlock const *rule_handler_code_block = rule_handler->m_rule_handler_code_block;
    assert(rule_handler_code_block != NULL);
    accept_handler_code_symbol->AppendArrayElement(
        new Preprocessor::Body(
            rule_handler_code_block->GetText(),
            rule_handler_code_block->GetFiLoc()));
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void ScannerState::GenerateNfa (
    Graph &nfa_graph,
    vector<Uint32> &start_state_index_array,
    Uint32 &next_accept_handler_index) const
{
    Uint32 master_start_state_index =
        nfa_graph.AddNode(
            new Regex::NodeData(
                Regex::IS_START_NODE,
                Regex::NOT_ACCEPT_NODE,
                m_scanner_state_id->GetText()));
    start_state_index_array.push_back(master_start_state_index);

    // each rule is effectively or'ed together (i.e. using regex operator '|')
    // except that each rule ends alone at its own accept state.
    for (RuleList::const_iterator it = m_rule_list->begin(),
                                  it_end = m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        rule->GenerateNfa(nfa_graph, master_start_state_index, next_accept_handler_index++);
    }
}

void ScannerState::PopulateAcceptHandlerCodeArraySymbol (
    string const &target_language_id,
    Preprocessor::ArraySymbol *accept_handler_code_symbol) const
{
    assert(accept_handler_code_symbol != NULL);

    for (RuleList::const_iterator it = m_rule_list->begin(),
                                  it_end = m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        rule->PopulateAcceptHandlerCodeArraySymbol(target_language_id, accept_handler_code_symbol);
    }
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Uint32 Representation::GetAcceptHandlerCount () const
{
    Uint32 accept_handler_count = 0;
    for (ScannerStateMap::const_iterator it = m_scanner_state_map->begin(),
                                         it_end = m_scanner_state_map->end();
         it != it_end;
         ++it)
    {
        ScannerState const *scanner_state = it->second;
        assert(scanner_state != NULL);
        accept_handler_count += scanner_state->GetAcceptHandlerCount();
    }
    return accept_handler_count;
}

Rule const *Representation::GetAcceptHandlerRule (Uint32 rule_index) const
{
    assert(rule_index < GetAcceptHandlerCount());
    for (ScannerStateMap::const_iterator it = m_scanner_state_map->begin(),
                                         it_end = m_scanner_state_map->end();
         it != it_end;
         ++it)
    {
        ScannerState const *scanner_state = it->second;
        assert(scanner_state != NULL);
        if (rule_index < scanner_state->GetAcceptHandlerCount())
            return scanner_state->m_rule_list->GetElement(rule_index);
        else
            rule_index -= scanner_state->GetAcceptHandlerCount();
    }
    assert(false && "GetAcceptHandlerCount() doesn't match reality");
    return NULL;
}

void Representation::GenerateNfaAndDfa () const
{
    // generate the NFA
    {
        assert(m_nfa_graph.GetNodeCount() == 0);
        assert(m_nfa_start_state.empty());
        assert(m_next_accept_handler_index == 0);

        // pre-allocate all the accept handler state nodes, because the first
        // contiguous block of N nodes are reserved for accept states (where N
        // is the number of accept states).
        for (Uint32 i = 0, accept_handler_count = GetAcceptHandlerCount();
             i < accept_handler_count;
             ++i)
        {
            m_nfa_graph.AddNode(
                new Regex::NodeData(
                    Regex::NOT_START_NODE,
                    Regex::IS_ACCEPT_NODE,
                    string("(") + GetAcceptHandlerRule(i)->m_rule_regex_string + ")",
                    i));
        }

        for (ScannerStateMap::const_iterator it = m_scanner_state_map->begin(),
                                             it_end = m_scanner_state_map->end();
             it != it_end;
             ++it)
        {
            ScannerState const *scanner_state = it->second;
            assert(scanner_state != NULL);
            scanner_state->GenerateNfa(
                m_nfa_graph,
                m_nfa_start_state,
                m_next_accept_handler_index);
        }

        assert(m_nfa_graph.GetNodeCount() >= m_scanner_state_map->size());
        assert(m_nfa_start_state.size() == m_scanner_state_map->size());
    }

    // generate the DFA
    Regex::GenerateDfa(m_nfa_graph, GetAcceptHandlerCount(), m_nfa_start_state, m_dfa_graph, m_dfa_start_state);
}

void Representation::PrintNfaGraph (string const &filename, string const &graph_name) const
{
    // don't print anything if no filename was specified.
    if (filename.empty())
        return;

    if (filename == "-")
        m_nfa_graph.PrintDotGraph(cout, graph_name);
    else
    {
        ofstream file(filename.c_str());
        if (file.is_open())
            m_nfa_graph.PrintDotGraph(file, graph_name);
        else
            EmitWarning("could not open file \"" + filename + "\" for writing");
    }
}

void Representation::PrintDfaGraph (string const &filename, string const &graph_name) const
{
    // don't print anything if no filename was specified.
    if (filename.empty())
        return;

    if (filename == "-")
        m_dfa_graph.PrintDotGraph(cout, graph_name);
    else
    {
        ofstream file(filename.c_str());
        if (file.is_open())
            m_dfa_graph.PrintDotGraph(file, graph_name);
        else
            EmitWarning("could not open file \"" + filename + "\" for writing");
    }
}

void Representation::GenerateAutomatonSymbols (
    Preprocessor::SymbolTable &symbol_table) const
{
    assert(m_nfa_graph.GetNodeCount() > 0);
    assert(m_dfa_graph.GetNodeCount() > 0);

    // _start -- value of %start -- the name of the initial scanner state
    {
        assert(m_start_directive != NULL);
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol("_start", FiLoc::ms_invalid);
        symbol->SetScalarBody(
            new Preprocessor::Body(
                m_start_directive->m_start_state_id->GetText(),
                FiLoc::ms_invalid));
    }

    // _nfa_initial_node_index[scanner state name] -- maps scanner state name => node index
    {
        Preprocessor::MapSymbol *nfa_initial_node_index_symbol =
            symbol_table.DefineMapSymbol("_nfa_initial_node_index", FiLoc::ms_invalid);
        Uint32 state_index = 0;
        for (ScannerStateMap::const_iterator it = m_scanner_state_map->begin(),
                                             it_end = m_scanner_state_map->end();
            it != it_end;
            ++it)
        {
            string const &scanner_state_name = it->first;
            ScannerState const *scanner_state = it->second;
            assert(scanner_state != NULL);
            assert(state_index < m_nfa_start_state.size());
            nfa_initial_node_index_symbol->SetMapElement(
                scanner_state_name,
                new Preprocessor::Body(
                    Sint32(m_nfa_start_state[state_index]),
                    FiLoc::ms_invalid));
            ++state_index;
        }
    }

    // _nfa_node_count -- the number of nodes in the nondeterministic finite automaton (NFA)
    {
        Preprocessor::ScalarSymbol *nfa_node_count_symbol =
            symbol_table.DefineScalarSymbol("_nfa_node_count", FiLoc::ms_invalid);
        nfa_node_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(m_nfa_graph.GetNodeCount()),
                FiLoc::ms_invalid));
    }

    // _nfa_node_transition_offset[_nfa_node_count] -- gives the first index of the
    // contiguous set of transitions for this node.
    //
    // _nfa_node_transition_count[_nfa_node_count] -- gives the number of transitions for
    // this node (the number of contiguous transitions which apply to this node).
    //
    // _nfa_transition_count -- gives the number of transitions in this NFA
    //
    // _nfa_transition_type_integer[_nfa_transition_count] -- gives the integer value
    // of the transition type -- 0 (TT_INPUT_ATOM), 1 (TT_INPUT_ATOM_RANGE),
    // 2 (TT_CONDITIONAL), or 3 (TT_EPSILON).
    //
    // _nfa_transition_type_name[_nfa_transition_count] -- gives the text name
    // of the transition type (i.e. "TT_EPSILON", etc).
    //
    // _nfa_transition_data_0[_nfa_transition_count] and
    // _nfa_transition_data_1[_nfa_transition_count] give the applicable associated
    // transition data.  For transition type TT_INPUT_ATOM, both data are the same
    // single ASCII value which this transition accepts.  For TT_INPUT_ATOM_RANGE,
    // data_0 and data_1 specify the ASCII values of the lower and upper range ends
    // for the acceptable input for this transition.  For TT_CONDITIONAL, both data
    // are the same single conditional enum value (see ConditionalType).  For
    // TT_EPSILON, both data are 0.
    //
    // _nfa_transition_target_node_index[_nfa_transition_count] gives the index of
    // the node which to transition to if this transition is exercised.
    {
        Preprocessor::ArraySymbol *nfa_node_transition_offset_symbol =
            symbol_table.DefineArraySymbol("_nfa_node_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_node_transition_count_symbol =
            symbol_table.DefineArraySymbol("_nfa_node_transition_count", FiLoc::ms_invalid);
        Preprocessor::ScalarSymbol *nfa_transition_count_symbol =
            symbol_table.DefineScalarSymbol("_nfa_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_type_integer_symbol =
            symbol_table.DefineArraySymbol("_nfa_transition_type_integer", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_type_name_symbol =
            symbol_table.DefineArraySymbol("_nfa_transition_type_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_data_0_symbol =
            symbol_table.DefineArraySymbol("_nfa_transition_data_0", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_data_1_symbol =
            symbol_table.DefineArraySymbol("_nfa_transition_data_1", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_target_node_index_symbol =
            symbol_table.DefineArraySymbol("_nfa_transition_target_node_index", FiLoc::ms_invalid);

        Sint32 total_transition_count = 0;

        for (Uint32 node_index = 0; node_index < m_nfa_graph.GetNodeCount(); ++node_index)
        {
            Graph::Node const &node = m_nfa_graph.GetNode(node_index);
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::Node::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                            it_end = node.GetTransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                nfa_transition_type_integer_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Type()),
                        FiLoc::ms_invalid));
                nfa_transition_type_name_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Regex::GetTransitionTypeString(transition.Type()),
                        FiLoc::ms_invalid));
                nfa_transition_data_0_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Data0()),
                        FiLoc::ms_invalid));
                nfa_transition_data_1_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Data1()),
                        FiLoc::ms_invalid));
                nfa_transition_target_node_index_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.TargetIndex()),
                        FiLoc::ms_invalid));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(total_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            nfa_node_transition_offset_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(node_transition_offset),
                    FiLoc::ms_invalid));
            nfa_node_transition_count_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(node_transition_count),
                    FiLoc::ms_invalid));
        }

        nfa_transition_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(total_transition_count),
                FiLoc::ms_invalid));
    }

    // _dfa_initial_node_index[scanner state name] -- maps scanner state name => node index
    {
        Preprocessor::MapSymbol *dfa_initial_node_index_symbol =
            symbol_table.DefineMapSymbol("_dfa_initial_node_index", FiLoc::ms_invalid);
        Uint32 state_index = 0;
        for (ScannerStateMap::const_iterator it = m_scanner_state_map->begin(),
                                             it_end = m_scanner_state_map->end();
            it != it_end;
            ++it)
        {
            string const &scanner_state_name = it->first;
            ScannerState const *scanner_state = it->second;
            assert(scanner_state != NULL);
            assert(state_index < m_dfa_start_state.size());
            dfa_initial_node_index_symbol->SetMapElement(
                scanner_state_name,
                new Preprocessor::Body(
                    Sint32(m_dfa_start_state[state_index]),
                    FiLoc::ms_invalid));
            ++state_index;
        }
    }

    // _dfa_node_count -- the number of nodes in the deterministic finite automaton (DFA)
    {
        Preprocessor::ScalarSymbol *dfa_node_count_symbol =
            symbol_table.DefineScalarSymbol("_dfa_node_count", FiLoc::ms_invalid);
        dfa_node_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(m_dfa_graph.GetNodeCount()),
                FiLoc::ms_invalid));
    }

    // _dfa_node_accept_handler_index[_dfa_node_count] -- gives the accept-handler-index
    // for this node, or _dfa_node_count if this is not an accept state
    //
    // _dfa_node_transition_offset[_dfa_node_count] -- gives the first index of the
    // contiguous set of transitions for this node.
    //
    // _dfa_node_transition_count[_dfa_node_count] -- gives the number of transitions for
    // this node (the number of contiguous transitions which apply to this node).
    //
    // _dfa_transition_count -- gives the number of transitions in this DFA
    //
    // _dfa_transition_type_integer[_dfa_transition_count] -- gives the integer value
    // of the transition type (i.e. the enum value of TT_EPSILON (3), etc).
    //
    // _dfa_transition_type_name[_dfa_transition_count] -- gives the text name
    // of the transition type (i.e. "TT_EPSILON", etc).
    //
    // _dfa_transition_data_0[_dfa_transition_count] and
    // _dfa_transition_data_1[_dfa_transition_count] give the data needed by
    // the different types of transitions.  TT_INPUT_ATOM uses only _dfa_transition_data_0
    // to indicate which atom value will be accepted.  TT_INPUT_ATOM_RANGE accepts all
    // atom values between _dfa_transition_data_0 and _dfa_transition_data_1, inclusive.
    // TT_EPSILON does not use either piece of data.  TT_CONDITIONAL interprets
    // _dfa_transition_data_0 as the bitmask for applicable conditionals and
    // _dfa_transition_data_1 as the values for said conditionals.  any other
    // usage of these values is undefined.
    //
    // _dfa_transition_target_node_index[_dfa_transition_count] gives the index of
    // the node which to transition to if this transition is exercised.
    {
        Preprocessor::ArraySymbol *dfa_node_accept_handler_index_symbol =
            symbol_table.DefineArraySymbol("_dfa_node_accept_handler_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_node_transition_offset_symbol =
            symbol_table.DefineArraySymbol("_dfa_node_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_node_transition_count_symbol =
            symbol_table.DefineArraySymbol("_dfa_node_transition_count", FiLoc::ms_invalid);
        Preprocessor::ScalarSymbol *dfa_transition_count_symbol =
            symbol_table.DefineScalarSymbol("_dfa_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_type_integer_symbol =
            symbol_table.DefineArraySymbol("_dfa_transition_type_integer", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_type_name_symbol =
            symbol_table.DefineArraySymbol("_dfa_transition_type_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_data_0_symbol =
            symbol_table.DefineArraySymbol("_dfa_transition_data_0", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_data_1_symbol =
            symbol_table.DefineArraySymbol("_dfa_transition_data_1", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_target_node_index_symbol =
            symbol_table.DefineArraySymbol("_dfa_transition_target_node_index", FiLoc::ms_invalid);

        Sint32 total_transition_count = 0;

        for (Uint32 node_index = 0; node_index < m_dfa_graph.GetNodeCount(); ++node_index)
        {
            Graph::Node const &node = m_dfa_graph.GetNode(node_index);
            assert(node.GetHasData());
            assert(node.GetDataAs<Regex::NodeData>().m_dfa_accept_handler_index <= SINT32_UPPER_BOUND);
            Sint32 node_accept_handler_index = node.GetDataAs<Regex::NodeData>().m_dfa_accept_handler_index;
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::Node::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                            it_end = node.GetTransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                dfa_transition_type_integer_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Type()),
                        FiLoc::ms_invalid));
                dfa_transition_type_name_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Regex::GetTransitionTypeString(transition.Type()),
                        FiLoc::ms_invalid));
                dfa_transition_data_0_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Data0()),
                        FiLoc::ms_invalid));
                dfa_transition_data_1_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Data1()),
                        FiLoc::ms_invalid));
                dfa_transition_target_node_index_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.TargetIndex()),
                        FiLoc::ms_invalid));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            dfa_node_accept_handler_index_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    node_accept_handler_index,
                    FiLoc::ms_invalid));
            dfa_node_transition_offset_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    node_transition_offset,
                    FiLoc::ms_invalid));
            dfa_node_transition_count_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    node_transition_count,
                    FiLoc::ms_invalid));
        }

        dfa_transition_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                total_transition_count,
                FiLoc::ms_invalid));
    }
}

void Representation::GenerateTargetLanguageDependentSymbols (
    string const &target_language_id,
    Preprocessor::SymbolTable &symbol_table) const
{
    // _accept_handler_count -- gives the number of accept handlers.
    //
    // _accept_handler_code[_accept_handler_count+1] -- specifies code
    // for all accept handlers.
    {
        Preprocessor::ScalarSymbol *accept_handler_count_symbol =
            symbol_table.DefineScalarSymbol("_accept_handler_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *accept_handler_code_symbol =
            symbol_table.DefineArraySymbol("_accept_handler_code", FiLoc::ms_invalid);

        accept_handler_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(m_next_accept_handler_index),
                FiLoc::ms_invalid));
        for (ScannerStateMap::const_iterator it = m_scanner_state_map->begin(),
                                             it_end = m_scanner_state_map->end();
            it != it_end;
            ++it)
        {
            ScannerState const *scanner_state = it->second;
            assert(scanner_state != NULL);
            scanner_state->PopulateAcceptHandlerCodeArraySymbol(
                target_language_id,
                accept_handler_code_symbol);
        }
    }
}

} // end of namespace Reflex
