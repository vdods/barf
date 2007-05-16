// ///////////////////////////////////////////////////////////////////////////
// trison_transition.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_TRANSITION_HPP_)
#define _TRISON_TRANSITION_HPP_

#include "trison.hpp"

#include "trison_enums.hpp"
#include "trison_rulephase.hpp"
#include "trison_stateid.hpp"

namespace Trison {

class StateMachine;

class Transition
{
public:

    Transition ()
        :
        m_transition_action(TA_COUNT),
        m_transition_id()
    { }

    inline TransitionAction GetTransitionAction () const
    {
        assert(m_transition_action < TA_COUNT && "state transition action not set");
        return m_transition_action;
    }
    inline StateId const &GetTransitionId () const
    {
        return m_transition_id;
    }
    inline StateId &GetTransitionId ()
    {
        return m_transition_id;
    }
    inline bool GetIsNontrivial () const
    {
        return !m_transition_id.empty();
    }

    inline void SetTransitionAction (TransitionAction transition_action)
    {
        m_transition_action = transition_action;
    }

    inline void AddRulePhase (RulePhase const &rule_phase)
    {
        m_transition_id.insert(rule_phase);
    }
    inline void RemoveRulePhase (RulePhase const &rule_phase)
    {
        assert(m_transition_id.find(rule_phase) != m_transition_id.end());
        m_transition_id.erase(rule_phase);
    }

    void PrettyPrint (ostream &stream, string const &name, StateMachine const *state_machine) const;
    void PrintTransitionArrayElement (ostream &stream, StateMachine const *state_machine) const;

private:

    TransitionAction m_transition_action;
    StateId m_transition_id;
}; // end of class Transition

} // end of namespace Trison

#endif // !defined(_TRISON_TRANSITION_HPP_)
