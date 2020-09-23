// 2019.11.15 - Victor Dods

#include "barftest/sem/ValueKind.hpp"

#include "barftest/Exception.hpp"

namespace barftest {
namespace sem {

std::string const &as_string (ValueKindContextual value_kind_contextual)
{
    static_assert(uint32_t(ValueKindContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(ValueKindContextual::__HIGHEST__)+1 - uint32_t(ValueKindContextual::__LOWEST__)] = {
        "CONSTANT",
        "VARIABLE",
        "DETERMINE_FROM_CONTEXT",
    };
    uint32_t n = uint32_t(value_kind_contextual);
    if (n > uint32_t(ValueKindContextual::__HIGHEST__))
        LVD_ABORT("Invalid ValueKindContextual");
    return s_string_table[n];
}

std::string const &as_string_lowercase (ValueKindContextual value_kind_contextual)
{
    static_assert(uint32_t(ValueKindContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(ValueKindContextual::__HIGHEST__)+1 - uint32_t(ValueKindContextual::__LOWEST__)] = {
        "constant",
        "variable",
        "determine_from_context",
    };
    uint32_t n = uint32_t(value_kind_contextual);
    if (n > uint32_t(ValueKindContextual::__HIGHEST__))
        LVD_ABORT("Invalid ValueKindContextual");
    return s_string_table[n];
}

} // end namespace sem
} // end namespace barftest
