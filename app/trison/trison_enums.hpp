// ///////////////////////////////////////////////////////////////////////////
// trison_enums.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_ENUMS_HPP_)
#define _TRISON_ENUMS_HPP_

#include "trison.hpp"

#include <iostream>

namespace Trison {

enum Associativity
{
    A_LEFT = 0,
    A_RIGHT,
    A_NONASSOC,

    A_COUNT
}; // end of enum Associativity

enum ParserDirectiveType
{
    PDT_HEADER_FILE_TOP = 0,
    PDT_HEADER_FILE_BOTTOM,
    PDT_CLASS_NAME,
    PDT_CLASS_INHERITANCE,
    PDT_SUPERCLASS_AND_MEMBER_CONSTRUCTORS,
    PDT_FORCE_VIRTUAL_DESTRUCTOR,
    PDT_CONSTRUCTOR_ACTIONS,
    PDT_DESTRUCTOR_ACTIONS,
    PDT_PARSE_METHOD_ACCESS,
    PDT_CLASS_METHODS_AND_MEMBERS,
    PDT_START_OF_PARSE_METHOD_ACTIONS,
    PDT_END_OF_PARSE_METHOD_ACTIONS,
    PDT_THROW_AWAY_TOKEN_ACTIONS,
    PDT_IMPLEMENTATION_FILE_TOP,
    PDT_IMPLEMENTATION_FILE_BOTTOM,
    PDT_BASE_ASSIGNED_TYPE,
    PDT_BASE_ASSIGNED_TYPE_SENTINEL,
    PDT_CUSTOM_CAST,

    PDT_COUNT
}; // end of enum ParserDirectiveType

enum TransitionAction
{
    TA_SHIFT_AND_PUSH_STATE = 0,
    TA_PUSH_STATE,
    TA_REDUCE_USING_RULE,
    TA_REDUCE_AND_ACCEPT_USING_RULE,

    TA_COUNT
}; // end of enum TransitionAction

void PrettyPrint (ostream &stream, Associativity associativity);
void PrettyPrint (ostream &stream, TransitionAction transition_action);

ostream &operator << (ostream &stream, Associativity associativity);
ostream &operator << (ostream &stream, ParserDirectiveType parser_directive_type);
ostream &operator << (ostream &stream, TransitionAction transition_action);

} // end of namespace Trison

#endif // !defined(_TRISON_ENUMS_HPP_)
