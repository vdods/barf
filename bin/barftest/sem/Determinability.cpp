// 2019.11.15 - Victor Dods

#include "sem/Determinability.hpp"

#include "Exception.hpp"

namespace cbz {
namespace sem {

std::string const &as_string (DeterminabilityContextual determinability_contextual)
{
    static_assert(uint32_t(DeterminabilityContextual::__LOWEST__) == 0);
    static std::string s_string_table[uint32_t(DeterminabilityContextual::__HIGHEST__)+1 - uint32_t(DeterminabilityContextual::__LOWEST__)] = {
        "CONSTANT",
        "VARIABLE",
        "DETERMINE_FROM_CONTEXT",
    };
    uint32_t n = uint32_t(determinability_contextual);
    if (n > uint32_t(DeterminabilityContextual::__HIGHEST__))
        LVD_ABORT_WITH_FIRANGE("Invalid DeterminabilityContextual", FiRange::INVALID);
    return s_string_table[n];
}

} // end namespace sem
} // end namespace cbz
