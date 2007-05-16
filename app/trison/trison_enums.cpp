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
        "reduce and accept using rule"
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

ostream &operator << (ostream &stream, ParserDirectiveType parser_directive_type)
{
    static string const s_parser_directive_type_string[PDT_COUNT] =
    {
        "PDT_HEADER_FILE_TOP",
        "PDT_HEADER_FILE_BOTTOM",
        "PDT_CLASS_NAME",
        "PDT_CLASS_INHERITANCE",
        "PDT_SUPERCLASS_AND_MEMBER_CONSTRUCTORS",
        "PDT_FORCE_VIRTUAL_DESTRUCTOR",
        "PDT_CONSTRUCTOR_ACTIONS",
        "PDT_DESTRUCTOR_ACTIONS",
        "PDT_PARSE_METHOD_ACCESS",
        "PDT_CLASS_METHODS_AND_MEMBERS",
        "PDT_START_OF_PARSE_METHOD_ACTIONS",
        "PDT_END_OF_PARSE_METHOD_ACTIONS",
        "PDT_THROW_AWAY_TOKEN_ACTIONS",
        "PDT_IMPLEMENTATION_FILE_TOP",
        "PDT_IMPLEMENTATION_FILE_BOTTOM",
        "PDT_BASE_ASSIGNED_TYPE",
        "PDT_BASE_ASSIGNED_TYPE_SENTINEL",
        "PDT_CUSTOM_CAST"
    };

    assert(parser_directive_type < PDT_COUNT);
    stream << s_parser_directive_type_string[parser_directive_type];
    return stream;
}

ostream &operator << (ostream &stream, TransitionAction transition_action)
{
    static string const s_transition_action_string[TA_COUNT] =
    {
        "TA_SHIFT_AND_PUSH_STATE",
        "TA_PUSH_STATE",
        "TA_REDUCE_USING_RULE",
        "TA_REDUCE_AND_ACCEPT_USING_RULE"
    };

    assert(transition_action < TA_COUNT);
    stream << s_transition_action_string[transition_action];
    return stream;
}

} // end of namespace Trison
