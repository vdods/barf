// ///////////////////////////////////////////////////////////////////////////
// trison_enums.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

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
