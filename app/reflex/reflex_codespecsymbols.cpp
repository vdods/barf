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

void PopulateAcceptHandlerStateMachineAndRegexSymbols (
    Rule const &rule,
    string const &state_machine_id,
    Preprocessor::ArraySymbol *accept_handler_state_machine,
    Preprocessor::ArraySymbol *accept_handler_regex)
{
    assert(accept_handler_state_machine != NULL);
    assert(accept_handler_regex != NULL);

    accept_handler_state_machine->AppendArrayElement(new Preprocessor::Body(state_machine_id));
    accept_handler_regex->AppendArrayElement(new Preprocessor::Body(rule.m_rule_regex_string));
}

void PopulateAcceptHandlerStateMachineAndRegexSymbols (
    StateMachine const &state_machine,
    Preprocessor::ArraySymbol *accept_handler_state_machine,
    Preprocessor::ArraySymbol *accept_handler_regex)
{
    assert(accept_handler_state_machine != NULL);
    assert(accept_handler_regex != NULL);

    for (RuleList::const_iterator it = state_machine.m_rule_list->begin(),
                                  it_end = state_machine.m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        PopulateAcceptHandlerStateMachineAndRegexSymbols(
            *rule,
            state_machine.m_state_machine_id->GetText(),
            accept_handler_state_machine,
            accept_handler_regex);
    }
}

void GenerateGeneralAutomatonSymbols (PrimarySource const &primary_source, Preprocessor::SymbolTable &symbol_table)
{
    EmitExecutionMessage("generating general automaton codespec symbols");

    // _accept_handler_count -- gives the number of accept handlers.
    {
        Preprocessor::ScalarSymbol *accept_handler_count =
            symbol_table.DefineScalarSymbol("_accept_handler_count", FiLoc::ms_invalid);
        accept_handler_count->SetScalarBody(
            new Preprocessor::Body(Sint32(primary_source.GetRuleCount())));
    }

    // _accept_handler_state_machine -- the name of the state machine this accept handler belongs to
    //
    // _accept_handler_regex -- a string literal of the regex associated with each accept handler
    {
        Preprocessor::ArraySymbol *accept_handler_state_machine =
            symbol_table.DefineArraySymbol("_accept_handler_state_machine", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *accept_handler_regex =
            symbol_table.DefineArraySymbol("_accept_handler_regex", FiLoc::ms_invalid);
        for (StateMachineMap::const_iterator state_machine_it = primary_source.m_state_machine_map->begin(),
                                            state_machine_it_end = primary_source.m_state_machine_map->end();
            state_machine_it != state_machine_it_end;
            ++state_machine_it)
        {
            StateMachine const *state_machine = state_machine_it->second;
            assert(state_machine != NULL);
            PopulateAcceptHandlerStateMachineAndRegexSymbols(*state_machine, accept_handler_state_machine, accept_handler_regex);
        }
    }

    // _start_with_state_machine -- value of %start_with_state_machine -- the name of the initial state machine
    {
        assert(primary_source.m_start_with_state_machine_directive != NULL);
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol("_start_with_state_machine", FiLoc::ms_invalid);
        symbol->SetScalarBody(
            new Preprocessor::Body(primary_source.m_start_with_state_machine_directive->m_state_machine_id->GetText()));
    }

    // _state_machine_mode_flags[state machine name] -- maps state machine name => state machine mode flags
    // where the flags are:
    //      0x01 - case insensitive
    //      0x02 - forgetful
    //      0x04 - ungreedy
    // the default behavior of the scanner is case sensitive, greedy, and not forgetful.
    // see StateMachine::ModeFlags.
    {
        Preprocessor::MapSymbol *state_machine_mode_flags =
            symbol_table.DefineMapSymbol("_state_machine_mode_flags", FiLoc::ms_invalid);
        for (StateMachineMap::const_iterator it = primary_source.m_state_machine_map->begin(),
                                             it_end = primary_source.m_state_machine_map->end();
            it != it_end;
            ++it)
        {
            string const &state_machine_name = it->first;
            StateMachine const *state_machine = it->second;
            assert(state_machine != NULL);
            state_machine_mode_flags->SetMapElement(
                state_machine_name,
                new Preprocessor::Body(Sint32(state_machine->m_mode_flags)));
        }
    }
}

void GenerateNfaSymbols (PrimarySource const &primary_source, Graph const &nfa_graph, vector<Uint32> const &nfa_start_state_index, Preprocessor::SymbolTable &symbol_table)
{
    assert(nfa_graph.GetNodeCount() > 0);

    EmitExecutionMessage("generating NFA codespec symbols");
    
    // _nfa_initial_node_index[state machine name] -- maps state machine name => node index
    {
        Preprocessor::MapSymbol *nfa_initial_node_index =
            symbol_table.DefineMapSymbol("_nfa_initial_node_index", FiLoc::ms_invalid);
        Uint32 state_index = 0;
        for (StateMachineMap::const_iterator it = primary_source.m_state_machine_map->begin(),
                                            it_end = primary_source.m_state_machine_map->end();
            it != it_end;
            ++it)
        {
            string const &state_machine_name = it->first;
            StateMachine const *state_machine = it->second;
            assert(state_machine != NULL);
            assert(state_index < nfa_start_state_index.size());
            nfa_initial_node_index->SetMapElement(
                state_machine_name,
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
    // of the transition type.  valid values are INPUT_ATOM=0, INPUT_ATOM_RANGE=1,
    // CONDITIONAL=2, EPSILON=3.
    //
    // _nfa_transition_type_name[_nfa_transition_count] -- gives the text name
    // of the transition type.  valid values are "INPUT_ATOM", "INPUT_ATOM_RANGE",
    // "CONDITIONAL", "EPSILON".
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

            for (Graph::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
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
                    new Preprocessor::Body(Sint32(transition.Data(0))));
                nfa_transition_data_1->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data(1))));
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

    EmitExecutionMessage("generating DFA codespec symbols");
    
    // _dfa_initial_node_index[state machine name] -- maps state machine name => node index
    {
        Preprocessor::MapSymbol *dfa_initial_node_index =
            symbol_table.DefineMapSymbol("_dfa_initial_node_index", FiLoc::ms_invalid);
        Uint32 state_index = 0;
        for (StateMachineMap::const_iterator it = primary_source.m_state_machine_map->begin(),
                                            it_end = primary_source.m_state_machine_map->end();
            it != it_end;
            ++it)
        {
            string const &state_machine_name = it->first;
            StateMachine const *state_machine = it->second;
            assert(state_machine != NULL);
            assert(state_index < dfa_start_state_index.size());
            dfa_initial_node_index->SetMapElement(
                state_machine_name,
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
    // _dfa_state_description[_dfa_state_count] -- the description of this graph node
    // (e.g. "25:SCAN_TEXT, 5, 8").
    //
    // _dfa_transition_count -- gives the number of transitions in this DFA
    //
    // _dfa_transition_type_integer[_dfa_transition_count] -- gives the integer value
    // of the transition type.  valid values are INPUT_ATOM=0, INPUT_ATOM_RANGE=1,
    // CONDITIONAL=2.
    //
    // _dfa_transition_type_name[_dfa_transition_count] -- gives the text name
    // of the transition type.  valid values are "INPUT_ATOM", "INPUT_ATOM_RANGE",
    // "CONDITIONAL".
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
        Preprocessor::ArraySymbol *dfa_state_description =
            symbol_table.DefineArraySymbol("_dfa_state_description", FiLoc::ms_invalid);
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
            Regex::NodeData const &node_data = dfa_graph.GetNode(node_index).GetDataAs<Regex::NodeData>();
            assert(node_data.m_dfa_accept_handler_index <= SINT32_UPPER_BOUND);
            Sint32 node_accept_handler_index = node_data.m_dfa_accept_handler_index;
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
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
                    new Preprocessor::Body(Sint32(transition.Data(0))));
                dfa_transition_data_1->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data(1))));
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
            dfa_state_description->AppendArrayElement(
                new Preprocessor::Body(node_data.GetAsText(node_index)));
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

void PopulateAcceptHandlerCodeArraySymbol (StateMachine const &state_machine, string const &target_id, Preprocessor::ArraySymbol *accept_handler_code)
{
    assert(accept_handler_code != NULL);

    for (RuleList::const_iterator it = state_machine.m_rule_list->begin(),
                                  it_end = state_machine.m_rule_list->end();
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
    EmitExecutionMessage("generating codespec symbols for target \"" + target_id + "\"");
    
    // _accept_handler_code[_accept_handler_count] -- specifies code
    // for all accept handlers.
    {
        Preprocessor::ArraySymbol *accept_handler_code =
            symbol_table.DefineArraySymbol("_accept_handler_code", FiLoc::ms_invalid);

        for (StateMachineMap::const_iterator it = primary_source.m_state_machine_map->begin(),
                                            it_end = primary_source.m_state_machine_map->end();
            it != it_end;
            ++it)
        {
            StateMachine const *state_machine = it->second;
            assert(state_machine != NULL);
            PopulateAcceptHandlerCodeArraySymbol(*state_machine, target_id, accept_handler_code);
        }
    }
}

} // end of namespace Reflex
