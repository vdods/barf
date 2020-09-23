// 2006.02.19 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(TRISON_ENUMS_HPP_)
#define TRISON_ENUMS_HPP_

#include "trison.hpp"

#include <iostream>

namespace Trison {

enum Associativity
{
    A_LEFT = 0,
    A_NONASSOC,
    A_RIGHT,

    A_COUNT
}; // end of enum Associativity

void PrettyPrint (ostream &stream, Associativity associativity);

ostream &operator << (ostream &stream, Associativity associativity);

} // end of namespace Trison

#endif // !defined(TRISON_ENUMS_HPP_)
