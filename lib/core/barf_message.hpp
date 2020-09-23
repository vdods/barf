// 2006.02.11 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_MESSAGE_HPP_)
#define BARF_MESSAGE_HPP_

#include "barf.hpp"

#include "barf_filoc.hpp"

// the application must define in the global namespace the following symbol(s):
extern bool g_errors_encountered;

namespace Barf {

void EmitExecutionMessage (string const &message);
void EmitWarning (string const &message, FiLoc const &filoc = FiLoc::ms_invalid);
void EmitError (string const &message, FiLoc const &filoc = FiLoc::ms_invalid);
void EmitFatalError (string const &message, FiLoc const &filoc = FiLoc::ms_invalid);

} // end of namespace Barf

#endif // !defined(BARF_MESSAGE_HPP_)
