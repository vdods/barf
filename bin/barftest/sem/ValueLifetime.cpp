// 2019.09.23 - Victor Dods

#include "barftest/sem/ValueLifetime.hpp"

#include "barftest/Exception.hpp"
#include <sstream>

namespace barftest {
namespace sem {

std::string const &as_string (ValueLifetimeContextual value_lifetime_contextual)
{
    static_assert(uint32_t(ValueLifetimeContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(ValueLifetimeContextual::__HIGHEST__)+1 - uint32_t(ValueLifetimeContextual::__LOWEST__)] = {
        "LOCAL",
        "GLOBAL",
        "DETERMINE_FROM_CONTEXT",
    };
    uint32_t n = uint32_t(value_lifetime_contextual);
    if (n > uint32_t(ValueLifetimeContextual::__HIGHEST__))
        LVD_ABORT("Invalid ValueLifetimeContextual");
    return s_string_table[n];
}

std::string const &as_string_lowercase (ValueLifetimeContextual value_lifetime_contextual)
{
    static_assert(uint32_t(ValueLifetimeContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(ValueLifetimeContextual::__HIGHEST__)+1 - uint32_t(ValueLifetimeContextual::__LOWEST__)] = {
        "local",
        "global",
        "determine_from_context",
    };
    uint32_t n = uint32_t(value_lifetime_contextual);
    if (n > uint32_t(ValueLifetimeContextual::__HIGHEST__))
        LVD_ABORT("Invalid ValueLifetimeContextual");
    return s_string_table[n];
}

} // end namespace sem
} // end namespace barftest
