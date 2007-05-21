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

enum ReturnStatus
{
    RS_SUCCESS = 0,
    RS_COMMANDLINE_ABORT,
    RS_PRIMARY_SOURCE_ERROR,
    RS_DETERMINISTIC_AUTOMATON_GENERATION_ERROR,
    RS_TARGETSPEC_ERROR,
    RS_CODESPEC_ERROR,
    RS_WRITE_TARGETS_ERROR,
    RS_FATAL_ERROR
}; // end of enum ReturnStatus

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
        exit(RS_COMMANDLINE_ABORT);
    }
    else if (GetOptions().GetIsHelpRequested())
    {
        GetOptions().PrintHelpMessage(cerr);
        exit(RS_SUCCESS);
    }
    else if (GetOptions().GetPrintTargetsSearchPathRequest() == Reflex::Options::PTSPR_SHORT)
    {
        cout << GetOptions().GetTargetsSearchPath().GetAsString("\n") << endl;
        exit(RS_SUCCESS);
    }
    else if (GetOptions().GetPrintTargetsSearchPathRequest() == Reflex::Options::PTSPR_VERBOSE)
    {
        cout << GetOptions().GetTargetsSearchPath().GetAsVerboseString("\n") << endl;
        exit(RS_SUCCESS);
    }
}

Reflex::PrimarySource const *ParsePrimarySource ()
{
    // parse the primary source and check everything possible without
    // having parsed the targetspec sources.  if any errors were
    // accumulated during this section, abort with an error code.

    Reflex::PrimarySource *primary_source = NULL;

    Ast::Base *parsed_tree_root = NULL;
    Reflex::Parser parser;
    parser.ScannerDebugSpew(GetOptions().GetIsVerbose(Reflex::Options::V_PRIMARY_SOURCE_SCANNER));
    if (GetOptions().GetIsVerbose(Reflex::Options::V_PRIMARY_SOURCE_PARSER))
        parser.SetDebugSpewLevel(2);

    if (!parser.OpenFile(GetOptions().GetInputFilename()))
        EmitError("file not found: \"" + GetOptions().GetInputFilename() + "\"");
    else if (parser.Parse(&parsed_tree_root) != Reflex::Parser::PRC_SUCCESS)
        EmitError(FiLoc(GetOptions().GetInputFilename()), "general reflex parse error");
    else
    {
        primary_source = Dsc<Reflex::PrimarySource *>(parsed_tree_root);
        assert(primary_source != NULL);
        if (GetOptions().GetIsVerbose(Reflex::Options::V_PRIMARY_SOURCE_AST))
            primary_source->Print(cerr);
    }

    if (g_errors_encountered)
        exit(RS_PRIMARY_SOURCE_ERROR);

    return primary_source;
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

void GenerateAndPrintNfaDotGraph (Reflex::PrimarySource const &primary_source, Automaton &nfa)
{
    // generate the NFA and print it (if the filename is even specified).
    // no error is possible in this section.

    Reflex::GenerateNfa(primary_source, nfa);
    PrintDotGraph(nfa.m_graph, GetOptions().GetNaDotGraphPath(), "NFA");
}

void GenerateAndPrintDfaDotGraph (Reflex::PrimarySource const &primary_source, Automaton const &nfa, Automaton &dfa)
{
    // generate the DFA and print it (if the filename is even specified).
    // GenerateDfa may throw an exception, which should be interpreted
    // as a program error.  if any errors were accumulated during this
    // section, abort with an error code.

    try {
        Reflex::GenerateDfa(primary_source, nfa, primary_source.GetRuleCount(), dfa);
        PrintDotGraph(dfa.m_graph, GetOptions().GetDaDotGraphPath(), "DFA");
    } catch (string const &exception) {
        EmitError(exception);
    }

    if (g_errors_encountered)
        exit(RS_DETERMINISTIC_AUTOMATON_GENERATION_ERROR);
}

void ParseTargetspecs (Reflex::PrimarySource const &primary_source)
{
    // for each target in the primary source, parse the corresponding
    // targetspec source, checking for required directives, doing everything
    // possible to check short of running the run-before-code-gen code.
    // if any errors were accumulated during this section, abort with
    // an error code.

    Targetspec::Parser parser;
    parser.ScannerDebugSpew(GetOptions().GetIsVerbose(Reflex::Options::V_TARGETSPEC_SCANNER));
    if (GetOptions().GetIsVerbose(Reflex::Options::V_TARGETSPEC_PARSER))
        parser.SetDebugSpewLevel(2);

    for (CommonLang::TargetMap::const_iterator it = primary_source.m_target_map->begin(),
                                               it_end = primary_source.m_target_map->end();
         it != it_end;
         ++it)
    {
        CommonLang::Target const *target = it->second;
        assert(target != NULL);
        target->ParseTargetspec("reflex", parser);
    }

    if (g_errors_encountered)
        exit(RS_TARGETSPEC_ERROR);
}

void ParseCodespecs (Reflex::PrimarySource const &primary_source)
{
    // for each codespec in every target, parse the codespec and
    // make all checks possible at this time.  if any errors were
    // accumulated during this section, abort with an error code.

    Preprocessor::Parser parser;
    parser.ScannerDebugSpew(GetOptions().GetIsVerbose(Reflex::Options::V_CODESPEC_SCANNER));
    if (GetOptions().GetIsVerbose(Reflex::Options::V_CODESPEC_PARSER))
        parser.SetDebugSpewLevel(2);

    for (CommonLang::TargetMap::const_iterator it = primary_source.m_target_map->begin(),
                                               it_end = primary_source.m_target_map->end();
         it != it_end;
         ++it)
    {
        CommonLang::Target const *target = it->second;
        assert(target != NULL);
        target->ParseCodespecs("reflex", parser);
    }

    if (g_errors_encountered)
        exit(RS_CODESPEC_ERROR);
}

void WriteTargets (Reflex::PrimarySource const &primary_source, Automaton const &nfa, Automaton const &dfa)
{
    // for each target, fill in an empty Preprocessor::SymbolTable with the
    // macros to be used by the codespecs, and execute each codespec, each
    // with its own copy of the symbol table.  if any errors were accumulated
    // during this section, abort with an error code.

    Preprocessor::SymbolTable global_symbol_table;

    Reflex::GenerateGeneralAutomatonSymbols(primary_source, global_symbol_table);
    Reflex::GenerateNfaSymbols(primary_source, nfa.m_graph, nfa.m_start_state_index, global_symbol_table);
    Reflex::GenerateDfaSymbols(primary_source, dfa.m_graph, dfa.m_start_state_index, global_symbol_table);

    for (CommonLang::TargetMap::const_iterator it = primary_source.m_target_map->begin(),
                                               it_end = primary_source.m_target_map->end();
         it != it_end;
         ++it)
    {
        string const &target_id = it->first;
        CommonLang::Target const *target = it->second;
        assert(target != NULL);

        Preprocessor::SymbolTable local_symbol_table(global_symbol_table);

        Reflex::GenerateTargetDependentSymbols(primary_source, target_id, local_symbol_table);
        if (GetOptions().GetIsVerbose(Reflex::Options::V_CODESPEC_SYMBOLS))
            local_symbol_table.Print(cerr);

        target->GenerateCode(local_symbol_table);
    }

    if (g_errors_encountered)
        exit(RS_WRITE_TARGETS_ERROR);
}

int main (int argc, char **argv)
{
    try {
        Reflex::PrimarySource const *primary_source = NULL;
        Automaton nfa, dfa;

        ParseAndHandleOptions(argc, argv);
        primary_source = ParsePrimarySource();
        GenerateAndPrintNfaDotGraph(*primary_source, nfa);
        GenerateAndPrintDfaDotGraph(*primary_source, nfa, dfa);
        ParseTargetspecs(*primary_source);
        ParseCodespecs(*primary_source);
        WriteTargets(*primary_source, nfa, dfa);

        delete primary_source;
        return RS_SUCCESS;
    } catch (string const &exception) {
        // this is the catch block for fatal errors
        cerr << exception << endl;
        return RS_FATAL_ERROR;
    }
}
