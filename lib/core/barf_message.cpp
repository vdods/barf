// ///////////////////////////////////////////////////////////////////////////
// barf_message.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_message.hpp"

#include <iostream>
#include <sstream>

#include "barf_optionsbase.hpp"

namespace Barf {

void EmitExecutionMessage (string const &message)
{
    if (GetOptions().GetIsVerbose(OptionsBase::V_EXECUTION))
        cerr << GetOptions().GetProgramName() << ": " << message << endl;
}

void EmitWarning (string const &message, FiLoc const &filoc)
{
    if (OptionsAreInitialized() && GetOptions().GetTreatWarningsAsErrors())
        EmitError(message, filoc);
    else if (filoc.GetIsValid())
        cerr << GetOptions().GetProgramName() << ": " << filoc << ": warning: " << message << endl;
    else
        cerr << GetOptions().GetProgramName() << ": " << "warning: " << message << endl;
}

void EmitError (string const &message, FiLoc const &filoc)
{
    g_errors_encountered = true;
    if (OptionsAreInitialized() && GetOptions().GetHaltOnFirstError())
        EmitFatalError(message, filoc);
    else
    {
#if DEBUG
        if (OptionsAreInitialized() && GetOptions().GetAssertOnError())
            assert(false && "you have requested to assert on error, human, and here it is");
#endif
        if (filoc.GetIsValid())
            cerr << GetOptions().GetProgramName() << ": " << filoc << ": error: " << message << endl;
        else
            cerr << GetOptions().GetProgramName() << ": " << "error: " << message << endl;
    }
}

void EmitFatalError (string const &message, FiLoc const &filoc)
{
    g_errors_encountered = true;
#if DEBUG
    if (OptionsAreInitialized() && GetOptions().GetAssertOnError())
        assert(false && "you have requested to assert on error, human, and here it is");
#endif
    if (filoc.GetIsValid())
        THROW_STRING(GetOptions().GetProgramName() << ": " << filoc << ": fatal error: " << message);
    else
        THROW_STRING(GetOptions().GetProgramName() << ": " << "fatal error: " << message);
}

} // end of namespace Barf
