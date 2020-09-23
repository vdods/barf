// 2019.11.15 - Victor Dods

#pragma once

#include "barftest/core.hpp"

namespace barftest {
namespace sem {

enum class ExpressionKindContextual : uint8_t
{
    METATYPE = 0, // Currently the only one is `Type`
    TYPE,
    VALUE,
    DETERMINE_FROM_CONTEXT,

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = METATYPE,
    __HIGHEST__ = DETERMINE_FROM_CONTEXT
};

std::string const &as_string (ExpressionKindContextual expression_kind_contextual);

inline std::ostream &operator << (std::ostream &out, ExpressionKindContextual expression_kind_contextual)
{
    return out << as_string(expression_kind_contextual);
}

enum class ExpressionKind : uint8_t
{
    METATYPE = uint8_t(ExpressionKindContextual::METATYPE),
    TYPE = uint8_t(ExpressionKindContextual::TYPE),
    VALUE = uint8_t(ExpressionKindContextual::VALUE),

    // NOTE: Make sure to keep these up to date.
    __LOWEST__ = METATYPE,
    __HIGHEST__ = VALUE
};

inline std::string const &as_string (ExpressionKind expression_kind) { return as_string(ExpressionKindContextual(expression_kind)); }

inline std::ostream &operator << (std::ostream &out, ExpressionKind expression_kind)
{
    return out << as_string(expression_kind);
}

} // end namespace sem
} // end namespace barftest
