// ///////////////////////////////////////////////////////////////////////////
// trison_state.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_STATE_HPP_)
#define _TRISON_STATE_HPP_

#include "trison.hpp"

#include <iostream>
#include <map>

#include "trison_stateidentifier.hpp"
#include "trison_transition.hpp"

namespace Trison {

class Nonterminal;
class Terminal;

class State
{
public:

    State (Uint32 state_index)
        :
        m_state_index(state_index)
    {
        m_terminal_transitions_offset = 0;
        m_default_transition_offset = 0;
        m_nonterminal_transitions_offset = 0;
    }

    inline Uint32 GetStateIndex () const { return m_state_index; }
    inline StateIdentifier const &GetStateIdentifier () const
    {
        assert(!m_state_identifier.empty());
        return m_state_identifier;
    }

    inline void SetStateIdentifier (StateIdentifier const &state_identifier)
    {
        m_state_identifier = state_identifier;
    }
    inline void SetTerminalTransitionsOffset (Uint32 terminal_transitions_offset)
    {
        assert(terminal_transitions_offset > 0);
        m_terminal_transitions_offset = terminal_transitions_offset;
    }
    inline void SetTerminalTransition (Terminal const *input_terminal, Transition const &transition)
    {
        assert(input_terminal != NULL);
        assert(transition.GetIsNontrivial());
        assert(m_terminal_transition_map.find(input_terminal) == m_terminal_transition_map.end());
        m_terminal_transition_map[input_terminal] = transition;
    }
    inline void SetDefaultTransitionOffset (Uint32 default_transition_offset)
    {
        assert(default_transition_offset > 0);
        m_default_transition_offset = default_transition_offset;
    }
    inline void SetDefaultTransition (Transition const &transition)
    {
        assert(m_default_transition.GetTransitionIdentifier().empty());
        m_default_transition = transition;
    }
    inline void SetNonterminalTransitionsOffset (Uint32 nonterminal_transitions_offset)
    {
        assert(nonterminal_transitions_offset > 0);
        m_nonterminal_transitions_offset = nonterminal_transitions_offset;
    }
    inline void SetNonterminalTransition (Nonterminal const *input_nonterminal, Transition const &transition)
    {
        assert(input_nonterminal != NULL);
        assert(!transition.GetTransitionIdentifier().empty());
        assert(m_nonterminal_transition_map.find(input_nonterminal) == m_nonterminal_transition_map.end());
        m_nonterminal_transition_map[input_nonterminal] = transition;
    }

    inline void SetConflictedTerminalTransition (Terminal const *input_terminal, Transition const &transition)
    {
        assert(input_terminal != NULL);
        assert(transition.GetIsNontrivial());
        m_conflicted_terminal_transition_map.insert(
            ConflictedTerminalTransition(input_terminal, transition));
    }
    inline void SetConflictedDefaultTransition (Transition const &transition)
    {
        assert(transition.GetIsNontrivial());
        m_conflicted_terminal_transition_map.insert(
            ConflictedTerminalTransition(NULL, transition));
    }

    inline bool operator == (State const &operand) const
    {
        return m_state_identifier == operand.m_state_identifier;
    }

    inline void AddSpawnedRulePhase (RulePhase const &spawned_rule_phase)
    {
        m_spawned_rule_phase_set.insert(spawned_rule_phase);
    }

    void PrettyPrint (ostream &stream, StateMachine const *state_machine) const;
    void PrintIndices (ostream &stream) const;
    void PrintTransitions (ostream &stream, bool is_last_state, StateMachine const *state_machine) const;

private:

    typedef map<Terminal const *, Transition> TerminalTransitionMap;
    typedef map<Nonterminal const *, Transition> NonterminalTransitionMap;
    typedef pair<Terminal const *, Transition> ConflictedTerminalTransition;
    typedef multimap<Terminal const *, Transition> ConflictedTerminalTransitionMap;

    Uint32 const m_state_index;
    StateIdentifier m_state_identifier;
    set<RulePhase> m_spawned_rule_phase_set;
    Uint32 m_terminal_transitions_offset;
    TerminalTransitionMap m_terminal_transition_map;
    Uint32 m_default_transition_offset;
    Transition m_default_transition;
    Uint32 m_nonterminal_transitions_offset;
    NonterminalTransitionMap m_nonterminal_transition_map;

    ConflictedTerminalTransitionMap m_conflicted_terminal_transition_map;
}; // end of class State

} // end of namespace Trison

#endif // !defined(_TRISON_STATE_HPP_)
