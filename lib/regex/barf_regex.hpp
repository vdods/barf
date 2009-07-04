// ///////////////////////////////////////////////////////////////////////////
// barf_regex.hpp by Victor Dods, created 2006/10/21
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_REGEX_HPP_)
#define BARF_REGEX_HPP_

// EVERY HPP AND CPP FILE IN REGEX SHOULD INCLUDE THIS FILE
// EVERY HPP AND CPP FILE IN REGEX SHOULD INCLUDE THIS FILE
// EVERY HPP AND CPP FILE IN REGEX SHOULD INCLUDE THIS FILE

#include "barf.hpp"

#include "barf_graph.hpp"

namespace Barf {
namespace Regex {

typedef Uint8 ConditionalType;

enum
{
    CT_BEGINNING_OF_INPUT = 0,
    CT_NOT_BEGINNING_OF_INPUT,

    CT_END_OF_INPUT,
    CT_NOT_END_OF_INPUT,

    CT_BEGINNING_OF_LINE,
    CT_NOT_BEGINNING_OF_LINE,

    CT_END_OF_LINE,
    CT_NOT_END_OF_LINE,

    CT_WORD_BOUNDARY,
    CT_NOT_WORD_BOUNDARY,

    CT_COUNT
};

string const &ConditionalTypeString (ConditionalType conditional_type);

// for pairing mask and conditional
struct Conditional
{
    Uint8 m_mask;
    Uint8 m_flags;

    Conditional (Uint8 mask, Uint8 flags) : m_mask(mask), m_flags(mask&flags) { }
    bool Accepts (Uint8 flags) { return ((flags^m_flags)&m_mask) == 0; }
    bool ConflictsWith (Conditional const &c) { return ((c.m_flags^m_flags)&(c.m_mask&m_mask)) != 0; }
    void operator ++ () { do { ++m_flags; } while (((m_flags^m_mask)&~m_mask) != 0); }
};

Conditional GetConditionalFromConditionalType (ConditionalType conditional_type);
ConditionalType GetConditionalTypeFromConditional (Conditional &conditional);

enum BakedControlCharType
{
    BCCT_CASE_SENSITIVITY_DISABLE = 0,
    BCCT_CASE_SENSITIVITY_ENABLE,

    BCCT_COUNT
}; // end of enum BakedControlCharType

string const &BakedControlCharTypeString (BakedControlCharType baked_control_char_type);

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(BARF_REGEX_HPP_)
