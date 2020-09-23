// 2020.05.26 - Victor Dods

#pragma once

#include "core.hpp"

namespace cbz {
namespace sem {

// GlobalValueLinkages are only for global symbols.
enum class GlobalValueLinkageContextual : uint8_t
{
    EXTERNAL = 0,
    INTERNAL,
    PRIVATE,
    DETERMINE_FROM_CONTEXT,

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = EXTERNAL,
    __HIGHEST__ = DETERMINE_FROM_CONTEXT
};

std::string const &as_string (GlobalValueLinkageContextual global_value_linkage_contextual);
std::string const &as_string_lowercase (GlobalValueLinkageContextual global_value_linkage_contextual);

inline std::ostream &operator << (std::ostream &out, GlobalValueLinkageContextual global_value_linkage_contextual)
{
    return out << as_string_lowercase(global_value_linkage_contextual);
}

// Represents specific, non-contextual GlobalValueLinkage
// GlobalValueLinkages are only for global symbols.
enum class GlobalValueLinkage : uint8_t
{
    EXTERNAL = uint8_t(GlobalValueLinkageContextual::EXTERNAL),
    INTERNAL = uint8_t(GlobalValueLinkageContextual::INTERNAL),
    PRIVATE = uint8_t(GlobalValueLinkageContextual::PRIVATE),

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = PRIVATE,
    __HIGHEST__ = EXTERNAL
};

inline std::string const &as_string (GlobalValueLinkage global_value_linkage) { return as_string(GlobalValueLinkageContextual(global_value_linkage)); }
inline std::string const &as_string_lowercase (GlobalValueLinkage global_value_linkage) { return as_string_lowercase(GlobalValueLinkageContextual(global_value_linkage)); }

inline std::ostream &operator << (std::ostream &out, GlobalValueLinkage global_value_linkage)
{
    return out << as_string_lowercase(global_value_linkage);
}

// This function converts GlobalValueLinkageContextual to GlobalValueLinkage, where DETERMINE_FROM_CONTEXT is converted to
// the value given as template argument VALUE_FOR_CONTEXT_.
template <GlobalValueLinkage VALUE_FOR_CONTEXT_>
GlobalValueLinkage global_value_linkage_determined (GlobalValueLinkageContextual gslc)
{
    if (gslc == GlobalValueLinkageContextual::DETERMINE_FROM_CONTEXT)
        return VALUE_FOR_CONTEXT_;
    else
        return GlobalValueLinkage(gslc);
}

} // end namespace sem
} // end namespace cbz
