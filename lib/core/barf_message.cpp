// 2006.02.11 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_message.hpp"

#include <iostream>
#include <sstream>

#include "barf_optionsbase.hpp"

namespace Barf {

void EmitExecutionMessage (string const &message)
{
    if (GetOptions().IsVerbose(OptionsBase::V_EXECUTION))
        cerr << GetOptions().ProgramName() << ": " << message << endl;
}

void EmitWarning (string const &message, FiLoc const &filoc)
{
    if (OptionsAreInitialized() && GetOptions().TreatWarningsAsErrors())
        EmitError(message, filoc);
    else if (filoc.IsValid())
        cerr << GetOptions().ProgramName() << ": " << filoc << ": warning: " << message << endl;
    else
        cerr << GetOptions().ProgramName() << ": " << "warning: " << message << endl;
}

void EmitError (string const &message, FiLoc const &filoc)
{
    g_errors_encountered = true;
    if (OptionsAreInitialized() && GetOptions().HaltOnFirstError())
        EmitFatalError(message, filoc);
    else
    {
        if (filoc.IsValid())
            cerr << GetOptions().ProgramName() << ": " << filoc << ": error: " << message << endl;
        else
            cerr << GetOptions().ProgramName() << ": " << "error: " << message << endl;

#if DEBUG
        if (OptionsAreInitialized() && GetOptions().AssertOnError())
            assert(false && "you have requested to assert on error, human, and here it is");
#endif
    }
}

void EmitFatalError (string const &message, FiLoc const &filoc)
{
    g_errors_encountered = true;
    
#if DEBUG
    if (filoc.IsValid())
        cerr << GetOptions().ProgramName() << ": " << filoc << ": error: " << message << endl;
    else
        cerr << GetOptions().ProgramName() << ": " << "error: " << message << endl;
        
    if (OptionsAreInitialized() && GetOptions().AssertOnError())
        assert(false && "you have requested to assert on error, human, and here it is");
#endif

    if (filoc.IsValid())
        THROW_STRING(GetOptions().ProgramName() << ": " << filoc << ": fatal error: " << message);
    else
        THROW_STRING(GetOptions().ProgramName() << ": " << "fatal error: " << message);
}

} // end of namespace Barf
