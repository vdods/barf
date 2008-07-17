// ///////////////////////////////////////////////////////////////////////////
// trison_enums.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_ENUMS_HPP_)
#define _TRISON_ENUMS_HPP_

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

#endif // !defined(_TRISON_ENUMS_HPP_)
