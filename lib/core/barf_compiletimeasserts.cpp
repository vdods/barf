// 2006.04.16 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_compiletimeasserts.hpp"

namespace Barf {

GLOBAL_SCOPE_COMPILE_TIME_ASSERT(check_sizeof_char, sizeof(char) == 1)
GLOBAL_SCOPE_COMPILE_TIME_ASSERT(check_sizeof_sint8, sizeof(Sint8) == 1)
GLOBAL_SCOPE_COMPILE_TIME_ASSERT(check_sizeof_uint8, sizeof(Uint8) == 1)
GLOBAL_SCOPE_COMPILE_TIME_ASSERT(check_sizeof_sint16, sizeof(Sint16) == 2)
GLOBAL_SCOPE_COMPILE_TIME_ASSERT(check_sizeof_uint16, sizeof(Uint16) == 2)
GLOBAL_SCOPE_COMPILE_TIME_ASSERT(check_sizeof_sint32, sizeof(Sint32) == 4)
GLOBAL_SCOPE_COMPILE_TIME_ASSERT(check_sizeof_uint32, sizeof(Uint32) == 4)

} // end of namespace Barf
