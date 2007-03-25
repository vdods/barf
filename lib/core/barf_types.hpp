// ///////////////////////////////////////////////////////////////////////////
// barf_types.hpp by Victor Dods, created 2006/10/14
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_TYPES_HPP_)
#define _BARF_TYPES_HPP_

// this file is included in barf.h, so it doesn't need to include barf.h

namespace Barf {

typedef signed char Sint8;
typedef unsigned char Uint8;

typedef signed short int Sint16;
typedef unsigned short int Uint16;

typedef signed int Sint32;
typedef unsigned int Uint32;

#define SINT8_LOWER_BOUND  Sint8(0x80)
#define SINT8_UPPER_BOUND  Sint8(0x7F)

#define UINT8_LOWER_BOUND  Uint8(0x00)
#define UINT8_UPPER_BOUND  Uint8(0xFF)

#define SINT16_LOWER_BOUND Sint16(0x8000)
#define SINT16_UPPER_BOUND Sint16(0x7FFF)

#define UINT16_LOWER_BOUND Uint16(0x0000)
#define UINT16_UPPER_BOUND Uint16(0xFFFF)

#define SINT32_LOWER_BOUND Sint32(0x80000000)
#define SINT32_UPPER_BOUND Sint32(0x7FFFFFFF)

#define UINT32_LOWER_BOUND Uint32(0x00000000)
#define UINT32_UPPER_BOUND Uint32(0xFFFFFFFF)

// i think it's probably pretty safe to assume there will not be more than
// 2^16 different concrete Ast subclasses (including applications' subclasses).
typedef Uint16 AstType;
// this type is used for specifying a function which can turn an
// AstType value into a human-readable string.
typedef string const &(*StringifyAstType)(AstType);

} // end of namespace Barf

#endif // !defined(_BARF_TYPES_HPP_)
