// ///////////////////////////////////////////////////////////////////////////
// trison_enums.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_enums.hpp"

namespace Trison {

void PrettyPrint (ostream &stream, Associativity associativity)
{
    static string const s_associativity_string[A_COUNT] =
    {
        "%left",
        "%right",
        "%nonassoc"
    };

    assert(associativity < A_COUNT);
    stream << s_associativity_string[associativity];
}

void PrettyPrint (ostream &stream, TransitionAction transition_action)
{
    static string const s_transition_action_string[TA_COUNT] =
    {
        "shift and push state",
        "push state",
        "reduce using rule",
        "reduce and accept using rule",
        "throw away lookahead token"
    };

    assert(transition_action < TA_COUNT);
    stream << s_transition_action_string[transition_action];
}

ostream &operator << (ostream &stream, Associativity associativity)
{
    static string const s_associativity_string[A_COUNT] =
    {
        "A_LEFT",
        "A_RIGHT",
        "A_NONASSOC"
    };

    assert(associativity < A_COUNT);
    stream << s_associativity_string[associativity];
    return stream;
}

ostream &operator << (ostream &stream, TransitionAction transition_action)
{
    static string const s_transition_action_string[TA_COUNT] =
    {
        "TA_SHIFT_AND_PUSH_STATE",
        "TA_PUSH_STATE",
        "TA_REDUCE_USING_RULE",
        "TA_REDUCE_AND_ACCEPT_USING_RULE",
        "TA_THROW_AWAY_LOOKAHEAD_TOKEN"
    };

    assert(transition_action < TA_COUNT);
    stream << s_transition_action_string[transition_action];
    return stream;
}

} // end of namespace Trison
