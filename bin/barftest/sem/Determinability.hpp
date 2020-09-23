// 2019.11.15 - Victor Dods

#pragma once

#include "core.hpp"

namespace cbz {
namespace sem {

enum class DeterminabilityContextual : uint8_t
{
    COMPILETIME = 0,
    RUNTIME,
    DETERMINE_FROM_CONTEXT,

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = COMPILETIME,
    __HIGHEST__ = DETERMINE_FROM_CONTEXT
};

std::string const &as_string (DeterminabilityContextual determinability_contextual);

// Represents specific, non-contextual Determinability
enum class Determinability : uint8_t
{
    COMPILETIME = uint8_t(DeterminabilityContextual::COMPILETIME),
    RUNTIME = uint8_t(DeterminabilityContextual::RUNTIME),

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = COMPILETIME,
    __HIGHEST__ = RUNTIME
};

// The DeterminabilityContextual as_string should work for Determinability.

} // end namespace sem
} // end namespace cbz
