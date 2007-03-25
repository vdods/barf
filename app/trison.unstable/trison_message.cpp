// ///////////////////////////////////////////////////////////////////////////
// trison_message.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_message.hpp"

#include "trison_options.hpp"

namespace Trison {

extern bool g_conflicts_encountered;

void EmitConflictWarning (string const &message)
{
    g_conflicts_encountered = true;
    if (GetOptions()->GetTreatWarningsAsErrors())
        cerr << GetOptions()->GetInputFilename() << ": error: " << message << endl;
    else
        cerr << GetOptions()->GetInputFilename() << ": warning: " << message << endl;
}

} // end of namespace Trison
