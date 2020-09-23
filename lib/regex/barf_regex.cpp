// 2006.11.03 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_regex.hpp"

namespace Barf {
namespace Regex {

string const &ConditionalTypeString (ConditionalType conditional_type)
{
    static string const s_conditional_type_string[CT_COUNT] =
    {
        "CT_BEGINNING_OF_INPUT",
        "CT_NOT_BEGINNING_OF_INPUT",
        "CT_END_OF_INPUT",
        "CT_NOT_END_OF_INPUT",
        "CT_BEGINNING_OF_LINE",
        "CT_NOT_BEGINNING_OF_LINE",
        "CT_END_OF_LINE",
        "CT_NOT_END_OF_LINE",
        "CT_WORD_BOUNDARY",
        "CT_NOT_WORD_BOUNDARY"
    };
    assert(conditional_type < CT_COUNT);
    return s_conditional_type_string[conditional_type];
}

Conditional GetConditionalFromConditionalType (ConditionalType conditional_type)
{
    switch (conditional_type)
    {
        case CT_BEGINNING_OF_INPUT:     return Conditional(0x01, 0x01);
        case CT_NOT_BEGINNING_OF_INPUT: return Conditional(0x01, 0x00);
        case CT_END_OF_INPUT:           return Conditional(0x02, 0x02);
        case CT_NOT_END_OF_INPUT:       return Conditional(0x02, 0x00);
        case CT_BEGINNING_OF_LINE:      return Conditional(0x04, 0x04);
        case CT_NOT_BEGINNING_OF_LINE:  return Conditional(0x04, 0x00);
        case CT_END_OF_LINE:            return Conditional(0x08, 0x08);
        case CT_NOT_END_OF_LINE:        return Conditional(0x08, 0x00);
        case CT_WORD_BOUNDARY:          return Conditional(0x10, 0x10);
        case CT_NOT_WORD_BOUNDARY:      return Conditional(0x10, 0x00);
        default: assert(false && "invalid ConditionalType"); return Conditional(0, 0);
    }
}

ConditionalType GetConditionalTypeFromConditional (Conditional &conditional)
{
    if (conditional.m_mask == 0)
        return CT_COUNT;

    // get the lowest bit
    Uint8 mask = 1;
    while ((mask & conditional.m_mask) == 0)
        mask <<= 1;
    // unset that bit in the conditional's mask
    conditional.m_mask &= ~mask;
    // return the corresponding ConditionalType enum value
    switch (mask)
    {
        case 0x01: return (conditional.m_flags&mask) != 0 ? CT_BEGINNING_OF_INPUT : CT_NOT_BEGINNING_OF_INPUT;
        case 0x02: return (conditional.m_flags&mask) != 0 ? CT_END_OF_INPUT       : CT_NOT_END_OF_INPUT;
        case 0x04: return (conditional.m_flags&mask) != 0 ? CT_BEGINNING_OF_LINE  : CT_NOT_BEGINNING_OF_LINE;
        case 0x08: return (conditional.m_flags&mask) != 0 ? CT_END_OF_LINE        : CT_NOT_END_OF_LINE;
        case 0x10: return (conditional.m_flags&mask) != 0 ? CT_WORD_BOUNDARY      : CT_NOT_WORD_BOUNDARY;
        default: assert(false && "this function doesn't match GetConditionalFromConditionalType"); return CT_COUNT;
    }
}

string const &BakedControlCharTypeString (BakedControlCharType baked_control_char_type)
{
    static string const s_baked_control_char_type_string[BCCT_COUNT] =
    {
        "BCCT_CASE_SENSITIVITY_DISABLE",
        "BCCT_CASE_SENSITIVITY_ENABLE"
    };
    assert(baked_control_char_type < BCCT_COUNT);
    return s_baked_control_char_type_string[baked_control_char_type];
}

} // end of namespace Regex
} // end of namespace Barf
