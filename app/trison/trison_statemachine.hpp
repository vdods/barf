// ///////////////////////////////////////////////////////////////////////////
// trison_statemachine.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_STATEMACHINE_HPP_)
#define _TRISON_STATEMACHINE_HPP_

#include "trison.hpp"

#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "trison_enums.hpp"
#include "trison_preprocessor.hpp"
#include "trison_state.hpp"
#include "trison_stateidentifier.hpp"
#include "trison_terminal.hpp"
#include "trison_transition.hpp"

namespace Trison {

class Grammar;
class Nonterminal;
class Rule;
class RuleToken;
class StateMachine;

class StateMachine
{
public:

    StateMachine (Grammar const *grammar);
    ~StateMachine ();

    Rule const *GetRule (Uint32 rule_index) const;
    Uint32 GetStateIndex (StateIdentifier const &state_identifier) const;

    void Generate ();

    void PrintStateMachineFile (ostream &stream) const;
    void PrintHeaderFile (ostream &stream);
    void PrintImplementationFile (ostream &stream);

private:

    void GetStateTransitionIdentifiersFromRulePhase (
        RulePhase const &rule_phase,
        map<Terminal *, Transition> *terminal_transition_map,
        map<Nonterminal *, Transition> *nonterminal_transition_map,
        Transition *default_transition) const;
    void GetRulePhasesWithNonterminalOnLeft (
        Nonterminal const *nonterminal,
        set<RulePhase> *rule_phase_set) const;
    void GetRulePhasesWithNonterminalOnLeft (
        Nonterminal const *nonterminal,
        set<RulePhase> *rule_phase_set,
        set<RulePhase> *visited_rule_phase_set) const;
    bool GetIsLastPhaseInRule (RulePhase const &rule_phase) const;
    RuleToken const *GetRuleTokenToLeft (RulePhase const &rule_phase) const;
    RuleToken const *GetRuleTokenToRight (RulePhase const &rule_phase) const;
    Associativity GetRuleAssociativity (Uint32 rule_index) const;
    Uint32 GetRulePrecedence (Uint32 rule_index) const;
    bool GetIsABinaryOperation (Uint32 rule_index) const;
    string const &GetAssignedType (string const &token_name) const;
    bool GetParserDirectiveShouldHaveLineDirective (ParserDirectiveType parser_directive_type) const;

    void GeneratePrecedenceMap ();
    void GenerateNonterminalMap ();
    void GenerateTerminalMap ();
    void GenerateReductionRuleVector ();
    void GenerateReductionRuleVectorForNonterminal (Nonterminal *nonterminal);
    void GenerateStatesFromTheStartNonterminal ();
    void GenerateState (StateIdentifier const &state_identifier);

    void AddErrorCollapsingTransitions (
        State *state,
        map<Terminal *, Transition> *terminal_transition_map);
    void ResolveReduceReduceAndErrorConflicts (
        State *state,
        StateIdentifier const &state_identifier,
        map<Terminal *, Transition> *terminal_transition_map,
        map<Nonterminal *, Transition> *nonterminal_transition_map,
        Transition *default_transition);
    void ResolveShiftReduceConflicts (
        StateIdentifier const &state_identifier,
        map<Terminal *, Transition> *terminal_transition_map,
        map<Nonterminal *, Transition> *nonterminal_transition_map,
        Transition *default_transition);
    void AddTransitions (
        State *state,
        map<Terminal *, Transition> *terminal_transition_map,
        map<Nonterminal *, Transition> *nonterminal_transition_map,
        Transition *default_transition);
    void AddSpawnedRulePhases (
        State *state,
        map<Nonterminal *, Transition> *nonterminal_transition_map);

    void GeneratePreprocessor ();

    typedef map<string, Nonterminal *> NonterminalMap;
    typedef map<string, Terminal *> TerminalMap;
    typedef vector<Rule *> RuleVector;
    typedef map<string, PrecedenceDirective const *> PrecedenceMap;
    typedef map<StateIdentifier, State *> StateMap;
    typedef vector<State *> StateVector;
    typedef list<StateIdentifier> TransitionGenerationQueue;

    Grammar const *const m_grammar;

    NonterminalMap m_nonterminal_map;
    TerminalMap m_terminal_map;
    RuleVector m_rule_vector;
    PrecedenceMap m_precedence_map;
    StateMap m_state_map;
    StateVector m_state_vector;
    TransitionGenerationQueue m_transition_generation_queue;
    Uint32 m_state_transition_count;
    Terminal *m_error_terminal;

    Uint32 m_shift_reduce_conflict_count;
    Uint32 m_reduce_reduce_conflict_count;

    Preprocessor m_preprocessor;
    static string const ms_header_file_template;
    static string const ms_implementation_file_template;
}; // end of class StateMachine

} // end of namespace Trison

#endif // !defined(_TRISON_STATEMACHINE_HPP_)
