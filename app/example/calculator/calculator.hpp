// ///////////////////////////////////////////////////////////////////////////
// calculator.hpp by Victor Dods, created 2007/11/10
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_CALCULATOR_HPP_)
#define _CALCULATOR_HPP_

// EVERY HPP AND CPP FILE IN CALCULATOR SHOULD INCLUDE THIS FILE
// EVERY HPP AND CPP FILE IN CALCULATOR SHOULD INCLUDE THIS FILE
// EVERY HPP AND CPP FILE IN CALCULATOR SHOULD INCLUDE THIS FILE

#include <cassert>
#include <cln/complex.h>
#include <cln/complex_io.h>
#include <cln/float.h>
#include <cln/float_io.h>
#include <cln/integer.h>
#include <cln/integer_io.h>
#include <cln/rational.h>
#include <cln/rational_io.h>
#include <cln/real.h>
#include <cln/real_io.h>
#include <string>

using namespace cln;
using namespace std;

// handy li'l macro for stringifying a boolean
#define BOOL_TO_STRING(x) ((x) ? "true" : "false")
// handy li'l macro to throw a string using stringstream semantics
#define THROW_STRING(x) do { ostringstream out; out << x; throw out.str(); } while (false)
// super handy inline ostringstream formatting macro -- returns std::string
#define FORMAT(x) static_cast<ostringstream &>(ostringstream().flush() << x).str()

namespace Calculator {

// useful little ready-made empty string
extern string const gs_empty_string;

// this should be used as a "safe" static_cast -- in debug mode, it asserts
// on the validity of the dynamic_cast result if the pointer isn't NULL to
// begin with.  in release mode, it just uses static_cast.
template <typename CastToType, typename CastFromType>
inline CastToType Dsc (CastFromType cast_from)
{
    assert(cast_from == static_cast<CastFromType>(0) ||
           dynamic_cast<CastToType>(cast_from) != static_cast<CastToType>(0));
    return static_cast<CastToType>(cast_from);
}

} // end of namespace Calculator

#endif // !defined(_CALCULATOR_HPP_)
