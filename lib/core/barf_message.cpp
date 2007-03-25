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

void EmitWarning (FileLocation const &file_location, string const &message)
{
    if (g_options != NULL && g_options->GetTreatWarningsAsErrors())
        EmitError(file_location, message);
    else
        cerr << file_location.GetAsString() << ": warning: " << message << endl;
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

void EmitError (FileLocation const &file_location, string const &message)
{
    g_errors_encountered = true;
    if (g_options != NULL && g_options->GetHaltOnFirstError())
        EmitFatalError(file_location, message);
    else
    {
#if DEBUG
        if (g_options != NULL && g_options->GetAssertOnError())
            assert(false && "you have requested to assert on error, human, and here it is");
#endif
        cerr << file_location.GetAsString() << ": error: " << message << endl;
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

void EmitFatalError (FileLocation const &file_location, string const &message)
{
    g_errors_encountered = true;
#if DEBUG
    if (g_options != NULL && g_options->GetAssertOnError())
        assert(false && "you have requested to assert on error, human, and here it is");
#endif
    THROW_STRING(file_location.GetAsString() << ": fatal: " << message);
}

} // end of namespace Barf
