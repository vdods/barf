// ///////////////////////////////////////////////////////////////////////////
// trison_transition.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_transition.hpp"

#include <iomanip>

#include "trison_statemachine.hpp"

namespace Trison {

void Transition::PrettyPrint (ostream &stream, string const &name, StateMachine const *state_machine) const
{
    assert(state_machine != NULL);
    stream << name << ": ";
    Trison::PrettyPrint(stream, m_transition_action);
    switch (m_transition_action)
    {
        case TA_SHIFT_AND_PUSH_STATE:
        case TA_PUSH_STATE:
        {
            Uint32 state_index = state_machine->GetStateIndex(m_transition_id);
//             // this should be uncommented if the assert in
//             // StateMachine::GetStateIndex fails.
//             if (state_index == Uint32(-1))
//                 stream << " N/A    (rule(s) " << m_transition_id << ")";
//             else
            stream << " " << state_index << "    (rule(s) " << m_transition_id << ")";
            break;
        }

        case TA_REDUCE_USING_RULE:
        case TA_REDUCE_AND_ACCEPT_USING_RULE:
        {
            assert(m_transition_id.size() == 1);
            RulePhase const &first_and_only_rule_phase = *m_transition_id.begin();
            stream << " " << first_and_only_rule_phase.m_rule_index;
            break;
        }

        default:
            assert(false && "Invalid TransitionAction");
            break;
    }
}

void Transition::PrintTransitionArrayElement (ostream &stream, StateMachine const *state_machine) const
{
    assert(state_machine != NULL);
    assert(GetIsNontrivial());

    stream << "{" << setw(31) << m_transition_action << ", ";
    switch (m_transition_action)
    {
        case TA_SHIFT_AND_PUSH_STATE:
        case TA_PUSH_STATE:
            stream << setw(4) << state_machine->GetStateIndex(m_transition_id) << "}";
            break;

        case TA_REDUCE_USING_RULE:
        case TA_REDUCE_AND_ACCEPT_USING_RULE:
        {
            assert(m_transition_id.size() == 1);
            RulePhase const &first_and_only_rule_phase = *m_transition_id.begin();
            stream << setw(4) << first_and_only_rule_phase.m_rule_index << "}";
            break;
        }

        default:
            assert(false && "Invalid TransitionAction");
            break;
    }
}

} // end of namespace Trison
