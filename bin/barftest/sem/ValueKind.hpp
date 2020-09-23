// 2019.11.15 - Victor Dods

#pragma once

#include "barftest/core.hpp"

namespace barftest {
namespace sem {

enum class ValueKindContextual : uint8_t
{
    CONSTANT = 0,
    VARIABLE,
    DETERMINE_FROM_CONTEXT,

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = CONSTANT,
    __HIGHEST__ = DETERMINE_FROM_CONTEXT
};

std::string const &as_string (ValueKindContextual value_kind_contextual);
std::string const &as_string_lowercase (ValueKindContextual value_kind_contextual);

inline std::ostream &operator << (std::ostream &out, ValueKindContextual value_kind_contextual)
{
    return out << as_string_lowercase(value_kind_contextual);
}

// Represents specific, non-contextual ValueKind
enum class ValueKind : uint8_t
{
    CONSTANT = uint8_t(ValueKindContextual::CONSTANT),
    VARIABLE = uint8_t(ValueKindContextual::VARIABLE),

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = CONSTANT,
    __HIGHEST__ = VARIABLE
};

inline std::string const &as_string (ValueKind value_kind) { return as_string(ValueKindContextual(value_kind)); }
inline std::string const &as_string_lowercase (ValueKind value_kind) { return as_string_lowercase(ValueKindContextual(value_kind)); }

inline std::ostream &operator << (std::ostream &out, ValueKind value_kind)
{
    return out << as_string_lowercase(value_kind);
}

} // end namespace sem
} // end namespace barftest
