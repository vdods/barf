// ///////////////////////////////////////////////////////////////////////////
// reflex_main.cpp by Victor Dods, created 2006/10/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "reflex.hpp"

#include <stdlib.h>

#include "barf_targetspec_ast.hpp"
#include "barf_targetspec_parser.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_parser.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "reflex_ast.hpp"
#include "reflex_automaton.hpp"
#include "reflex_codespecsymbols.hpp"
#include "reflex_options.hpp"
#include "reflex_parser.hpp"

// these globals are required by barf_message.h
OptionsBase *g_options = NULL;
bool g_errors_encountered = false;

void ParseAndHandleOptions (int argc, char **argv)
{
    // create the Options singleton and parse the commandline.
    // if the abort flag is set (indicating commandline error),
    // exit with an error code.  otherwise perform handle-and-quit
    // options (printing the help message, etc) if present.

    g_options = new Reflex::Options(argv[0]);
    GetOptions().Parse(argc, argv);
    if (GetOptions().GetAbort())
    {
        exit(1);
    }
    else if (GetOptions().GetIsHelpRequested())
    {
        GetOptions().PrintHelpMessage(cerr);
        exit(0);
    }
    else if (GetOptions().GetPrintTargetsSearchPathRequest() == Reflex::Options::PTSPR_SHORT)
    {
        cout << GetOptions().GetTargetsSearchPath().GetAsString("\n") << endl;
        exit(0);
    }
    else if (GetOptions().GetPrintTargetsSearchPathRequest() == Reflex::Options::PTSPR_VERBOSE)
    {
        cout << GetOptions().GetTargetsSearchPath().GetAsVerboseString("\n") << endl;
        exit(0);
    }
}

Reflex::Representation const *ParsePrimarySource ()
{
    // parse the primary source and check everything possible without
    // having parsed the targetspec sources.  if any errors were
    // accumulated during this section, abort with an error code.

    Reflex::Representation *representation = NULL;

    Reflex::Parser parser;
    if (GetOptions().GetShowScanningSpew())
        parser.ScannerDebugSpew(true);
    if (GetOptions().GetShowParsingSpew())
        parser.SetDebugSpewLevel(2);

    if (!parser.OpenFile(GetOptions().GetInputFilename()))
        EmitError("file not found: \"" + GetOptions().GetInputFilename() + "\"");
    else if (parser.Parse() != Reflex::Parser::PRC_SUCCESS)
        EmitError(FiLoc(GetOptions().GetInputFilename()), "general reflex parse error");
    else
    {
        representation = Dsc<Reflex::Representation *>(parser.GetAcceptedToken());
        assert(representation != NULL);
        if (GetOptions().GetShowSyntaxTree())
            representation->Print(cerr);
    }

    if (g_errors_encountered)
        exit(2);

    return representation;
}

void PrintDotGraph (Graph const &graph, string const &filename, string const &graph_name)
{
    if (filename.empty())
        return;

    if (filename == "-")
        graph.PrintDotGraph(cout, graph_name);
    else
    {
        ofstream file(filename.c_str());
        if (!file.is_open())
            EmitWarning("could not open file \"" + filename + "\" for writing");
        else
            graph.PrintDotGraph(file, graph_name);
    }
}

void GenerateAndPrintNfa (Reflex::Representation const &representation, Automaton &nfa)
{
    // generate the NFA and print it (if the filename is even specified).
    // no error is possible in this section.

    Reflex::GenerateNfa(representation, nfa);
    PrintDotGraph(nfa.m_graph, GetOptions().GetNaDotGraphPath(), "NFA");
}

void GenerateAndPrintDfa (Reflex::Representation const &representation, Automaton const &nfa, Automaton &dfa)
{
    // generate the DFA and print it (if the filename is even specified).
    // GenerateDfa may throw an exception, which should be interpreted
    // as a program error.  if any errors were accumulated during this
    // section, abort with an error code.

    try {
        Reflex::GenerateDfa(nfa, representation.GetRuleCount(), dfa);
        PrintDotGraph(dfa.m_graph, GetOptions().GetDaDotGraphPath(), "DFA");
    } catch (string const &exception) {
        EmitError(exception);
    }

    if (g_errors_encountered)
        exit(3);
}

void ParseTargetSpecs (Reflex::Representation const &representation)
{
    // for each target in the primary source, parse the corresponding
    // targetspec source, checking for required directives, doing everything
    // possible to check short of running the run-before-code-gen code.
    // if any errors were accumulated during this section, abort with
    // an error code.

    TargetSpec::Parser parser;
    if (GetOptions().GetShowTargetSpecParsingSpew())
        parser.SetDebugSpewLevel(2);

    for (CommonLang::TargetMap::const_iterator it = representation.m_target_map->begin(),
                                               it_end = representation.m_target_map->end();
         it != it_end;
         ++it)
    {
        CommonLang::Target const *target = it->second;
        assert(target != NULL);
        target->ParseTargetSpec("reflex", parser);
    }

    if (g_errors_encountered)
        exit(4);
}

void ParseCodeSpecs (Reflex::Representation const &representation)
{
    // for each codespec in every target, parse the codespec and
    // make all checks possible at this time.  if any errors were
    // accumulated during this section, abort with an error code.

    Preprocessor::Parser parser;
    if (GetOptions().GetShowPreprocessorParsingSpew())
        parser.SetDebugSpewLevel(2);

    for (CommonLang::TargetMap::const_iterator it = representation.m_target_map->begin(),
                                               it_end = representation.m_target_map->end();
         it != it_end;
         ++it)
    {
        CommonLang::Target const *target = it->second;
        assert(target != NULL);
        target->ParseCodeSpecs("reflex", parser);
    }

    if (g_errors_encountered)
        exit(5);
}

void WriteTargets (Reflex::Representation const &representation, Automaton const &nfa, Automaton const &dfa)
{
    // for each target, fill in an empty Preprocessor::SymbolTable with the
    // macros to be used by the codespecs, and execute each codespec, each
    // with its own copy of the symbol table.  if any errors were accumulated
    // during this section, abort with an error code.

    Preprocessor::SymbolTable global_symbol_table;

    Reflex::GenerateGeneralAutomatonSymbols(representation, global_symbol_table);
    Reflex::GenerateNfaSymbols(representation, nfa.m_graph, nfa.m_start_state_index, global_symbol_table);
    Reflex::GenerateDfaSymbols(representation, dfa.m_graph, dfa.m_start_state_index, global_symbol_table);

    for (CommonLang::TargetMap::const_iterator it = representation.m_target_map->begin(),
                                               it_end = representation.m_target_map->end();
         it != it_end;
         ++it)
    {
        string const &target_id = it->first;
        CommonLang::Target const *target = it->second;
        assert(target != NULL);

        Preprocessor::SymbolTable local_symbol_table(global_symbol_table);

        Reflex::GenerateTargetDependentSymbols(representation, target_id, local_symbol_table);

        target->GenerateCode(local_symbol_table);
    }

    if (g_errors_encountered)
        exit(6);
}

int main (int argc, char **argv)
{
    try {
        Reflex::Representation const *representation = NULL;
        Automaton nfa, dfa;

        ParseAndHandleOptions(argc, argv);
        representation = ParsePrimarySource();
        GenerateAndPrintNfa(*representation, nfa);
        GenerateAndPrintDfa(*representation, nfa, dfa);
        ParseTargetSpecs(*representation);
        ParseCodeSpecs(*representation);
        WriteTargets(*representation, nfa, dfa);

        delete representation;
        return 0;
    } catch (string const &exception) {
        // this is the catch block for fatal errors
        cerr << exception << endl;
        return 7;
    }
}
