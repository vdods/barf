// ///////////////////////////////////////////////////////////////////////////
// trison_main.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison.hpp"

#include <fstream>

#include "trison_ast.hpp"
#include "trison_options.hpp"
#include "trison_parser.hpp"
#include "trison_statemachine.hpp"

// these globals are required by barf_message.hpp
OptionsBase *g_options = NULL;
bool g_errors_encountered = false;

namespace Trison {

// required by trison_message.hpp
bool g_conflicts_encountered = false;

} // end of namespace Trison

using namespace Trison;

int main (int argc, char **argv)
{
    // note: screw deleting all the heap objects, since everything will
    // be freed at the end of this function by the OS anyway.
    // TODO: don't screw deleting all the heap objects,
    // because.. well, fuck that shit!

    try
    {
        g_options = new Options(argv[0]);
        GetOptions().Parse(argc, argv);
        if (GetOptions().GetAbort())
            return 1;
        else if (GetOptions().GetIsHelpRequested())
        {
            GetOptions().PrintHelpMessage(cerr);
            return 0;
        }

        Parser parser;

        if (GetOptions().GetShowParsingSpew())
            parser.SetDebugSpewLevel(2);

        if (!parser.SetInputFilename(GetOptions().GetInputFilename()))
        {
            if (GetOptions().GetInputFilename().empty())
                cerr << "error: no input file" << endl;
            else
                cerr << "error: input file \"" << GetOptions().GetInputFilename()
                     << "\" could not be read" << endl;
            return 1;
        }

        Parser::ParserReturnCode parser_return_code = parser.Parse();

        Grammar *parsed_grammar = Dsc<Grammar *>(parser.GetAcceptedToken());
        assert(parsed_grammar != NULL);

        if (g_errors_encountered || parser_return_code != Parser::PRC_SUCCESS)
            return 1;

        if (GetOptions().GetShowSyntaxTree())
            parsed_grammar->Print(cerr);

        // this code block exists so that the lifetime of state_machine
        // exists entirely within the lifetime of *g_parsed_grammar
        {
            StateMachine state_machine(parsed_grammar);

            if (g_errors_encountered)
                return 1;

            state_machine.Generate();

            if (g_errors_encountered)
                return 1;

            if (!(GetOptions().GetTreatWarningsAsErrors() && g_conflicts_encountered) &&
                GetOptions().GetIsOutputBasenameSpecified())
            {
                assert(!GetOptions().GetHeaderFilename().empty());
                assert(!GetOptions().GetImplementationFilename().empty());

                ofstream header_file(GetOptions().GetHeaderFilename().c_str());
                state_machine.PrintHeaderFile(header_file);
                header_file.close();

                ofstream implementation_file(GetOptions().GetImplementationFilename().c_str());
                state_machine.PrintImplementationFile(implementation_file);
                implementation_file.close();
            }

            if (!GetOptions().GetStateMachineFilename().empty())
            {
                if (GetOptions().GetStateMachineFilename() == "-")
                {
                    state_machine.PrintStateMachineFile(cout);
                }
                else
                {
                    ofstream state_machine_file(GetOptions().GetStateMachineFilename().c_str());
                    state_machine.PrintStateMachineFile(state_machine_file);
                    state_machine_file.close();
                }
            }
        }

        return (GetOptions().GetTreatWarningsAsErrors() && g_conflicts_encountered) ? 1 : 0;
    }
    catch (string const &exception)
    {
        cerr << exception << endl;
        return 1;
    }
}
