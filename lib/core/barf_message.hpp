// ///////////////////////////////////////////////////////////////////////////
// barf_message.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_MESSAGE_HPP_)
#define _BARF_MESSAGE_HPP_

#include "barf.hpp"

#include "barf_filoc.hpp"

// the application must define in the global namespace the following symbol(s):
extern bool g_errors_encountered;

namespace Barf {

void EmitWarning (string const &message);
void EmitWarning (FiLoc const &filoc, string const &message);

void EmitError (string const &message);
void EmitError (FiLoc const &filoc, string const &message);

void EmitFatalError (string const &message);
void EmitFatalError (FiLoc const &filoc, string const &message);

} // end of namespace Barf

#endif // !defined(_BARF_MESSAGE_HPP_)
