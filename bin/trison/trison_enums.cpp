// 2006.02.19 - Copyright Victor Dods - Licensed under Apache 2.0

#include "trison_enums.hpp"

namespace Trison {

void PrettyPrint (ostream &stream, Associativity associativity)
{
    static string const s_associativity_string[A_COUNT] =
    {
        "%left",
        "%right",
        "%nonassoc"
    };

    assert(associativity < A_COUNT);
    stream << s_associativity_string[associativity];
}

ostream &operator << (ostream &stream, Associativity associativity)
{
    static string const s_associativity_string[A_COUNT] =
    {
        "A_LEFT",
        "A_RIGHT",
        "A_NONASSOC"
    };

    assert(associativity < A_COUNT);
    stream << s_associativity_string[associativity];
    return stream;
}

} // end of namespace Trison
