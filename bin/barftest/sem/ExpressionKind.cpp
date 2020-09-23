// 2019.11.15 - Victor Dods

#include "sem/ExpressionKind.hpp"

#include "Exception.hpp"

namespace cbz {
namespace sem {

std::string const &as_string (ExpressionKindContextual expression_kind_contextual)
{
    static_assert(uint32_t(ExpressionKindContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(ExpressionKindContextual::__HIGHEST__)+1 - uint32_t(ExpressionKindContextual::__LOWEST__)] = {
        "METATYPE",
        "TYPE",
        "VALUE",
        "DETERMINE_FROM_CONTEXT",
    };
    uint32_t n = uint32_t(expression_kind_contextual);
    if (n > uint32_t(ExpressionKindContextual::__HIGHEST__))
        LVD_ABORT("Invalid ExpressionKindContextual");
    return s_string_table[n];
}

} // end namespace sem
} // end namespace cbz
