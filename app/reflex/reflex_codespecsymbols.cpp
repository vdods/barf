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
            new Preprocessor::Body(Sint32(primary_source.RuleCount())));
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
    //      0x02 - ungreedy
    // the default behavior of the scanner is case sensitive and greedy.
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
    assert(nfa_graph.NodeCount() > 0);

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
            new Preprocessor::Body(Sint32(nfa_graph.NodeCount())));
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

        for (Uint32 node_index = 0; node_index < nfa_graph.NodeCount(); ++node_index)
        {
            Graph::Node const &node = nfa_graph.GetNode(node_index);
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::TransitionSet::const_iterator it = node.TransitionSetBegin(),
                                                      it_end = node.TransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                nfa_transition_type_integer->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Type())));
                nfa_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(Regex::TransitionTypeString(transition.Type())));
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
    assert(dfa_graph.NodeCount() > 0);

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
            new Preprocessor::Body(Sint32(dfa_graph.NodeCount())));
    }

    // DEFINITION: a 'fast' transition is one that uses a lookup table to determine
    // the next state.  this gives constant-time transitions (neglecting buffer-filling).
    // a non-fast transition is one that runs through a list of transitions to determine
    // the next state.  this gives linear time complexity.
    //
    // _dfa_state_accept_handler_index[_dfa_state_count] -- gives the accept-handler-index
    // for this node, or _dfa_state_count if this is not an accept state
    //
    // _dfa_state_transition_offset[_dfa_state_count] -- gives the first index of the
    // contiguous set of transitions for this node.
    //
    // _dfa_state_transition_count[_dfa_state_count] -- gives the number of transitions for
    // this node (the number of contiguous transitions which apply to this node).
    //
    // _dfa_state_fast_transition_offset[_dfa_state_count] -- gives the offset into the
    // transition table for the transitions belonging to this state.
    //
    // _dfa_state_fast_transition_count[_dfa_state_count] -- gives the number of transitions
    // belonging to this state; the state's lookup table consists of the transitions having
    // indices in the range [_dfa_state_fast_transition_offset[i],
    // _dfa_state_fast_transition_offset[i]+_dfa_state_fast_transition_count[i]), noting
    // that the range is [ , ), i.e. the right endpoint is not included.
    //
    // _dfa_state_fast_transition_first_index[_dfa_state_count] -- if the state's transition
    // type is INPUT_ATOM, then this gives the lowest input_atom value in the transition
    // table.  it is byte-valued (i.e. the range is [0,255].  for any other transition type,
    // this value is not used.
    //
    // _dfa_state_fast_transition_type_integer[_dfa_state_count] -- this gives the
    // integer-valued transition type of the state, which can only be 0=INPUT_ATOM or
    // 2=CONDITIONAL.
    //
    // _dfa_state_fast_transition_type_name[_dfa_state_count] -- this gives the
    // text name of the transition type of the state, which can only be "INPUT_ATOM"
    // or "CONDITIONAL".
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
    //
    // _dfa_fast_transition_count -- gives the number of fast transitions in this DFA
    //
    // _dfa_fast_transition_target_node_index[_dfa_fast_transition_count] -- this is
    // the array of all lookup tables for 'fast' transitions.  the value is the index of
    // the 'next state' for when the transition in question is exercised.  the values
    // in _dfa_state_fast_transition_offset give offsets into this lookup table.
    {
        Preprocessor::ArraySymbol *dfa_state_accept_handler_index =
            symbol_table.DefineArraySymbol("_dfa_state_accept_handler_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_transition_offset =
            symbol_table.DefineArraySymbol("_dfa_state_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_transition_count =
            symbol_table.DefineArraySymbol("_dfa_state_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_fast_transition_offset =
            symbol_table.DefineArraySymbol("_dfa_state_fast_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_fast_transition_count =
            symbol_table.DefineArraySymbol("_dfa_state_fast_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_fast_transition_first_index =
            symbol_table.DefineArraySymbol("_dfa_state_fast_transition_first_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_fast_transition_type_integer =
            symbol_table.DefineArraySymbol("_dfa_state_fast_transition_type_integer", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_state_fast_transition_type_name =
            symbol_table.DefineArraySymbol("_dfa_state_fast_transition_type_name", FiLoc::ms_invalid);
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

        Preprocessor::ScalarSymbol *dfa_fast_transition_count =
            symbol_table.DefineScalarSymbol("_dfa_fast_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dfa_fast_transition_target_node_index =
            symbol_table.DefineArraySymbol("_dfa_fast_transition_target_node_index", FiLoc::ms_invalid);

        Sint32 total_transition_count = 0;
        Sint32 total_fast_transition_count = 0;

        for (Uint32 node_index = 0; node_index < dfa_graph.NodeCount(); ++node_index)
        {
            Graph::Node const &node = dfa_graph.GetNode(node_index);
            assert(node.HasData());
            Regex::NodeData const &node_data = dfa_graph.GetNode(node_index).DataAs<Regex::NodeData>();
            assert(node_data.m_dfa_accept_handler_index <= SINT32_UPPER_BOUND);
            Sint32 node_accept_handler_index = node_data.m_dfa_accept_handler_index;
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_fast_transition_offset = total_fast_transition_count;
            Sint32 node_transition_count = 0;
            Sint32 node_fast_transition_count = 0;

            // ensure that the transition type is the same for all transitions at this node
            // TODO: is this checked somewhere earlier?
            if (node.TransitionCount() > 0)
            {
                Graph::Transition const &first_transition = *node.TransitionSetBegin();
                for (Graph::TransitionSet::const_iterator it = node.TransitionSetBegin(),
                                                          it_end = node.TransitionSetEnd();
                     it != it_end;
                     ++it)
                {
                    Graph::Transition const &transition = *it;
                    switch (first_transition.Type())
                    {
                        case Regex::TT_INPUT_ATOM:
                        case Regex::TT_INPUT_ATOM_RANGE:
                            assert(transition.Type() == Regex::TT_INPUT_ATOM || transition.Type() == Regex::TT_INPUT_ATOM_RANGE);
                            break;

                        case Regex::TT_CONDITIONAL:
                            assert(transition.Type() == Regex::TT_CONDITIONAL);
                            assert(transition.Data(0) == first_transition.Data(0)); // make sure the bitmask is the same
                            break;

                        case Regex::TT_EPSILON:
                        default:
                            assert(false && "should not happen in DFA");
                            break;
                    }
                }
            }

            // construct lookup tables for the 'fast' transition symbols
            Uint32 input_atom_target_index[256];
            Uint32 input_atom_lowest = 256;
            Uint32 input_atom_highest = 0;
            Uint32 conditional_target_index[1<<Regex::CT_FLAG_COUNT];
            for (Uint32 i = 0; i < 256; ++i)
                input_atom_target_index[i] = dfa_graph.NodeCount(); // sentinel value is the node count (= _dfa_state_count)
            for (Uint32 flags = 0; flags < (1<<Regex::CT_FLAG_COUNT); ++flags)
                conditional_target_index[flags] = dfa_graph.NodeCount(); // sentinel value is the node count (= _dfa_state_count)

            // iterate through all the transitions and fill the symbols
            for (Graph::TransitionSet::const_iterator it = node.TransitionSetBegin(),
                                                      it_end = node.TransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                dfa_transition_type_integer->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Type())));
                dfa_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(Regex::TransitionTypeString(transition.Type())));
                dfa_transition_data_0->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data(0))));
                dfa_transition_data_1->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data(1))));
                dfa_transition_target_node_index->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.TargetIndex())));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(total_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;

                switch (transition.Type())
                {
                    case Regex::TT_INPUT_ATOM:
                        assert(transition.Data(0) < 256);
                        input_atom_target_index[transition.Data(0)] = transition.TargetIndex();
                        input_atom_lowest = min(input_atom_lowest, transition.Data(0));
                        input_atom_highest = max(input_atom_highest, transition.Data(0));
                        break;

                    case Regex::TT_INPUT_ATOM_RANGE:
                        assert(transition.Data(0) < transition.Data(1));
                        assert(transition.Data(1) < 256);
                        for (Uint32 i = transition.Data(0); i <= transition.Data(1); ++i)
                            input_atom_target_index[i] = transition.TargetIndex();
                        input_atom_lowest = min(input_atom_lowest, transition.Data(0));
                        input_atom_highest = max(input_atom_highest, transition.Data(1));
                        break;

                    case Regex::TT_CONDITIONAL:
                        assert(transition.Data(0) < (1<<Regex::CT_FLAG_COUNT));
                        assert(transition.Data(1) < (1<<Regex::CT_FLAG_COUNT));
                        for (Uint32 flags = 0; flags < (1<<Regex::CT_FLAG_COUNT); ++flags)
                        {
                            // data 0 is the bitmask, data 1 is the flag values
                            if ((flags & transition.Data(0)) == transition.Data(1))
                            {
                                assert(conditional_target_index[flags] == dfa_graph.NodeCount());
                                conditional_target_index[flags] = transition.TargetIndex();
                                assert(conditional_target_index[flags] != dfa_graph.NodeCount());
                            }
                        }
                        break;

                    case Regex::TT_EPSILON:
                    default:
                        assert(false && "should not happen in DFA");
                        break;
                }
            }

            dfa_state_accept_handler_index->AppendArrayElement(
                new Preprocessor::Body(node_accept_handler_index));
            dfa_state_transition_offset->AppendArrayElement(
                new Preprocessor::Body(node_transition_offset));
            dfa_state_transition_count->AppendArrayElement(
                new Preprocessor::Body(node_transition_count));

            dfa_state_fast_transition_offset->AppendArrayElement(
                new Preprocessor::Body(node_fast_transition_offset));
            // determine and set the fast transition count and first-index,
            // fill in dfa_fast_transition_target_node_index
            if (node.TransitionCount() > 0)
            {
                Graph::Transition const &first_transition = *node.TransitionSetBegin();

                Sint32 count = 0;
                Uint8 first_index = 0;
                if (first_transition.Type() == Regex::TT_CONDITIONAL)
                {
                    count = (1<<Regex::CT_FLAG_COUNT);
                    first_index = 0; // arbitrary, since it is unused
                    for (Uint32 flags = 0; flags < (1<<Regex::CT_FLAG_COUNT); ++flags)
                        dfa_fast_transition_target_node_index->AppendArrayElement(
                            new Preprocessor::Body(Sint32(conditional_target_index[flags])));
                }
                else
                {
                    assert(input_atom_lowest <= input_atom_highest);
                    assert(input_atom_highest < 256);
                    count = input_atom_highest - input_atom_lowest + 1;
                    first_index = input_atom_lowest;
                    for (Uint32 i = input_atom_lowest; i <= input_atom_highest; ++i)
                        dfa_fast_transition_target_node_index->AppendArrayElement(
                            new Preprocessor::Body(Sint32(input_atom_target_index[i])));
                }
                dfa_state_fast_transition_count->AppendArrayElement(
                    new Preprocessor::Body(count));
                dfa_state_fast_transition_first_index->AppendArrayElement(
                    new Preprocessor::Body(first_index));
                assert(node_fast_transition_count < SINT32_UPPER_BOUND-count);
                node_fast_transition_count += count;
                assert(total_fast_transition_count < SINT32_UPPER_BOUND-count);
                total_fast_transition_count += count;

                TransitionType fast_transition_type = first_transition.Type() == Regex::TT_CONDITIONAL ? Regex::TT_CONDITIONAL : Regex::TT_INPUT_ATOM;
                dfa_state_fast_transition_type_integer->AppendArrayElement(
                    new Preprocessor::Body(Sint32(fast_transition_type)));
                dfa_state_fast_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(Regex::TransitionTypeString(fast_transition_type)));
            }
            else
            {
                dfa_state_fast_transition_count->AppendArrayElement(
                    new Preprocessor::Body(0));
                dfa_state_fast_transition_first_index->AppendArrayElement(
                    new Preprocessor::Body(0));
                dfa_state_fast_transition_type_integer->AppendArrayElement(
                    new Preprocessor::Body(Sint32(Regex::TT_INPUT_ATOM))); // arbitrary
                dfa_state_fast_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(Regex::TransitionTypeString(Regex::TT_INPUT_ATOM))); // same as above line
            }

            dfa_state_description->AppendArrayElement(
                new Preprocessor::Body(node_data.AsText(node_index)));
        }

        dfa_transition_count->SetScalarBody(
            new Preprocessor::Body(total_transition_count));
        dfa_fast_transition_count->SetScalarBody(
            new Preprocessor::Body(total_fast_transition_count));
    }
}

void PopulateAcceptHandlerCodeArraySymbol (Rule const &rule, string const &target_id, Preprocessor::ArraySymbol *accept_handler_code)
{
    assert(accept_handler_code != NULL);
    CommonLang::RuleHandler const *rule_handler = rule.m_rule_handler_map->Element(target_id);
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
