// ///////////////////////////////////////////////////////////////////////////
// barf_message.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_MESSAGE_HPP_)
#define BARF_MESSAGE_HPP_

#include "barf.hpp"

#include "barf_filelocation.hpp"

// the application must define in the global namespace the following symbol(s):
extern bool g_errors_encountered;

namespace Barf {

void EmitExecutionMessage (string const &message);
void EmitWarning (string const &message, FileLocation const &filoc = FileLocation::ms_invalid);
void EmitError (string const &message, FileLocation const &filoc = FileLocation::ms_invalid);
void EmitFatalError (string const &message, FileLocation const &filoc = FileLocation::ms_invalid);

} // end of namespace Barf

#endif // !defined(BARF_MESSAGE_HPP_)
