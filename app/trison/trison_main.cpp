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

#include "barf_targetspec_ast.hpp"
#include "barf_targetspec_parser.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_parser.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "trison_ast.hpp"
#include "trison_codespecsymbols.hpp"
#include "trison_dpda.hpp"
#include "trison_npda.hpp"
#include "trison_options.hpp"
#include "trison_parser.hpp"

// unnamed namespace to hide g_options from other files
namespace {
Trison::Options *g_options = NULL;
}

// required by barf_optionsbase.hpp
bool OptionsAreInitialized () { return g_options != NULL; }
OptionsBase const &GetOptions ()
{
    assert(g_options != NULL && "g_options not initialized yet");
    return *g_options;
}

// required by barf_message.h
bool g_errors_encountered = false;

namespace Trison {

// required by trison_message.h
bool g_conflicts_encountered = false;

} // end of namespace Trison

enum ReturnStatus
{
    RS_SUCCESS = 0,
    RS_COMMANDLINE_ABORT,
    RS_PREDEFINE_ERROR,
    RS_INPUT_FILE_ERROR,
    RS_PRIMARY_SOURCE_ERROR,
    RS_POSTDEFINE_ERROR,
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

    g_options = new Trison::Options(argv[0]);
    g_options->AddDefaultSearchPathEntries();
    g_options->Parse(argc, argv);
    EmitExecutionMessage("beginning execution");
    g_options->ProcessSearchPath();
    
    if (TrisonOptions().Abort())
    {
        exit(RS_COMMANDLINE_ABORT);
    }
    else if (TrisonOptions().IsHelpRequested())
    {
        TrisonOptions().PrintHelpMessage(cerr);
        exit(RS_SUCCESS);
    }
    else if (TrisonOptions().GetPrintSearchPathRequest() == Trison::Options::PSPR_SHORT)
    {
        cout << TrisonOptions().GetSearchPath().AsString("\n") << endl;
        exit(RS_SUCCESS);
    }
    else if (TrisonOptions().GetPrintSearchPathRequest() == Trison::Options::PSPR_VERBOSE)
    {
        cout << TrisonOptions().GetSearchPath().AsVerboseString("\n") << endl;
        exit(RS_SUCCESS);
    }
}

Trison::PrimarySource const *ParsePrimarySource ()
{
    // parse the primary source and check everything possible without
    // having parsed the targetspec sources.  if any errors were
    // accumulated during this section, abort with an error code.

    Trison::PrimarySource *primary_source = NULL;
    Ast::Base *parsed_tree_root = NULL;
    
    Trison::Parser parser;
    parser.ScannerDebugSpew(TrisonOptions().IsVerbose(Trison::Options::V_PRIMARY_SOURCE_SCANNER));
    parser.DebugSpew(TrisonOptions().IsVerbose(Trison::Options::V_PRIMARY_SOURCE_PARSER));

    // go through the predefine commandline directives and parse them.
    for (vector<string>::size_type i = 0; i < TrisonOptions().PredefineCount(); ++i)
    {
        parser.OpenString(TrisonOptions().Predefine(i), "<predefine>");

        if (parser.Parse(&parsed_tree_root, Trison::Parser::ParseNonterminal::target_directive) != Trison::Parser::PRC_SUCCESS)
            EmitError("general reflex parse error (in predefine) -- " + TrisonOptions().HowtoReportError());
        else if (!g_errors_encountered)
        {
            CommonLang::TargetDirective *target_directive = Dsc<CommonLang::TargetDirective *>(parsed_tree_root);
            parser.GetTargetMap().SetTargetDirective(target_directive);
        }
        parsed_tree_root = NULL;
    }

    if (g_errors_encountered)
        exit(RS_PREDEFINE_ERROR);

    if (!parser.OpenFile(TrisonOptions().InputFilename()))
        EmitError("file not found: \"" + TrisonOptions().InputFilename() + "\"");

    if (g_errors_encountered)
        exit(RS_INPUT_FILE_ERROR);
        
    if (parser.Parse(&parsed_tree_root) != Trison::Parser::PRC_SUCCESS)
        EmitError("general trison parse error -- " + TrisonOptions().HowtoReportError(), FiLoc(TrisonOptions().InputFilename()));
    else if (g_errors_encountered)
        exit(RS_PRIMARY_SOURCE_ERROR);
    else
    {
        primary_source = Dsc<Trison::PrimarySource *>(parsed_tree_root);
        assert(primary_source != NULL);
    }

    // go through the postdefine commandline directives and parse them.
    for (vector<string>::size_type i = 0; i < TrisonOptions().PostdefineCount(); ++i)
    {
        parser.OpenString(TrisonOptions().Postdefine(i), "<postdefine>");

        if (parser.Parse(&parsed_tree_root, Trison::Parser::ParseNonterminal::target_directive) != Trison::Parser::PRC_SUCCESS)
            EmitError("general reflex parse error (in postdefine) -- " + TrisonOptions().HowtoReportError());
        else if (!g_errors_encountered)
        {
            CommonLang::TargetDirective *target_directive = Dsc<CommonLang::TargetDirective *>(parsed_tree_root);
            parser.GetTargetMap().SetTargetDirective(target_directive);
        }
        parsed_tree_root = NULL;
    }

    if (g_errors_encountered)
        exit(RS_POSTDEFINE_ERROR);

    primary_source->SetTargetMap(parser.StealTargetMap());
    if (TrisonOptions().IsVerbose(Trison::Options::V_PRIMARY_SOURCE_AST))
        primary_source->Print(cerr);
            
    return primary_source;
}

void PrintDotGraph (Graph const &graph, string const &filename, string const &graph_name)
{
    if (filename.empty())
        return;

    if (filename == "-")
    {
        EmitExecutionMessage("printing " + graph_name + " graph to <stdout>");
        graph.PrintDotGraph(cout, graph_name);
    }
    else
    {
        EmitExecutionMessage("opening file \"" + filename + "\" for output of " + graph_name + " graph");
        ofstream file(filename.c_str());
        if (file.is_open())
        {
            EmitExecutionMessage("opened file \"" + filename + "\" successfully");
            graph.PrintDotGraph(file, graph_name);
        }
        else
            EmitError("could not open file \"" + filename + "\" for writing");
    }
}

void GenerateNpdaGraphAndPrintDotGraph (Trison::PrimarySource const &primary_source, Graph &npda_graph)
{
    // generate the NPDA and print it (if the filename is even specified).
    // no error is possible in this section.

    EmitExecutionMessage("generating NPDA graph");
    Trison::GenerateNpda(primary_source, npda_graph);
    EmitExecutionMessage("done generating NPDA graph");
    PrintDotGraph(npda_graph, TrisonOptions().NaDotGraphPath(), "NPDA");
}

void GenerateDpdaGraphAndPrintDotGraph (Trison::PrimarySource const &primary_source, Graph const &npda_graph, Graph &dpda_graph, Uint32 &lalr_lookahead_count)
{
    // generate the DPDA and print it (if the filename is even specified).
    // GenerateDpda may throw an exception, which should be interpreted
    // as a program error.  if any errors were accumulated during this
    // section, abort with an error code.

    try {
        EmitExecutionMessage("attempting to generate DPDA graph");
        Trison::GenerateDpda(primary_source, npda_graph, dpda_graph, lalr_lookahead_count);
        EmitExecutionMessage("generated DPDA graph successfully");
        PrintDotGraph(dpda_graph, TrisonOptions().DaDotGraphPath(), "DPDA");
    } catch (string const &exception) {
        EmitError(exception);
    }

    if (g_errors_encountered)
        exit(RS_DETERMINISTIC_AUTOMATON_GENERATION_ERROR);
}

void GenerateDpdaStatesFile (Trison::PrimarySource const &primary_source, Graph const &npda_graph, Graph const &dpda_graph, Uint32 lalr_lookahead_count)
{
    // generate the bison-like .states file associated with the DPDA parser.

    // TODO -- check if the dpda was actually generated

    string filename(TrisonOptions().DpdaStatesPath());

    if (filename.empty())
        return;

    if (filename == "-")
    {
        EmitExecutionMessage("printing DPDA states file to <stdout>");
        Trison::PrintDpdaStatesFile(primary_source, npda_graph, dpda_graph, lalr_lookahead_count, cout);
    }
    else
    {
        EmitExecutionMessage("opening file \"" + filename + "\" for output of DPDA states file");
        ofstream file(filename.c_str());
        if (file.is_open())
        {
            EmitExecutionMessage("opened file \"" + filename + "\" successfully");
            Trison::PrintDpdaStatesFile(primary_source, npda_graph, dpda_graph, lalr_lookahead_count, file);
        }
        else
            EmitError("could not open file \"" + filename + "\" for writing");
    }
}

void ParseTargetspecs (Trison::PrimarySource const &primary_source)
{
    // for each target in the primary source, parse the corresponding
    // targetspec source, checking for required directives, doing everything
    // possible to check short of running the run-before-code-gen code.
    // if any errors were accumulated during this section, abort with
    // an error code.

    Targetspec::Parser parser;
    parser.ScannerDebugSpew(TrisonOptions().IsVerbose(Trison::Options::V_TARGETSPEC_SCANNER));
    parser.DebugSpew(TrisonOptions().IsVerbose(Trison::Options::V_TARGETSPEC_PARSER));

    for (CommonLang::TargetMap::const_iterator it = primary_source.GetTargetMap().begin(),
                                               it_end = primary_source.GetTargetMap().end();
         it != it_end;
         ++it)
    {
        CommonLang::Target const *target = it->second;
        assert(target != NULL);
        target->ParseTargetspec(parser);
    }

    if (g_errors_encountered)
        exit(RS_TARGETSPEC_ERROR);
}

void ParseCodespecs (Trison::PrimarySource const &primary_source)
{
    // for each codespec in every target, parse the codespec and
    // make all checks possible at this time.  if any errors were
    // accumulated during this section, abort with an error code.

    Preprocessor::Parser parser;
    parser.ScannerDebugSpew(TrisonOptions().IsVerbose(Trison::Options::V_CODESPEC_SCANNER));
    parser.DebugSpew(TrisonOptions().IsVerbose(Trison::Options::V_CODESPEC_PARSER));

    for (CommonLang::TargetMap::const_iterator it = primary_source.GetTargetMap().begin(),
                                               it_end = primary_source.GetTargetMap().end();
         it != it_end;
         ++it)
    {
        CommonLang::Target const *target = it->second;
        assert(target != NULL);
        EmitExecutionMessage("parsing codespecs for target \"" + target->m_target_id + "\"");
        target->ParseCodespecs(parser);
        EmitExecutionMessage("done parsing codespecs for target \"" + target->m_target_id + "\"");
    }

    if (g_errors_encountered)
        exit(RS_CODESPEC_ERROR);
}

void WriteTargets (Trison::PrimarySource const &primary_source, Graph const &npda_graph, Graph const &dpda_graph)
{
    // for each target, fill in an empty Preprocessor::SymbolTable with the
    // macros to be used by the codespecs, and execute each codespec, each
    // with its own copy of the symbol table.  if any errors were accumulated
    // during this section, abort with an error code.

    Preprocessor::SymbolTable global_symbol_table;

    Trison::GenerateGeneralAutomatonSymbols(primary_source, global_symbol_table);
    Trison::GenerateNpdaSymbols(primary_source, npda_graph, global_symbol_table);
    // Trison::GenerateDpdaSymbols(primary_source, dpda_graph, global_symbol_table);

    for (CommonLang::TargetMap::const_iterator it = primary_source.GetTargetMap().begin(),
                                               it_end = primary_source.GetTargetMap().end();
         it != it_end;
         ++it)
    {
        string const &target_id = it->first;
        CommonLang::Target const *target = it->second;
        assert(target != NULL);

        Preprocessor::SymbolTable local_symbol_table(global_symbol_table);

        Trison::GenerateTargetDependentSymbols(primary_source, target_id, local_symbol_table);
        if (TrisonOptions().IsVerbose(Trison::Options::V_CODESPEC_SYMBOLS))
            local_symbol_table.Print(cerr);

        target->GenerateCode(local_symbol_table);
    }

    if (g_errors_encountered)
        exit(RS_WRITE_TARGETS_ERROR);
}

int main (int argc, char **argv)
{
    try {
        Trison::PrimarySource const *primary_source = NULL;
        Graph npda_graph, dpda_graph;
        // Uint32 lalr_lookahead_count = static_cast<Uint32>(-1);

        ParseAndHandleOptions(argc, argv);
        primary_source = ParsePrimarySource();
        GenerateNpdaGraphAndPrintDotGraph(*primary_source, npda_graph);
        // GenerateDpdaGraphAndPrintDotGraph(*primary_source, npda_graph, dpda_graph, lalr_lookahead_count);
        // GenerateDpdaStatesFile(*primary_source, npda_graph, dpda_graph, lalr_lookahead_count);
        ParseTargetspecs(*primary_source);
        ParseCodespecs(*primary_source);
        WriteTargets(*primary_source, npda_graph, dpda_graph);

        delete primary_source;
        EmitExecutionMessage("ending execution successfully");
        return RS_SUCCESS;
    } catch (string const &exception) {
        // this is the catch block for fatal errors
        cerr << exception << endl;
        return RS_FATAL_ERROR;
    }
}

