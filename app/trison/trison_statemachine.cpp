// ///////////////////////////////////////////////////////////////////////////
// trison_statemachine.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_statemachine.hpp"

#include <iomanip>
#include <sstream>

#include "trison_ast.hpp"
#include "trison_options.hpp"
#include "trison_message.hpp"

namespace Trison {

StateMachine::StateMachine (Grammar const *grammar)
    :
    m_grammar(grammar)
{
    assert(m_grammar != NULL);

    // this is 1 because transition 0 is the special "NULL" transition.
    m_state_transition_count = 1;

    m_shift_reduce_conflict_count = 0;
    m_reduce_reduce_conflict_count = 0;

    // put all the necessary data into containers that are indexed appropriately
    GeneratePrecedenceMap();
    GenerateNonterminalMap();
    GenerateTerminalMap();
    GenerateReductionRuleVector();
}

StateMachine::~StateMachine ()
{
    // the nonterminals are owned by Grammar, so we don't need to delete them.
    // we own the terminals, so we must delete them.
    for_each(
        m_terminal_map.begin(),
        m_terminal_map.end(),
        DeletePairSecondFunctor<pair<string, Terminal *> >());
    // the rules are owned by Grammar, so we don't need to delete them.
    // we own the states, so we must delete them.
    for_each(
        m_state_map.begin(),
        m_state_map.end(),
        DeletePairSecondFunctor<pair<StateIdentifier, State *> >());
}

Rule const *StateMachine::GetRule (Uint32 rule_index) const
{
    assert(rule_index < m_rule_vector.size());
    return m_rule_vector[rule_index];
}

Uint32 StateMachine::GetStateIndex (StateIdentifier const &state_identifier) const
{
    StateMap::const_iterator it = m_state_map.find(state_identifier);

    {
    assert(it != m_state_map.end() && "if this assert fails, uncomment the stuff using Uint32(-1), and delete this assert");
    }
//     {
//     if (it == m_state_map.end())
//         return Uint32(-1);
//     }

    State const *state = it->second;
    assert(state != NULL);
    Uint32 state_index = state->GetStateIndex();
    assert(state_index < m_state_vector.size());
    return state_index;
}

void StateMachine::Generate ()
{
    // generate the states and state transitions
    GenerateStatesFromTheStartNonterminal();
    // generate the preprocessor so we're ready to print the header/implementation files
    GeneratePreprocessor();

    // if there were conflicts, emit a warning
    if (m_shift_reduce_conflict_count > 0)
    {
        ostringstream buffer;
        buffer << m_shift_reduce_conflict_count << " shift/reduce conflict(s)";
        EmitConflictWarning(buffer.str());
    }

    if (m_reduce_reduce_conflict_count > 0)
    {
        ostringstream buffer;
        buffer << m_reduce_reduce_conflict_count << " reduce/reduce conflict(s)";
        EmitConflictWarning(buffer.str());
    }
}

void StateMachine::PrintStateMachineFile (ostream &stream) const
{
    assert(!m_rule_vector.empty());
    assert(!m_state_vector.empty());

    // TODO: print the precedence directives

    // print a numbered list of the grammar's rules.

    stream << "//////////////////////////////////////////////////////////////////////////////\n"
           << "// GRAMMAR\n"
           << "//////////////////////////////////////////////////////////////////////////////\n"
           << endl;
    for (RuleVector::const_iterator it = m_rule_vector.begin(),
                                 it_end = m_rule_vector.end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        stream << Tabs(1);
        rule->PrettyPrint(stream);
        stream << endl;
    }
    stream << endl;

    // print the generated state machine.
    stream << "//////////////////////////////////////////////////////////////////////////////\n"
           << "// STATE MACHINE - " << m_state_vector.size() << " STATES\n"
           << "//////////////////////////////////////////////////////////////////////////////\n"
           << endl;
    for (StateVector::const_iterator it = m_state_vector.begin(),
                                  it_end = m_state_vector.end();
         it != it_end;
         ++it)
    {
        State const *state = *it;
        assert(state != NULL);
        state->PrettyPrint(stream, this);
        stream << endl;
    }
}

void StateMachine::PrintHeaderFile (ostream &stream)
{
    assert(!m_rule_vector.empty());
    assert(!m_state_vector.empty());

    stream <<
        m_preprocessor.ProcessString(
            ms_header_file_template,
            GetOptions()->GetHeaderFilename());
}

void StateMachine::PrintImplementationFile (ostream &stream)
{
    assert(!m_rule_vector.empty());
    assert(!m_state_vector.empty());

    stream <<
        m_preprocessor.ProcessString(
            ms_implementation_file_template,
            GetOptions()->GetImplementationFilename());
}

void StateMachine::GetStateTransitionIdentifiersFromRulePhase (
    RulePhase const &rule_phase,
    map<Terminal *, Transition> *terminal_transition_map,
    map<Nonterminal *, Transition> *nonterminal_transition_map,
    Transition *default_transition) const
{
    assert(rule_phase.m_rule_index < m_rule_vector.size());
    assert(terminal_transition_map != NULL);
    assert(nonterminal_transition_map != NULL);
    assert(default_transition != NULL);

    // if there are no tokens to the right in the current phase, this is
    // the last rule phase in its rule, so add it to the default state
    // identifier, and return.
    if (GetIsLastPhaseInRule(rule_phase))
    {
        assert(rule_phase.m_rule_index < m_rule_vector.size());
        Rule const *rule = m_rule_vector[rule_phase.m_rule_index];
        assert(rule != NULL);
        if (rule->GetOwnerNonterminal() == m_grammar->GetStartNonterminal())
            default_transition->SetTransitionAction(TA_REDUCE_AND_ACCEPT_USING_RULE);
        else
            default_transition->SetTransitionAction(TA_REDUCE_USING_RULE);
        default_transition->AddRulePhase(rule_phase);
        return;
    }

    // get the token name
    RuleToken const *rule_token = GetRuleTokenToRight(rule_phase);
    assert(rule_token != NULL);
    string const &token_name = rule_token->GetTokenIdentifier()->GetText();

    // if the token name corresponds to a terminal,
    TerminalMap::const_iterator terminal_it;
    NonterminalMap::const_iterator nonterminal_it;
    if ((terminal_it = m_terminal_map.find(token_name)) != m_terminal_map.end())
    {
        Terminal *terminal = terminal_it->second;
        assert(terminal != NULL);
        // add it to the terminal rule phase set
        (*terminal_transition_map)[terminal].SetTransitionAction(TA_SHIFT_AND_PUSH_STATE);
        (*terminal_transition_map)[terminal].AddRulePhase(rule_phase + 1);
    }
    // if the token name corresponds to a nonterminal,
    else if ((nonterminal_it = m_nonterminal_map.find(token_name)) != m_nonterminal_map.end())
    {
        Nonterminal *nonterminal = nonterminal_it->second;
        assert(nonterminal != NULL);
        // check if it's already in the nonterminal's transition identifier.  if not, add
        // it and traverse it.  this check is necessary to avoid infinite recursion
        // due to recursive nonterminals.
        Transition &nonterminal_transition = (*nonterminal_transition_map)[nonterminal];
        nonterminal_transition.SetTransitionAction(TA_PUSH_STATE);
        StateIdentifier &nonterminal_transition_identifier = nonterminal_transition.GetTransitionIdentifier();
        if (nonterminal_transition_identifier.find(rule_phase + 1) == nonterminal_transition_identifier.end())
        {
            nonterminal_transition.AddRulePhase(rule_phase + 1);
            for (RuleList::const_iterator rule_it = nonterminal->GetRuleListBegin(),
                                         rule_it_end = nonterminal->GetRuleListEnd();
                 rule_it != rule_it_end;
                 ++rule_it)
            {
                Rule *rule = *rule_it;
                assert(rule != NULL);
                RulePhase spawned_rule_phase(rule->GetIndex(), 0);

                GetStateTransitionIdentifiersFromRulePhase(
                    spawned_rule_phase,
                    terminal_transition_map,
                    nonterminal_transition_map,
                    default_transition);

                if (!GetIsLastPhaseInRule(spawned_rule_phase))
                {
                    RuleToken const *spawned_rule_token = GetRuleTokenToRight(spawned_rule_phase);
                    assert(spawned_rule_token != NULL);
                    string const &spawned_token_name = spawned_rule_token->GetTokenIdentifier()->GetText();
                    NonterminalMap::const_iterator spawned_nonterminal_it =
                        m_nonterminal_map.find(spawned_token_name);
                    if (spawned_nonterminal_it != m_nonterminal_map.end())
                    {
                        Nonterminal *spawned_nonterminal = spawned_nonterminal_it->second;
                        Transition &spawned_nonterminal_transition =
                            (*nonterminal_transition_map)[spawned_nonterminal];
                        spawned_nonterminal_transition.AddRulePhase(spawned_rule_phase + 1);
                    }
                }
            }
        }
    }
    // otherwise, this is an unidentified token identifier
    else
        EmitError(
            rule_token->GetTokenIdentifier()->GetFileLocation(),
            "undeclared token \"" + token_name + "\"");
}

void StateMachine::GetRulePhasesWithNonterminalOnLeft (
    Nonterminal const *const nonterminal,
    set<RulePhase> *const rule_phase_set) const
{
    set<RulePhase> visited_rule_phase_set;
    GetRulePhasesWithNonterminalOnLeft(
        nonterminal,
        rule_phase_set,
        &visited_rule_phase_set);
}

void StateMachine::GetRulePhasesWithNonterminalOnLeft (
    Nonterminal const *const nonterminal,
    set<RulePhase> *const rule_phase_set,
    set<RulePhase> *const visited_rule_phase_set) const
{
    assert(nonterminal != NULL);
    assert(rule_phase_set != NULL);
    assert(visited_rule_phase_set != NULL);

    for (Uint32 rule_index = 0, rule_count = m_rule_vector.size();
         rule_index < rule_count;
         ++rule_index)
    {
        Rule const *rule = m_rule_vector[rule_index];
        assert(rule != NULL);
        for (Uint32 rule_token_index = 0, rule_token_count = rule->GetRuleTokenCount();
             rule_token_index < rule_token_count;
             ++rule_token_index)
        {
            RuleToken const *rule_token = rule->GetRuleToken(rule_token_index);
            assert(rule_token != NULL);
            TokenIdentifier const *rule_token_identifier = rule_token->GetTokenIdentifier();
            assert(rule_token_identifier != NULL);
            string const &rule_token_name = rule_token_identifier->GetText();
            NonterminalMap::const_iterator nonterminal_it = m_nonterminal_map.find(rule_token_name);
            if (nonterminal_it != m_nonterminal_map.end())
            {
                Nonterminal const *rule_token_nonterminal = nonterminal_it->second;
                assert(rule_token_nonterminal != NULL);
                if (rule_token_nonterminal == nonterminal)
                {
                    // we don't want to add a rule phase which is the last
                    // phase in a rule, because that gives no lookahead.
                    // if it is the last rule phase in the rule, call this function
                    // recursively on the owner nonterminal for the current rule.
                    RulePhase matching_rule_phase(rule_index, rule_token_index + 1);
                    if (visited_rule_phase_set->find(matching_rule_phase) ==
                        visited_rule_phase_set->end())
                    {
                        visited_rule_phase_set->insert(matching_rule_phase);
                        if (GetIsLastPhaseInRule(matching_rule_phase))
                            GetRulePhasesWithNonterminalOnLeft(
                                rule->GetOwnerNonterminal(),
                                rule_phase_set,
                                visited_rule_phase_set);
                        else
                            rule_phase_set->insert(matching_rule_phase);
                    }
                }
            }
        }
    }
}

bool StateMachine::GetIsLastPhaseInRule (RulePhase const &rule_phase) const
{
    assert(rule_phase.m_rule_index < m_rule_vector.size());
    assert(rule_phase.m_phase <= m_rule_vector[rule_phase.m_rule_index]->GetRuleTokenCount());
    return rule_phase.m_phase == m_rule_vector[rule_phase.m_rule_index]->GetRuleTokenCount();
}

RuleToken const *StateMachine::GetRuleTokenToLeft (RulePhase const &rule_phase) const
{
    assert(rule_phase.m_rule_index < m_rule_vector.size());
    return m_rule_vector[rule_phase.m_rule_index]->GetRuleTokenToLeft(rule_phase.m_phase);
}

RuleToken const *StateMachine::GetRuleTokenToRight (RulePhase const &rule_phase) const
{
    assert(rule_phase.m_rule_index < m_rule_vector.size());
    return m_rule_vector[rule_phase.m_rule_index]->GetRuleTokenToRight(rule_phase.m_phase);
}

Associativity StateMachine::GetRuleAssociativity (Uint32 rule_index) const
{
    assert(rule_index < m_rule_vector.size());
    return m_rule_vector[rule_index]->GetAssociativity();
}

Uint32 StateMachine::GetRulePrecedence (Uint32 rule_index) const
{
    assert(rule_index < m_rule_vector.size());
    AstCommon::Identifier const *precedence_directive_identifier =
        m_rule_vector[rule_index]->GetPrecedenceDirectiveIdentifier();
    if (precedence_directive_identifier != NULL)
    {
        string const &precedence_directive_name = precedence_directive_identifier->GetText();
        PrecedenceMap::const_iterator it = m_precedence_map.find(precedence_directive_name);
        if (it == m_precedence_map.end())
        {
            EmitError(
                precedence_directive_identifier->GetFileLocation(),
                "undeclared precedence directive identifier \"" + precedence_directive_name + "\"");
            return 0;
        }
        else
        {
            PrecedenceDirective const *precedence_directive = it->second;
            assert(precedence_directive != NULL);
            return precedence_directive->GetPrecedenceLevel();
        }
    }
    else
        return 0;
}

bool StateMachine::GetIsABinaryOperation (Uint32 rule_index) const
{
    assert(rule_index < m_rule_vector.size());
    assert(m_rule_vector[rule_index] != NULL);
    return m_rule_vector[rule_index]->GetIsABinaryOperation();
}

string const &StateMachine::GetAssignedType (string const &token_name) const
{
    assert(!token_name.empty());

    TerminalMap::const_iterator terminal_it;
    NonterminalMap::const_iterator nonterminal_it;
    if ((terminal_it = m_terminal_map.find(token_name)) != m_terminal_map.end())
    {
        Terminal const *terminal = terminal_it->second;
        assert(terminal != NULL);
        AstCommon::String const *assigned_type = terminal->GetAssignedType();
        if (assigned_type != NULL)
            return assigned_type->GetText();
        else
            return m_grammar->GetParserDirectiveSet()->GetDirectiveValueText(PDT_BASE_ASSIGNED_TYPE);
    }
    else if ((nonterminal_it = m_nonterminal_map.find(token_name)) != m_nonterminal_map.end())
    {
        Nonterminal const *nonterminal = nonterminal_it->second;
        assert(nonterminal != NULL);
        AstCommon::String const *assigned_variable_type = nonterminal->GetAssignedVariableType();
        if (assigned_variable_type != NULL)
            return assigned_variable_type->GetText();
        else
            return m_grammar->GetParserDirectiveSet()->GetDirectiveValueText(PDT_BASE_ASSIGNED_TYPE);
    }
    return gs_empty_string;
}

bool StateMachine::GetParserDirectiveShouldHaveLineDirective (
    ParserDirectiveType parser_directive_type) const
{
    switch (parser_directive_type)
    {
        case PDT_HEADER_FILE_TOP:
        case PDT_HEADER_FILE_BOTTOM:
        case PDT_CLASS_INHERITANCE:
        case PDT_SUPERCLASS_AND_MEMBER_CONSTRUCTORS:
        case PDT_CONSTRUCTOR_ACTIONS:
        case PDT_DESTRUCTOR_ACTIONS:
        case PDT_CLASS_METHODS_AND_MEMBERS:
        case PDT_START_OF_PARSE_METHOD_ACTIONS:
        case PDT_END_OF_PARSE_METHOD_ACTIONS:
        case PDT_THROW_AWAY_TOKEN_ACTIONS:
        case PDT_IMPLEMENTATION_FILE_TOP:
        case PDT_IMPLEMENTATION_FILE_BOTTOM:
            return true;

        case PDT_CLASS_NAME:
        case PDT_FORCE_VIRTUAL_DESTRUCTOR:
        case PDT_PARSE_METHOD_ACCESS:
        case PDT_BASE_ASSIGNED_TYPE:
        case PDT_BASE_ASSIGNED_TYPE_SENTINEL:
        case PDT_CUSTOM_CAST:
            return false;

        default:
            assert(false && "Invalid ParserDirectiveType");
            return false;
    }
}

void StateMachine::GeneratePrecedenceMap ()
{
//     cerr << "*** StateMachine::GeneratePrecedenceMap();" << endl;
    for (PrecedenceDirectiveList::const_iterator it = m_grammar->GetPrecedenceDirectiveList()->begin(),
                                                 it_end = m_grammar->GetPrecedenceDirectiveList()->end();
         it != it_end;
         ++it)
    {
        PrecedenceDirective const *precedence_directive = *it;
        assert(precedence_directive != NULL);
        // make sure each directive's identifier isn't already in the map
        string const &precedence_directive_name = precedence_directive->GetIdentifier()->GetText();
        if (m_precedence_map.find(precedence_directive_name) != m_precedence_map.end())
            EmitError(
                precedence_directive->GetFileLocation(),
                "duplicate precedence directive identifier \"" + precedence_directive_name + "\" "
                "(first encountered at " +
                m_precedence_map[precedence_directive_name]->GetFileLocation().GetAsString() + ")");
        else
            m_precedence_map[precedence_directive_name] = precedence_directive;
    }
}

void StateMachine::GenerateNonterminalMap ()
{
    // go through the nonterminal list and put each in a map, indexed by name
    for (NonterminalList::iterator it = m_grammar->GetNonterminalList()->begin(),
                                   it_end = m_grammar->GetNonterminalList()->end();
         it != it_end;
         ++it)
    {
        Nonterminal *nonterminal = *it;
        assert(nonterminal != NULL);
        string const &nonterminal_name = nonterminal->GetIdentifier()->GetText();
        if (m_nonterminal_map.find(nonterminal_name) != m_nonterminal_map.end())
            EmitError(
                nonterminal->GetFileLocation(),
                "duplicate nonterminal name \"" + nonterminal_name + "\" "
                "(first encountered at " +
                m_nonterminal_map[nonterminal_name]->GetFileLocation().GetAsString() + ")");
        else
            m_nonterminal_map[nonterminal_name] = nonterminal;
    }
}

void StateMachine::GenerateTerminalMap ()
{
    // there is always a special end terminal which signals the end
    // of the input (be it file input or string input or whatever).
    {
        string end_name("END_");
        TokenIdentifierIdentifier *end_terminal_identifier =
            new TokenIdentifierIdentifier(end_name, FileLocation::ms_invalid);
        m_terminal_map[end_name] =
            new Terminal(end_terminal_identifier, NULL, true);
    }

    // there is always a special %error terminal which is used in error handling
    {
        string error_name("%error");
        TokenIdentifierIdentifier *error_terminal_identifier =
            new TokenIdentifierIdentifier(error_name, FileLocation::ms_invalid);
        m_error_terminal = new Terminal(error_terminal_identifier, NULL, true);
        m_terminal_map[error_name] = m_error_terminal;

    }

    // go through the token directives, iterating through each token, and create
    // a Terminal to put in the terminal map, indexed by name.
    for (TokenDirectiveList::const_iterator it = m_grammar->GetTokenDirectiveList()->begin(),
                                            it_end = m_grammar->GetTokenDirectiveList()->end();
         it != it_end;
         ++it)
    {
        TokenDirective const *token_directive = *it;
        assert(token_directive != NULL);
        AstCommon::String const *assigned_variable_type = token_directive->GetAssignedType();

        for (TokenIdentifierList::const_iterator
                 token_it = token_directive->GetTokenIdentifierBegin(),
                 token_it_end = token_directive->GetTokenIdentifierEnd();
             token_it != token_it_end;
             ++token_it)
        {
            TokenIdentifier const *token_identifier = *token_it;
            assert(token_identifier != NULL);
            string const &terminal_name = token_identifier->GetText();

            if (m_terminal_map.find(terminal_name) != m_terminal_map.end())
                EmitError(
                    token_identifier->GetFileLocation(),
                    "duplicate token name \"" + terminal_name + "\" "
                    "(first encountered at " +
                    m_terminal_map[terminal_name]->GetFileLocation().GetAsString() + ")");
            else
                m_terminal_map[terminal_name] =
                    new Terminal(token_identifier, assigned_variable_type, false);
        }
    }
}

void StateMachine::GenerateReductionRuleVector ()
{
    // record each of the start nonterminal's rules in the rule vector,
    // in the other they were parsed.
    GenerateReductionRuleVectorForNonterminal(m_grammar->GetStartNonterminal());
    // go through the nonterminal list and record each rule in the
    // rule vector, in the order they were parsed.
    for (NonterminalList::iterator it = m_grammar->GetNonterminalList()->begin(),
                                   it_end = m_grammar->GetNonterminalList()->end();
         it != it_end;
         ++it)
    {
        Nonterminal *nonterminal = *it;
        assert(nonterminal != NULL);
        GenerateReductionRuleVectorForNonterminal(nonterminal);
    }
}

void StateMachine::GenerateReductionRuleVectorForNonterminal (
    Nonterminal *nonterminal)
{
    assert(nonterminal != NULL);
    for (RuleList::const_iterator rule_it = nonterminal->GetRuleListBegin(),
                                  rule_it_end = nonterminal->GetRuleListEnd();
         rule_it != rule_it_end;
         ++rule_it)
    {
        Rule *rule = *rule_it;
        assert(rule != NULL);
        rule->SetIndex(m_rule_vector.size());

        // for each rule, make sure that %error doesn't occur twice in a row
        bool last_rule_token_was_error_token = false;
        for (Uint32 rule_token_index = 0, rule_token_count = rule->GetRuleTokenCount();
             rule_token_index < rule_token_count;
             ++rule_token_index)
        {
            RuleToken const *rule_token = rule->GetRuleToken(rule_token_index);
            assert(rule_token != NULL);
            bool this_rule_token_is_error_token =
                rule_token->GetTokenIdentifier()->GetText() == "%error";
            if (this_rule_token_is_error_token && last_rule_token_was_error_token)
                EmitError(rule_token->GetFileLocation(), "may not have two %error tokens in a row");
            last_rule_token_was_error_token = this_rule_token_is_error_token;
        }

        m_rule_vector.push_back(rule);
    }
}

void StateMachine::GenerateStatesFromTheStartNonterminal ()
{
    // generate the state identifier for state 0 (the starting state),
    // using the first phase for each of the start nonterminal's rules.
    Nonterminal const *start_nonterminal = m_grammar->GetStartNonterminal();
    assert(start_nonterminal != NULL);
    StateIdentifier starting_state_identifier;
    for (Uint32 rule_index = 0, rule_count = m_rule_vector.size();
         rule_index < rule_count &&
         m_rule_vector[rule_index]->GetOwnerNonterminal() == start_nonterminal;
         ++rule_index)
    {
        starting_state_identifier.insert(RulePhase(rule_index, 0));
    }
    // generate all the states in the order in which they were identified.
    assert(m_transition_generation_queue.empty());
    m_transition_generation_queue.push_back(starting_state_identifier);
    while (!m_transition_generation_queue.empty())
    {
        GenerateState(*m_transition_generation_queue.begin());
        m_transition_generation_queue.pop_front();
    }
}

void StateMachine::GenerateState (StateIdentifier const &state_identifier)
{
    assert(!state_identifier.empty());

    // check if the state exists already.  if so, just return.
    if (m_state_map.find(state_identifier) != m_state_map.end())
        return;

    // create the state and add it to the state map
    State *state = new State(m_state_vector.size());
    state->SetStateIdentifier(state_identifier);
    m_state_map[state_identifier] = state;
    m_state_vector.push_back(state);

    // determine which terminals and nonterminals this state can transition
    // to, as well as which states each one transitions to.  also determine
    // which rules are at the last rule phase and require a default action.
    map<Terminal *, Transition> terminal_transition_map;
    map<Nonterminal *, Transition> nonterminal_transition_map;
    Transition default_transition;
    for (StateIdentifier::const_iterator it = state->GetStateIdentifier().begin(),
                                         it_end = state->GetStateIdentifier().end();
         it != it_end;
         ++it)
    {
        RulePhase const &rule_phase = *it;
        GetStateTransitionIdentifiersFromRulePhase(
            rule_phase,
            &terminal_transition_map,
            &nonterminal_transition_map,
            &default_transition);
    }

    // TODO: add in check that all rule-tokens-to-the-left are the same?

    AddErrorCollapsingTransitions(
        state,
        &terminal_transition_map);
    ResolveReduceReduceAndErrorConflicts(
        state,
        state_identifier,
        &terminal_transition_map,
        &nonterminal_transition_map,
        &default_transition);
    ResolveShiftReduceConflicts(
        state_identifier,
        &terminal_transition_map,
        &nonterminal_transition_map,
        &default_transition);
    AddTransitions(
        state,
        &terminal_transition_map,
        &nonterminal_transition_map,
        &default_transition);
    AddSpawnedRulePhases(
        state,
        &nonterminal_transition_map);
}

void StateMachine::AddErrorCollapsingTransitions (
    State *const state,
    map<Terminal *, Transition> *const terminal_transition_map)
{
    assert(state != NULL);
    assert(terminal_transition_map != NULL);

    // if %error exists to the left of any of this state's rule phases,
    // add an %error terminal transition action (to throw it away)
    for (StateIdentifier::const_iterator it = state->GetStateIdentifier().begin(),
                                         it_end = state->GetStateIdentifier().end();
         it != it_end;
         ++it)
    {
        RulePhase const &rule_phase = *it;
        // only operate on rule phases that actually have tokens to the left
        if (rule_phase.m_phase > 0)
        {
            RuleToken const *rule_token = GetRuleTokenToLeft(rule_phase);
            assert(rule_token != NULL);
            if (rule_token->GetTokenIdentifier()->GetText() == "%error")
            {
                (*terminal_transition_map)[m_error_terminal].SetTransitionAction(
                    TA_THROW_AWAY_LOOKAHEAD_TOKEN);
            }
        }
    }
}

void StateMachine::ResolveReduceReduceAndErrorConflicts (
    State *state,
    StateIdentifier const &state_identifier,
    map<Terminal *, Transition> *const terminal_transition_map,
    map<Nonterminal *, Transition> *const nonterminal_transition_map,
    Transition *const default_transition)
{
    assert(state != NULL);
    assert(!state_identifier.empty());
    assert(terminal_transition_map != NULL);
    assert(nonterminal_transition_map != NULL);
    assert(default_transition != NULL);

    bool state_accepts_error_token =
        default_transition->GetTransitionIdentifier().size() > 0 &&
        terminal_transition_map->find(m_error_terminal) != terminal_transition_map->end();
    bool add_reduce_lookahead_transitions =
        default_transition->GetTransitionIdentifier().size() > 1 ||
        state_accepts_error_token;

    // if there is more than one rule phase in the default state identifier,
    // or any of the terminal state identifiers, there are reduce/reduce
    // conflicts, so look for terminals further ahead in appropriate rules
    // to see how the conflict can be resolved.
    //
    // also, if there is a default action and an %error action (both of
    // which are catch-alls), there is a conflict which can be resolved
    // in the same manner as a reduce/reduce.
    if (add_reduce_lookahead_transitions)
    {
        map<Terminal *, set<RulePhase> > resolution_rule_phase_map;
        for (StateIdentifier::const_iterator
                it = default_transition->GetTransitionIdentifier().begin(),
                it_end = default_transition->GetTransitionIdentifier().end();
             it != it_end;
             ++it)
        {
            RulePhase const &rule_phase = *it;
            Rule const *rule = GetRule(rule_phase.m_rule_index);
            assert(rule != NULL);
            Nonterminal const *rule_owner_nonterminal = rule->GetOwnerNonterminal();
            assert(rule_owner_nonterminal != NULL);

            set<RulePhase> rule_phases_with_nonterminal_on_left;
            GetRulePhasesWithNonterminalOnLeft(
                rule_owner_nonterminal,
                &rule_phases_with_nonterminal_on_left);

            for (set<RulePhase>::iterator
                     rule_phase_it = rule_phases_with_nonterminal_on_left.begin(),
                     rule_phase_it_end = rule_phases_with_nonterminal_on_left.end();
                 rule_phase_it != rule_phase_it_end;
                 ++rule_phase_it)
            {
                RulePhase const &matched_rule_phase = *rule_phase_it;

                map<Terminal *, Transition> rule_phase_terminal_transition_map;
                map<Nonterminal *, Transition> dummy_nonterminal_transition_map;
                Transition dummy_default_transition;
                GetStateTransitionIdentifiersFromRulePhase(
                    matched_rule_phase,
                    &rule_phase_terminal_transition_map,
                    &dummy_nonterminal_transition_map,
                    &dummy_default_transition);

                for (map<Terminal *, Transition>::iterator
                         terminal_it = rule_phase_terminal_transition_map.begin(),
                         terminal_it_end = rule_phase_terminal_transition_map.end();
                     terminal_it != terminal_it_end;
                     ++terminal_it)
                {
                    Terminal *terminal = terminal_it->first;
                    assert(terminal != NULL);
                    resolution_rule_phase_map[terminal].insert(rule_phase);
                }
            }
        }

        StateIdentifier &default_transition_identifier =
            default_transition->GetTransitionIdentifier();
        Transition eliminated_default_transition;
        eliminated_default_transition.SetTransitionAction(TA_REDUCE_USING_RULE);
        for (map<Terminal *, set<RulePhase> >::iterator
                 it = resolution_rule_phase_map.begin(),
                 it_end = resolution_rule_phase_map.end();
             it != it_end;
             ++it)
        {
            Terminal *terminal = it->first;
            assert(terminal != NULL);
            set<RulePhase> &rule_phase_set = it->second;
            assert(!rule_phase_set.empty());

            // if a terminal applies to more than one default rule,
            // it is a real reduce/reduce conflict.
            // is this conflict resolveable in a non-arbitrary way?
            RulePhase const &first_rule_phase = *rule_phase_set.begin();
            rule_phase_set.erase(first_rule_phase);
            for (set<RulePhase>::iterator rule_phase_it = rule_phase_set.begin(),
                                          rule_phase_it_end = rule_phase_set.end();
                 rule_phase_it != rule_phase_it_end;
                 ++rule_phase_it)
            {
                RulePhase const &conflicted_rule = *rule_phase_it;
                default_transition_identifier.erase(conflicted_rule);
                eliminated_default_transition.AddRulePhase(conflicted_rule);
            }
            rule_phase_set.insert(first_rule_phase);

            // we are about to add terminal actions, but before doing
            // that, we must check for shift/reduce conflicts
            if (terminal_transition_map->find(terminal) != terminal_transition_map->end())
            {
                // this if-statement is provisional (but the contents should
                // remain regardless).  pending lots of testing, this comment
                // can go away eventually.
                if (!state_accepts_error_token)
                {
                    state->SetConflictedTerminalTransition(
                        terminal,
                        (*terminal_transition_map)[terminal]);
                    terminal_transition_map->erase(terminal);
                    ++m_shift_reduce_conflict_count;
                }
                continue;
            }

            // add a terminal transition
            (*terminal_transition_map)[terminal].SetTransitionAction(TA_REDUCE_USING_RULE);
            (*terminal_transition_map)[terminal].AddRulePhase(first_rule_phase);

            // only remove the transition from the default rule if either there is
            // still more than one default transition rule, or if this state
            // accepts an error token (which would be a shift/reduce conflict
            // due to both default and %error being catch-alls).
            if (default_transition->GetTransitionIdentifier().size() > 1 ||
                state_accepts_error_token)
            {
                default_transition->GetTransitionIdentifier().erase(first_rule_phase);
            }
        }

        if (eliminated_default_transition.GetIsNontrivial())
        {
            state->SetConflictedDefaultTransition(eliminated_default_transition);
            ++m_reduce_reduce_conflict_count;
        }
    }
}

void StateMachine::ResolveShiftReduceConflicts (
    StateIdentifier const &state_identifier,
    map<Terminal *, Transition> *const terminal_transition_map,
    map<Nonterminal *, Transition> *const nonterminal_transition_map,
    Transition *const default_transition)
{
    assert(!state_identifier.empty());
    assert(terminal_transition_map != NULL);
    assert(nonterminal_transition_map != NULL);
    assert(default_transition != NULL);

    // check for shift/reduce conflicts (which can only happen if
    // there is a default state transition).
    if (!default_transition->GetTransitionIdentifier().empty())
    {
        // this assertion exists because if there was more than one rule in
        // the default action (reduce/reduce conflict), it had to have been
        // resolved by now.
        assert(default_transition->GetTransitionIdentifier().size() == 1);
        RulePhase const &default_rule_phase =
            *default_transition->GetTransitionIdentifier().begin();

        bool skip_rest;
        for (map<Terminal *, Transition>::iterator
                 it = terminal_transition_map->begin(),
                 it_end = terminal_transition_map->end();
             it != it_end;
             ++it)
        {
            skip_rest = false;

            Transition &terminal_transition = it->second;
            StateIdentifier &terminal_state_identifier =
                terminal_transition.GetTransitionIdentifier();
            RulePhase const &first_terminal_rule_phase =
                *terminal_state_identifier.begin();
            assert(terminal_transition.GetIsNontrivial());

            // if there are more than one rule phases for this terminal, we
            // must make sure their precedences are either both above, both
            // below or both equal to that of the default rule's precedence.
            Uint32 first_terminal_rule_precedence =
                GetRulePrecedence(first_terminal_rule_phase.m_rule_index);
            Uint32 default_rule_precedence =
                GetRulePrecedence(default_rule_phase.m_rule_index);
            if (terminal_state_identifier.size() > 1)
            {
                for (StateIdentifier::const_iterator rule_phase_it = terminal_state_identifier.begin(),
                                                  rule_phase_it_end = terminal_state_identifier.end();
                     rule_phase_it != rule_phase_it_end;
                     ++rule_phase_it)
                {
                    Uint32 current_rule_index = (*rule_phase_it).m_rule_index;
                    Uint32 current_rule_precedence =
                        GetRulePrecedence(current_rule_index);
                    if (current_rule_precedence < default_rule_precedence !=
                        first_terminal_rule_precedence < default_rule_precedence
                        ||
                        current_rule_precedence > default_rule_precedence !=
                        first_terminal_rule_precedence > default_rule_precedence)
                    {
                        // is this conflict resolveable in a meaningful way?
                        ostringstream buffer;
                        buffer << "conflict in rule precedence between rules \""
                               << first_terminal_rule_phase.m_rule_index << ", "
                               << default_rule_phase.m_rule_index << " and "
                               << current_rule_index;
                        EmitError(FileLocation(GetOptions()->GetInputFilename()), buffer.str());
                        skip_rest = true;
                    }
                }

                if (skip_rest)
                    continue;

                // at this point, all the rules matched by the terminal are guaranteed
                // to all have lower precedence, all have equal precedence, or all
                // have higher precedence than the default.
            }

            // if this terminal's rules have lower precedences than the
            // default rule, eliminate it (eliminate the terminal completely).
            if (first_terminal_rule_precedence < default_rule_precedence)
                terminal_state_identifier.clear();
            // if the precedences are equal, then rely on associativity to resolve
            // the shift/reduce conflict -- but associativity can only resolve
            // conflicts between binary operations.
            else if (first_terminal_rule_precedence == default_rule_precedence &&
                     GetIsABinaryOperation(first_terminal_rule_phase.m_rule_index) &&
                     GetIsABinaryOperation(default_rule_phase.m_rule_index))
            {
                Associativity first_rule_associativity =
                    GetRuleAssociativity(first_terminal_rule_phase.m_rule_index);
                // if the terminal's rules don't have all the same associativity,
                // generate an error
                for (StateIdentifier::const_iterator rule_phase_it = terminal_state_identifier.begin(),
                                                  rule_phase_it_end = terminal_state_identifier.end();
                     rule_phase_it != rule_phase_it_end;
                     ++rule_phase_it)
                {
                    Uint32 current_rule_index = (*rule_phase_it).m_rule_index;
                    Associativity current_rule_associativity =
                        GetRuleAssociativity(current_rule_index);
                    if (current_rule_associativity != first_rule_associativity)
                    {
                        // is this conflict resolveable in a meaningful way?
                        ostringstream buffer;
                        buffer << "conflict in rule associativity between rules "
                               << first_terminal_rule_phase.m_rule_index << " and "
                               << default_rule_phase.m_rule_index;
                        EmitError(FileLocation(GetOptions()->GetInputFilename()), buffer.str());
                        skip_rest = true;
                    }
                }

                if (skip_rest)
                    continue;

                // at this point, all the rules matched by the terminal are guaranteed
                // to have the same associativity.

                // if left-assoc, the default action should be taken (reduce).
                if (first_rule_associativity == A_LEFT)
                    terminal_state_identifier.clear();
                // if right-assoc, the terminal action should be taken (shift)
                else if (first_rule_associativity == A_RIGHT)
                {
                    // don't actually do anything -- allow the terminal action to
                    // remain, because it takes priority over the default action.
                }
                // if nonassoc, then check the associativity of the default rule.
                else if (first_rule_associativity == A_NONASSOC)
                {
                    // if the default rule is nonassoc, there is a
                    // problem with the grammar. so emit an error.
                    if (GetRuleAssociativity(default_rule_phase.m_rule_index) == A_NONASSOC)
                    {
                        ostringstream buffer;
                        buffer << "nesting of %nonassoc between rules "
                            << first_terminal_rule_phase.m_rule_index << " and "
                            << default_rule_phase.m_rule_index;
                        EmitError(FileLocation(GetOptions()->GetInputFilename()), buffer.str());
                    }
                    // otherwise, use the terminal's action, which requires
                    // no effort on our part.
                }
            }
        }
    }
}

void StateMachine::AddTransitions (
    State *const state,
    map<Terminal *, Transition> *const terminal_transition_map,
    map<Nonterminal *, Transition> *const nonterminal_transition_map,
    Transition *const default_transition)
{
    assert(terminal_transition_map != NULL);
    assert(nonterminal_transition_map != NULL);
    assert(default_transition != NULL);

    // add transitions, queueing non-existent states for generation when necessary
    Uint32 terminal_transition_count = 0;
    Uint32 nonterminal_transition_count = 0;

    // add the terminal state transitions
    for (map<Terminal *, Transition>::iterator
             it = terminal_transition_map->begin(),
             it_end = terminal_transition_map->end();
         it != it_end;
         ++it)
    {
        Terminal *terminal = it->first;
        Transition const &transition = it->second;
        assert(terminal != NULL);

        if (transition.GetIsNontrivial())
        {
            ++terminal_transition_count;
            state->SetTerminalTransition(terminal, transition);
            if (!transition.GetTransitionIdentifier().empty() &&
                transition.GetTransitionAction() != TA_REDUCE_USING_RULE &&
                transition.GetTransitionAction() != TA_REDUCE_AND_ACCEPT_USING_RULE)
            {
                m_transition_generation_queue.push_back(transition.GetTransitionIdentifier());
            }
        }
    }
    if (terminal_transition_count > 0)
        state->SetTerminalTransitionsOffset(m_state_transition_count);
    m_state_transition_count += terminal_transition_count;

    // add the default state transition
    if (!default_transition->GetTransitionIdentifier().empty())
    {
        state->SetDefaultTransitionOffset(m_state_transition_count++);
        state->SetDefaultTransition(*default_transition);
        if (default_transition->GetTransitionAction() != TA_REDUCE_USING_RULE &&
            default_transition->GetTransitionAction() != TA_REDUCE_AND_ACCEPT_USING_RULE)
        {
            m_transition_generation_queue.push_back(default_transition->GetTransitionIdentifier());
        }
    }

    // add the nonterminal state transitions
    for (map<Nonterminal *, Transition>::iterator
             it = nonterminal_transition_map->begin(),
             it_end = nonterminal_transition_map->end();
         it != it_end;
         ++it)
    {
        Nonterminal *nonterminal = it->first;
        Transition const &transition = it->second;
        assert(nonterminal != NULL);

        if (!transition.GetTransitionIdentifier().empty())
        {
            ++nonterminal_transition_count;
            state->SetNonterminalTransition(it->first, it->second);
            m_transition_generation_queue.push_back(transition.GetTransitionIdentifier());
        }
    }
    if (nonterminal_transition_count > 0)
        state->SetNonterminalTransitionsOffset(m_state_transition_count);
    m_state_transition_count += nonterminal_transition_count;
}

void StateMachine::AddSpawnedRulePhases (
    State *const state,
    map<Nonterminal *, Transition> *const nonterminal_transition_map)
{
    assert(state != NULL);
    assert(nonterminal_transition_map != NULL);

    // add the spawned rule phases (which correspond one-to-one
    // with the nonterminal transition map
    for (map<Nonterminal *, Transition>::const_iterator
             it = nonterminal_transition_map->begin(),
             it_end = nonterminal_transition_map->end();
         it != it_end;
         ++it)
    {
        Nonterminal const *nonterminal = it->first;
        assert(nonterminal != NULL);
        for (RuleList::const_iterator rule_it = nonterminal->GetRuleListBegin(),
                                     rule_it_end = nonterminal->GetRuleListEnd();
             rule_it != rule_it_end;
             ++rule_it)
        {
            Rule const *rule = *rule_it;
            assert(rule != NULL);
            state->AddSpawnedRulePhase(RulePhase(rule->GetIndex(), 0));
        }
    }
}

void StateMachine::GeneratePreprocessor ()
{
    ParserDirectiveSet const *parser_directive_set = m_grammar->GetParserDirectiveSet();
    assert(parser_directive_set != NULL);

    // check the consistency of parser directives and options and whatnot
    {
        // nothing for now
    }

    // input each of the parser directives' values into the preprocessor
    for (Uint32 i = 0; i < PDT_COUNT; ++i)
    {
        ParserDirectiveType parser_directive_type =
            ParserDirectiveType(i);
        string line_directive;
        string returning_line_directive;
        if (GetParserDirectiveShouldHaveLineDirective(parser_directive_type) &&
            parser_directive_set->GetDirectiveFileLocation(parser_directive_type).GetHasLineNumber() &&
            GetOptions()->GetWithLineDirectives())
        {
            line_directive = "\n";
            line_directive += parser_directive_set->GetDirectiveFileLocation(parser_directive_type).GetLineDirectiveString();
            line_directive += '\n';
            returning_line_directive = Preprocessor::GetReturningLineDirectiveTag();
        }
        m_preprocessor.SetReplacementValue(
            Preprocessor::GetReplacementFromParserDirectiveType(parser_directive_type),
            line_directive +
            parser_directive_set->GetDirectiveValueText(parser_directive_type) +
            returning_line_directive);
    }

    // CLASS_INHERITANCE - has to be done special
    if (parser_directive_set->GetIsDirectiveSpecified(PDT_CLASS_INHERITANCE))
    {
        m_preprocessor.SetReplacementValue(
            Preprocessor::CLASS_INHERITANCE,
            "    : " + parser_directive_set->GetDirectiveValueText(PDT_CLASS_INHERITANCE));
    }

    // SUPERCLASS_AND_MEMBER_CONSTRUCTORS - has to be done special
    if (parser_directive_set->GetIsDirectiveSpecified(PDT_CLASS_INHERITANCE))
    {
        m_preprocessor.SetReplacementValue(
            Preprocessor::SUPERCLASS_AND_MEMBER_CONSTRUCTORS,
            "    : " +
            parser_directive_set->GetDirectiveValueText(PDT_SUPERCLASS_AND_MEMBER_CONSTRUCTORS));
    }

    // FORCE_VIRTUAL_DESTRUCTOR - has to be done special
    if (parser_directive_set->GetIsDirectiveSpecified(PDT_CLASS_INHERITANCE) ||
        parser_directive_set->GetIsDirectiveSpecified(PDT_FORCE_VIRTUAL_DESTRUCTOR))
    {
        m_preprocessor.SetReplacementValue(
            Preprocessor::FORCE_VIRTUAL_DESTRUCTOR,
            "virtual ");
    }

    // CUSTOM_CAST - has to be done special
    if (!parser_directive_set->GetIsDirectiveSpecified(PDT_CUSTOM_CAST))
    {
        m_preprocessor.SetReplacementValue(
            Preprocessor::CUSTOM_CAST,
            "static_cast");
    }

    // create the non-parser-directive values and input them
    string base_assigned_type =
        parser_directive_set->GetDirectiveValueText(PDT_BASE_ASSIGNED_TYPE);
    string class_name =
        parser_directive_set->GetDirectiveValueText(PDT_CLASS_NAME);

    // TERMINAL_TOKEN_DECLARATIONS and TERMINAL_TOKEN_STRINGS
    {
        ostringstream declaration_buffer;
        ostringstream strings_buffer;

        string end_name("END_");
        string error_name("ERROR_");
        bool first_has_been_written = false;

        // write out all of the grammar's non-single-character terminal tokens
        for (TerminalMap::const_iterator it = m_terminal_map.begin(),
                                     it_end = m_terminal_map.end();
             it != it_end;
             ++it)
        {
            Terminal const *terminal = it->second;
            assert(terminal != NULL);
            if (terminal->GetTokenIdentifier()->GetAstType() == AT_TOKEN_IDENTIFIER_IDENTIFIER)
            {
                string token_implementation_file_identifier =
                    terminal->GetImplementationFileIdentifier();
                assert(!token_implementation_file_identifier.empty());
                if (token_implementation_file_identifier != end_name &&
                    token_implementation_file_identifier != error_name)
                {
                    if (!first_has_been_written)
                    {
                        declaration_buffer << "            " << token_implementation_file_identifier << " = 0x100,\n";
                        first_has_been_written = true;
                    }
                    else
                        declaration_buffer << "            " << token_implementation_file_identifier << ",\n";

                    strings_buffer << "        \"" << terminal->GetTokenIdentifier()->GetText() << "\",\n";
                }
            }
        }

        declaration_buffer << "\n"
                           << "            // special end-of-input terminal\n"
                           << "            " << end_name << ",\n";
        strings_buffer << "        \"" << end_name << "\",\n";

        m_preprocessor.SetReplacementValue(
            Preprocessor::TERMINAL_TOKEN_DECLARATIONS,
            declaration_buffer.str());
        m_preprocessor.SetReplacementValue(
            Preprocessor::TERMINAL_TOKEN_STRINGS,
            strings_buffer.str());
    }

    // NONTERMINAL_TOKEN_DECLARATIONS and NONTERMINAL_TOKEN_STRINGS
    {
        ostringstream declaration_buffer;
        ostringstream strings_buffer;

        // write out all of the grammar's nonterminal tokens
        for (NonterminalMap::const_iterator it = m_nonterminal_map.begin(),
                                         it_end = m_nonterminal_map.end();
             it != it_end;
             ++it)
        {
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            declaration_buffer << "            " << nonterminal->GetImplementationFileIdentifier() << ",\n";
            strings_buffer << "        \"" << nonterminal->GetIdentifier()->GetText() << "\",\n";
        }

        declaration_buffer << "\n"
                           << "            // special start nonterminal\n"
                           << "            START_,\n";
        strings_buffer << "        \"START_\",\n";

        m_preprocessor.SetReplacementValue(
            Preprocessor::NONTERMINAL_TOKEN_DECLARATIONS,
            declaration_buffer.str());
        m_preprocessor.SetReplacementValue(
            Preprocessor::NONTERMINAL_TOKEN_STRINGS,
            strings_buffer.str());
    }

    // REDUCTION_RULE_HANDLER_DECLARATIONS
    {
        ostringstream buffer;

        for (Uint32 rule_index = 0, rule_count = m_rule_vector.size();
            rule_index < rule_count;
            ++rule_index)
        {
            buffer << "    " << base_assigned_type << " ReductionRuleHandler"
                   << setw(4) << setfill('0') << rule_index << setfill(' ') << " ();\n";
        }

        m_preprocessor.SetReplacementValue(
            Preprocessor::REDUCTION_RULE_HANDLER_DECLARATIONS,
            buffer.str());
    }

    // REDUCTION_RULE_HANDLER_DEFINITIONS
    {
        ostringstream buffer;

        for (Uint32 rule_index = 0, rule_count = m_rule_vector.size();
             rule_index < rule_count;
             ++rule_index)
        {
            Rule const *rule = m_rule_vector[rule_index];
            assert(rule != NULL);
            buffer << "// ";
            rule->PrettyPrintWithAssignedIdentifiers(buffer);
            buffer << "\n";

            buffer << base_assigned_type << " " << class_name << "::ReductionRuleHandler"
                   << setw(4) << setfill('0') << rule_index << setfill(' ') << " ()\n"
                   << "{\n";

            if (rule->GetCodeBlock() != NULL)
            {
                // declare the assigned variable identifiers
                for (Uint32 rule_token_index = 0, rule_token_count = rule->GetRuleTokenCount();
                     rule_token_index < rule_token_count;
                     ++rule_token_index)
                {
                    RuleToken const *rule_token = rule->GetRuleToken(rule_token_index);
                    assert(rule_token != NULL);
                    if (rule_token->GetAssignedIdentifier() != NULL)
                        buffer << "    assert(" << rule_token_index << " < m_reduction_rule_token_count);\n"
                               << "    " << GetAssignedType(rule_token->GetTokenIdentifier()->GetText())
                               << " " << rule_token->GetAssignedIdentifier()->GetText() << " = "
                               << m_preprocessor.GetReplacementValue(Preprocessor::CUSTOM_CAST) << "< "
                               << GetAssignedType(rule_token->GetTokenIdentifier()->GetText())
                               << " >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + "
                               << rule_token_index << "]);\n";
                }

                string line_directive;
                string returning_line_directive;
                if (rule->GetCodeBlock()->GetFileLocation().GetHasLineNumber() &&
                    GetOptions()->GetWithLineDirectives())
                {
                    line_directive = "\n";
                    line_directive += rule->GetCodeBlock()->GetFileLocation().GetLineDirectiveString();
                    line_directive += '\n';
                    returning_line_directive = Preprocessor::GetReturningLineDirectiveTag();
                }
                buffer << line_directive
                       << rule->GetCodeBlock()->GetText()
                       << returning_line_directive << "\n";
            }
            buffer << "    return "
                   << m_preprocessor.GetReplacementValue(Preprocessor::BASE_ASSIGNED_TYPE_SENTINEL)
                   << ";\n"
                   << "}\n"
                   << "\n";
        }

        m_preprocessor.SetReplacementValue(
            Preprocessor::REDUCTION_RULE_HANDLER_DEFINITIONS,
            buffer.str());
    }

    // REDUCTION_RULE_LOOKUP_TABLE
    {
        ostringstream buffer;

        for (Uint32 rule_index = 0, rule_count = m_rule_vector.size();
             rule_index < rule_count;
             ++rule_index)
        {
            Rule const *rule = m_rule_vector[rule_index];
            assert(rule != NULL);
            Nonterminal const *owner_nonterminal = rule->GetOwnerNonterminal();
            assert(owner_nonterminal != NULL);
            buffer << "    {" << setw(30)
                   << ("Token::" + owner_nonterminal->GetImplementationFileIdentifier())
                   << ", " << setw(2) << rule->GetRuleTokenCount() << ", &"
                   << class_name << "::ReductionRuleHandler" << setw(4)
                   << setfill('0') << rule_index << setfill(' ') << ", \"";
            rule->PrettyPrint(buffer);
//             if (rule_index < rule_count - 1)
                buffer << "\"},\n";
//             else
//                 buffer << "\"}\n";
        }

        m_preprocessor.SetReplacementValue(
            Preprocessor::REDUCTION_RULE_LOOKUP_TABLE,
            buffer.str());
    }

    // STATE_TRANSITION_LOOKUP_TABLE
    {
        ostringstream buffer;

        for (Uint32 state_index = 0, state_count = m_state_vector.size();
             state_index < state_count;
             ++state_index)
        {
            buffer << "    ";
            m_state_vector[state_index]->PrintIndices(buffer);
            if (state_index < state_count - 1)
                buffer << ", // state " << setw(4) << state_index << "\n";
            else
                buffer << "  // state " << setw(4) << state_index << "\n";
        }

        m_preprocessor.SetReplacementValue(
            Preprocessor::STATE_TRANSITION_LOOKUP_TABLE,
            buffer.str());
    }

    // STATE_TRANSITION_TABLE
    {
        ostringstream buffer;

        for (Uint32 state_index = 0, state_count = m_state_vector.size();
             state_index < state_count;
             ++state_index)
        {
            bool is_last_state = state_index == state_count - 1;
            buffer << "// ///////////////////////////////////////////////////////////////////////////\n"
                   << "// state " << setw(4) << state_index << "\n"
                   << "// ///////////////////////////////////////////////////////////////////////////\n";
            State const *state = m_state_vector[state_index];
            assert(state != NULL);
            state->PrintTransitions(buffer, is_last_state, this);
            if (!is_last_state)
                buffer << "\n";
        }

        m_preprocessor.SetReplacementValue(
            Preprocessor::STATE_TRANSITION_TABLE,
            buffer.str());
    }

    // HEADER_FILENAME
    {
        m_preprocessor.SetReplacementValue(
            Preprocessor::HEADER_FILENAME,
            GetFilenamePortion(GetOptions()->GetHeaderFilename()));
    }
}

} // end of namespace Trison
