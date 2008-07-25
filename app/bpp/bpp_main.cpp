// ///////////////////////////////////////////////////////////////////////////
// bpp_main.cpp by Victor Dods, created 2006/11/04
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "bpp.hpp"

#include <fstream>

#include "bpp_options.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_parser.hpp"
#include "barf_preprocessor_textifier.hpp"

// unnamed namespace to hide g_options from other files
namespace {
Bpp::Options *g_options = NULL;
}

// required by barf_optionsbase.hpp
bool OptionsAreInitialized () { return g_options != NULL; }
OptionsBase const &GetOptions ()
{
    assert(g_options != NULL && "g_options not initialized yet");
    return *g_options;
}

// required by barf_message.hpp
bool g_errors_encountered = false;

int main (int argc, char **argv)
{
    try
    {
        g_options = new Bpp::Options(argv[0]);
        g_options->Parse(argc, argv);
        if (GetBppOptions().GetAbort())
            return 1;
        else if (GetBppOptions().GetIsHelpRequested())
        {
            GetBppOptions().PrintHelpMessage(cerr);
            return 0;
        }

        EmitExecutionMessage("beginning execution");
        Preprocessor::Parser parser;
        parser.ScannerDebugSpew(GetBppOptions().GetIsVerbose(Bpp::Options::V_PRIMARY_SOURCE_SCANNER));
        parser.DebugSpew(GetBppOptions().GetIsVerbose(Bpp::Options::V_PRIMARY_SOURCE_PARSER));

        if (GetBppOptions().GetInputFilename() == "-" || GetBppOptions().GetInputFilename().empty())
        {
            EmitExecutionMessage("using <stdin> for input");
            g_options->SetInputFilename("<stdin>");
            parser.OpenUsingStream(&cin, GetBppOptions().GetInputFilename(), true);
        }
        else
        {
            EmitExecutionMessage("opening file \"" + GetBppOptions().GetInputFilename() + "\" for input");
            if (parser.OpenFile(GetBppOptions().GetInputFilename()))
            {
                EmitExecutionMessage("opened file \"" + GetBppOptions().GetInputFilename() + "\" successfully");
            }
            else
            {
                EmitError("file not found: \"" + GetBppOptions().GetInputFilename() + "\"");
                return 2;
            }
        }

        ostream *out = NULL;
        ofstream out_fstream;
        if (GetBppOptions().GetOutputFilename() == "-" || GetBppOptions().GetOutputFilename().empty())
        {
            EmitExecutionMessage("using <stdout> for output");
            g_options->SetOutputFilename("<stdout>");
            out = &cout;
        }
        else
        {
            EmitExecutionMessage("opening file \"" + GetBppOptions().GetOutputFilename() + "\" for output");
            out_fstream.open(GetBppOptions().GetOutputFilename().c_str(), ofstream::out|ofstream::trunc);
            if (out_fstream.is_open())
            {
                EmitExecutionMessage("opened file \"" + GetBppOptions().GetOutputFilename() + "\" successfully");
            }
            else
            {
                EmitError("unable to open file \"" + GetBppOptions().GetOutputFilename() + "\" for writing");
                return 3;
            }
            out = &out_fstream;
        }
        assert(out != NULL);

        Ast::Base *parsed_tree_root = NULL;
        if (parser.Parse(&parsed_tree_root) != Preprocessor::Parser::PRC_SUCCESS)
        {
            EmitError("general preprocessor parse error -- " + GetBppOptions().HowtoReportError(), FiLoc(GetBppOptions().GetInputFilename()));
            return 4;
        }

        if (g_errors_encountered)
            return 5;

        Preprocessor::Body const *body = Dsc<Preprocessor::Body const *>(parsed_tree_root);
        assert(body != NULL);

        if (GetBppOptions().GetIsVerbose(Bpp::Options::V_PRIMARY_SOURCE_AST))
            body->Print(cerr);

        EmitExecutionMessage("preprocessing input and generating output");
        Preprocessor::Textifier textifier(*out, GetBppOptions().GetOutputFilename());
        textifier.SetGeneratesLineDirectives(false);
        Preprocessor::SymbolTable symbol_table;
        body->Execute(textifier, symbol_table);
        if (g_errors_encountered)
            return 6;

        delete body;
        EmitExecutionMessage("ending execution successfully");
    }
    catch (string const &exception)
    {
        cerr << exception << endl;
        return 7;
    }

    return 0;
}
