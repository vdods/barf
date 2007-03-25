// ///////////////////////////////////////////////////////////////////////////
// barf_compiletimeasserts.hpp by Victor Dods, created 2006/10/14
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_COMPILETIMEASSERTS_HPP_)
#define _BARF_COMPILETIMEASSERTS_HPP_

#include "barf.hpp"

namespace Barf {

/// @cond IGNORE_THIS
template <bool condition> struct ThisCompileErrorIsActuallyAFailedCompileTimeAssert;

template <>
struct ThisCompileErrorIsActuallyAFailedCompileTimeAssert<true>
{
    enum { BLAH };
};
/// @endcond

// this assert is intended to be used in the global scope and will produce a
// compile error on conditions that are decidable during compilation
// (e.g. "sizeof(char) == 1").  each call to this macro must supply an assert
// name that is unique to that source file (e.g. CHAR_SIZE_CHECK).
#define GLOBAL_SCOPE_COMPILE_TIME_ASSERT(assert_name, x) \
namespace { inline int assert_name () { return ThisCompileErrorIsActuallyAFailedCompileTimeAssert<bool(x)>::BLAH; } }

// this assert is intended to be used within the body of a function/method
// and will produce a compile error on conditions that are decidable during
// compilation (e.g. "sizeof(something) == 4").
#define CODE_SCOPE_COMPILE_TIME_ASSERT(x) \
ThisCompileErrorIsActuallyAFailedCompileTimeAssert<bool(x)>::BLAH

} // end of namespace Barf

#endif // !defined(_BARF_COMPILETIMEASSERTS_HPP_)
