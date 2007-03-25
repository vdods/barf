// ///////////////////////////////////////////////////////////////////////////
// trison_rulephase.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_RULE_PHASE_HPP_)
#define _TRISON_RULE_PHASE_HPP_

#include "trison.hpp"

#include <ostream>
#include <set>

namespace Trison {

struct RulePhase
{
    // index into the rule vector
    Uint32 const m_rule_index;
    // phase (0 means a, 1 means b, 2 means c, etc).
    Uint32 const m_phase;

    RulePhase (Uint32 rule_index, Uint32 phase)
        :
        m_rule_index(rule_index),
        m_phase(phase)
    { }
    RulePhase (RulePhase const &rule_phase)
        :
        m_rule_index(rule_phase.m_rule_index),
        m_phase(rule_phase.m_phase)
    { }

    bool operator == (RulePhase const &operand) const
    {
        return m_rule_index == operand.m_rule_index && m_phase == operand.m_phase;
    }
    bool operator < (RulePhase const &operand) const
    {
        return (m_rule_index < operand.m_rule_index) ?
               true :
               (m_rule_index == operand.m_rule_index && m_phase < operand.m_phase);
    }
    RulePhase operator + (Uint32 phase_addend) const
    {
        assert(phase_addend == 1);
        return RulePhase(m_rule_index, m_phase + phase_addend);
    }
}; // end of class RulePhase

inline ostream &operator << (ostream &stream, RulePhase const &rule_phase)
{
    assert(rule_phase.m_phase <= Uint32('z' - 'a'));
    stream << rule_phase.m_rule_index << char(rule_phase.m_phase + 'a');
    return stream;
}

} // end of namespace Trison

#endif // !defined(_TRISON_RULE_PHASE_HPP_)
