// 2019.09.23 - Victor Dods

#pragma once

#include "barftest/core.hpp"

namespace barftest {
namespace sem {

enum class ValueLifetimeContextual : uint8_t
{
    LOCAL = 0,
    GLOBAL,
    DETERMINE_FROM_CONTEXT,

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = LOCAL,
    __HIGHEST__ = DETERMINE_FROM_CONTEXT
};

std::string const &as_string (ValueLifetimeContextual value_lifetime_contextual);
std::string const &as_string_lowercase (ValueLifetimeContextual value_lifetime_contextual);

inline std::ostream &operator << (std::ostream &out, ValueLifetimeContextual value_lifetime_contextual)
{
    return out << as_string_lowercase(value_lifetime_contextual);
}

// Represents specific, non-contextual ValueLifetime.
// Should this really be ValueLifetime?
enum class ValueLifetime : uint8_t
{
    LOCAL = uint8_t(ValueLifetimeContextual::LOCAL),
    GLOBAL = uint8_t(ValueLifetimeContextual::GLOBAL),

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = LOCAL,
    __HIGHEST__ = GLOBAL
};

inline std::string const &as_string (ValueLifetime value_lifetime) { return as_string(ValueLifetimeContextual(value_lifetime)); }
inline std::string const &as_string_lowercase (ValueLifetime value_lifetime) { return as_string_lowercase(ValueLifetimeContextual(value_lifetime)); }

inline std::ostream &operator << (std::ostream &out, ValueLifetime value_lifetime)
{
    return out << as_string_lowercase(value_lifetime);
}

} // end namespace sem
} // end namespace barftest
