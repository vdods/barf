// 2020.05.26 - Victor Dods

#include "sem/GlobalValueLinkage.hpp"

#include "Exception.hpp"

namespace cbz {
namespace sem {

std::string const &as_string (GlobalValueLinkageContextual global_value_linkage_contextual)
{
    static_assert(uint32_t(GlobalValueLinkageContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(GlobalValueLinkageContextual::__HIGHEST__)+1 - uint32_t(GlobalValueLinkageContextual::__LOWEST__)] =
    {
        "EXTERNAL",
        "INTERNAL",
        "PRIVATE",
        "DETERMINE_FROM_CONTEXT",
    };
    uint32_t n = uint32_t(global_value_linkage_contextual);
    if (n > uint32_t(GlobalValueLinkageContextual::__HIGHEST__))
        LVD_ABORT("Invalid GlobalValueLinkageContextual");
    return s_string_table[n];
}

std::string const &as_string_lowercase (GlobalValueLinkageContextual global_value_linkage_contextual)
{
    static_assert(uint32_t(GlobalValueLinkageContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(GlobalValueLinkageContextual::__HIGHEST__)+1 - uint32_t(GlobalValueLinkageContextual::__LOWEST__)] =
    {
        "external",
        "internal",
        "private",
        "determine_from_context",
    };
    uint32_t n = uint32_t(global_value_linkage_contextual);
    if (n > uint32_t(GlobalValueLinkageContextual::__HIGHEST__))
        LVD_ABORT("Invalid GlobalValueLinkageContextual");
    return s_string_table[n];
}

} // end namespace sem
} // end namespace cbz
