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

void EmitWarning (string const &message)
{
    if (g_options != NULL && g_options->GetTreatWarningsAsErrors())
        EmitError(message);
    else
        cerr << "warning: " << message << endl;
}

void EmitWarning (FiLoc const &filoc, string const &message)
{
    if (g_options != NULL && g_options->GetTreatWarningsAsErrors())
        EmitError(filoc, message);
    else
        cerr << filoc.GetAsString() << ": warning: " << message << endl;
}

void EmitError (string const &message)
{
    g_errors_encountered = true;
    if (g_options != NULL && g_options->GetHaltOnFirstError())
        EmitFatalError(message);
    else
    {
#if DEBUG
        if (g_options != NULL && g_options->GetAssertOnError())
            assert(false && "you have requested to assert on error, human, and here it is");
#endif
        cerr << "error: " << message << endl;
    }
}

void EmitError (FiLoc const &filoc, string const &message)
{
    g_errors_encountered = true;
    if (g_options != NULL && g_options->GetHaltOnFirstError())
        EmitFatalError(filoc, message);
    else
    {
#if DEBUG
        if (g_options != NULL && g_options->GetAssertOnError())
            assert(false && "you have requested to assert on error, human, and here it is");
#endif
        cerr << filoc.GetAsString() << ": error: " << message << endl;
    }
}

void EmitFatalError (string const &message)
{
    g_errors_encountered = true;
#if DEBUG
    if (g_options != NULL && g_options->GetAssertOnError())
        assert(false && "you have requested to assert on error, human, and here it is");
#endif
    THROW_STRING("fatal: " << message);
}

void EmitFatalError (FiLoc const &filoc, string const &message)
{
    g_errors_encountered = true;
#if DEBUG
    if (g_options != NULL && g_options->GetAssertOnError())
        assert(false && "you have requested to assert on error, human, and here it is");
#endif
    THROW_STRING(filoc.GetAsString() << ": fatal: " << message);
}

} // end of namespace Barf
