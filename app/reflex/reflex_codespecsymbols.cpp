// ///////////////////////////////////////////////////////////////////////////
// reflex_codespecsymbols.cpp by Victor Dods, created 2007/05/06
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "reflex_codespecsymbols.hpp"

#include "barf_graph.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "barf_regex_graph.hpp"
#include "barf_regex_nfa.hpp"
#include "reflex_ast.hpp"

namespace Reflex {

void PopulateAcceptHandlerScannerModeAndRegexSymbols (
    Rule const &rule,
    string const &scanner_mode_id,
    Preprocessor::ArraySymbol *accept_handler_scanner_mode,
    Preprocessor::ArraySymbol *accept_handler_regex)
{
    assert(accept_handler_scanner_mode != NULL);
    assert(accept_handler_regex != NULL);
    
    accept_handler_scanner_mode->AppendArrayElement(new Preprocessor::Body(scanner_mode_id));
    accept_handler_regex->AppendArrayElement(new Preprocessor::Body(GetStringLiteral(rule.m_rule_regex_string)));
}

void PopulateAcceptHandlerScannerModeAndRegexSymbols (
    ScannerMode const &scanner_mode,
    Preprocessor::ArraySymbol *accept_handler_scanner_mode,
    Preprocessor::ArraySymbol *accept_handler_regex)
{
    assert(accept_handler_scanner_mode != NULL);
    assert(accept_handler_regex != NULL);

    for (RuleList::const_iterator it = scanner_mode.m_rule_list->begin(),
                                  it_end = scanner_mode.m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        PopulateAcceptHandlerScannerModeAndRegexSymbols(
            *rule, 
            scanner_mode.m_scanner_mode_id->GetText(), 
            accept_handler_scanner_mode, 
            accept_handler_regex);
    }
}

void GenerateGeneralAutomatonSymbols (PrimarySource const &primary_source, Preprocessor::SymbolTable &symbol_table)
{
    // _accept_handler_count -- gives the number of accept handlers.
    {
        Preprocessor::ScalarSymbol *accept_handler_count =
            symbol_table.DefineScalarSymbol("_accept_handler_count", FiLoc::ms_invalid);
        accept_handler_count->SetScalarBody(
            new Preprocessor::Body(Sint32(primary_source.GetRuleCount())));
    }
    
    // _accept_handler_scanner_mode -- the name of the scanner mode this accept handler belongs to
    //
    // _accept_handler_regex -- a string literal of the regex associated with each accept handler
    {
        Preprocessor::ArraySymbol *accept_handler_scanner_mode =
            symbol_table.DefineArraySymbol("_accept_handler_scanner_mode", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *accept_handler_regex =
            symbol_table.DefineArraySymbol("_accept_handler_regex", FiLoc::ms_invalid);
        for (ScannerModeMap::const_iterator scanner_mode_it = primary_source.m_scanner_mode_map->begin(),
                                            scanner_mode_it_end = primary_source.m_scanner_mode_map->end();
            scanner_mode_it != scanner_mode_it_end;
            ++scanner_mode_it)
        {
            ScannerMode const *scanner_mode = scanner_mode_it->second;
            assert(scanner_mode != NULL);
            PopulateAcceptHandlerScannerModeAndRegexSymbols(*scanner_mode, accept_handler_scanner_mode, accept_handler_regex);
        }
    }
    
    // _start_in_scanner_mode -- value of %start_in_scanner_mode -- the name of the initial scanner mode
    {
        assert(primary_source.m_start_in_scanner_mode_directive != NULL);
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol("_start_in_scanner_mode", FiLoc::ms_invalid);
        symbol->SetScalarBody(
            new Preprocessor::Body(primary_source.m_start_in_scanner_mode_directive->m_scanner_mode_id->GetText()));
    }
}

void GenerateNfaSymbols (PrimarySource const &primary_source, Graph const &nfa_graph, vector<Uint32> const &nfa_start_state_index, Preprocessor::SymbolTable &symbol_table)
{
    assert(nfa_graph.GetNodeCount() > 0);

    // _nfa_initial_node_index[scanner mode name] -- maps scanner mode name => node index
    {
        Preprocessor::MapSymbol *nfa_initial_node_index =
            symbol_table.DefineMapSymbol("_nfa_initial_node_index", FiLoc::ms_invalid);
        Uint32 state_index = 0;
        for (ScannerModeMap::const_iterator it = primary_source.m_scanner_mode_map->begin(),
                                            it_end = primary_source.m_scanner_mode_map->end();
            it != it_end;
            ++it)
        {
            string const &scanner_mode_name = it->first;
            ScannerMode const *scanner_mode = it->second;
            assert(scanner_mode != NULL);
            assert(state_index < nfa_start_state_index.size());
            nfa_initial_node_index->SetMapElement(
                scanner_mode_name,
                new Preprocessor::Body(Sint32(nfa_start_state_index[state_index])));
            ++state_index;
        }
    }

    // _nfa_state_count -- the number of nodes in the nondeterministic finite automaton (NFA)
    {
        Preprocessor::ScalarSymbol *nfa_state_count =
            symbol_table.DefineScalarSymbol("_nfa_state_count", FiLoc::ms_invalid);
        nfa_state_count->SetScalarBody(
            new Preprocessor::Body(Sint32(nfa_graph.GetNodeCount())));
    }

    // _nfa_state_transition_offset[_nfa_state_count] -- gives the first index of the
    // contiguous set of transitions for this node.
    //
    // _nfa_state_transition_count[_nfa_state_count] -- gives the number of transitions for
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
        Preprocessor::ArraySymbol *nfa_state_transition_offset =
            symbol_table.DefineArraySymbol("_nfa_state_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_state_transition_count =
            symbol_table.DefineArraySymbol("_nfa_state_transition_count", FiLoc::ms_invalid);
        Preprocessor::ScalarSymbol *nfa_transition_count =
            symbol_table.DefineScalarSymbol("_nfa_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_type_integer =
            symbol_table.DefineArraySymbol("_nfa_transition_type_integer", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_type_name =
            symbol_table.DefineArraySymbol("_nfa_transition_type_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_data_0 =
            symbol_table.DefineArraySymbol("_nfa_transition_data_0", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_data_1 =
            symbol_table.DefineArraySymbol("_nfa_transition_data_1", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nfa_transition_target_node_index =
            symbol_table.DefineArraySymbol("_nfa_transition_target_node_index", FiLoc::ms_invalid);

        Sint32 total_transition_count = 0;

        for (Uint32 node_index = 0; node_index < nfa_graph.GetNodeCount(); ++node_index)
        {
            Graph::Node const &node = nfa_graph.GetNode(node_index);
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::Node::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                            it_end = node.GetTransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                nfa_transition_type_integer->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Type())));
                nfa_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(Regex::GetTransitionTypeString(transition.Type())));
                nfa_transition_data_0->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data0())));
                nfa_transition_data_1->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data1())));
                nfa_transition_target_node_index->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.TargetIndex())));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(total_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            nfa_state_transition_offset->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_offset)));
            nfa_state_transition_count->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_count)));
        }

        nfa_transition_count->SetScalarBody(
            new Preprocessor::Body(Sint32(total_transition_count)));
    }
}

void GenerateDfaSymbols (PrimarySource const &primary_source, Graph const &dfa_graph, vector<Uint32> const &dfa_start_state_index, Preprocessor::SymbolTable &symbol_table)
{
    assert(dfa_graph.GetNodeCount() > 0);

    // _dfa_initial_node_index[scanner mode name] -- maps scanner mode name => node index
    {
        Preprocessor::MapSymbol *dfa_initial_node_index =
            symbol_table.DefineMapSymbol("_dfa_initial_node_index", FiLoc::ms_invalid);
        Uint32 state_index = 0;
        for (ScannerModeMap::const_iterator it = primary_source.m_scanner_mode_map->begin(),
                                            it_end = primary_source.m_scanner_mode_map->end();
            it != it_end;
            ++it)
        {
            string const &scanner_mode_name = it->first;
            ScannerMode const *scanner_mode = it->second;
            assert(scanner_mode != NULL);
            assert(state_index < dfa_start_state_index.size());
            dfa_initial_node_index->SetMapElement(
                scanner_mode_name,
                new Preprocessor::Body(Sint32(dfa_start_state_index[state_index])));
            ++state_index;
        }
    }

    // _dfa_state_count -- the number of nodes in the deterministic finite automaton (DFA)
    {
        Preprocessor::ScalarSymbol *dfa_state_count =
            symbol_table.DefineScalarSymbol("_dfa_state_count", FiLoc::ms_invalid);
        dfa_state_count->SetScalarBody(
            new Preprocessor::Body(Sint32(dfa_graph.GetNodeCount())));
    }

    // _dfa_state_accept_handler_index[_dfa_state_count] -- gives the accept-handler-index
    // for this node, or _dfa_state_count if this is not an accept state
    //
    // _dfa_state_transition_offset[_dfa_state_count] -- gives the first index of the
    // contiguous set of transitions for this node.
    //
    // _dfa_state_transition_count[_dfa_state_count] -- gives the number of transitions for
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
        Preprocessor::ArraySymbol *dfa_state_accept_handler_index =
            symbol_table.DefineArraySymbol("_dfa_state_accept_handler_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_transition_offset =
            symbol_table.DefineArraySymbol("_dfa_state_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_transition_count =
            symbol_table.DefineArraySymbol("_dfa_state_transition_count", FiLoc::ms_invalid);
        Preprocessor::ScalarSymbol *dfa_transition_count =
            symbol_table.DefineScalarSymbol("_dfa_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_type_integer =
            symbol_table.DefineArraySymbol("_dfa_transition_type_integer", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_type_name =
            symbol_table.DefineArraySymbol("_dfa_transition_type_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_data_0 =
            symbol_table.DefineArraySymbol("_dfa_transition_data_0", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_data_1 =
            symbol_table.DefineArraySymbol("_dfa_transition_data_1", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_transition_target_node_index =
            symbol_table.DefineArraySymbol("_dfa_transition_target_node_index", FiLoc::ms_invalid);

        Sint32 total_transition_count = 0;

        for (Uint32 node_index = 0; node_index < dfa_graph.GetNodeCount(); ++node_index)
        {
            Graph::Node const &node = dfa_graph.GetNode(node_index);
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

                dfa_transition_type_integer->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Type())));
                dfa_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(Regex::GetTransitionTypeString(transition.Type())));
                dfa_transition_data_0->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data0())));
                dfa_transition_data_1->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data1())));
                dfa_transition_target_node_index->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.TargetIndex())));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            dfa_state_accept_handler_index->AppendArrayElement(
                new Preprocessor::Body(node_accept_handler_index));
            dfa_state_transition_offset->AppendArrayElement(
                new Preprocessor::Body(node_transition_offset));
            dfa_state_transition_count->AppendArrayElement(
                new Preprocessor::Body(node_transition_count));
        }

        dfa_transition_count->SetScalarBody(
            new Preprocessor::Body(total_transition_count));
    }
}

void PopulateAcceptHandlerCodeArraySymbol (Rule const &rule, string const &target_id, Preprocessor::ArraySymbol *accept_handler_code)
{
    assert(accept_handler_code != NULL);
    CommonLang::RuleHandler const *rule_handler = rule.m_rule_handler_map->GetElement(target_id);
    assert(rule_handler != NULL);
    Ast::CodeBlock const *rule_handler_code_block = rule_handler->m_rule_handler_code_block;
    assert(rule_handler_code_block != NULL);
    accept_handler_code->AppendArrayElement(
        new Preprocessor::Body(
            rule_handler_code_block->GetText(),
            rule_handler_code_block->GetFiLoc()));
}

void PopulateAcceptHandlerCodeArraySymbol (ScannerMode const &scanner_mode, string const &target_id, Preprocessor::ArraySymbol *accept_handler_code)
{
    assert(accept_handler_code != NULL);

    for (RuleList::const_iterator it = scanner_mode.m_rule_list->begin(),
                                  it_end = scanner_mode.m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        PopulateAcceptHandlerCodeArraySymbol(*rule, target_id, accept_handler_code);
    }
}

void GenerateTargetDependentSymbols (PrimarySource const &primary_source, string const &target_id, Preprocessor::SymbolTable &symbol_table)
{
    // _accept_handler_code[_accept_handler_count] -- specifies code
    // for all accept handlers.
    {
        Preprocessor::ArraySymbol *accept_handler_code =
            symbol_table.DefineArraySymbol("_accept_handler_code", FiLoc::ms_invalid);

        for (ScannerModeMap::const_iterator it = primary_source.m_scanner_mode_map->begin(),
                                            it_end = primary_source.m_scanner_mode_map->end();
            it != it_end;
            ++it)
        {
            ScannerMode const *scanner_mode = it->second;
            assert(scanner_mode != NULL);
            PopulateAcceptHandlerCodeArraySymbol(*scanner_mode, target_id, accept_handler_code);
        }
    }
}

} // end of namespace Reflex
