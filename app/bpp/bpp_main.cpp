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

// these globals are required by barf_message.h
OptionsBase *g_options = NULL;
bool g_errors_encountered = false;

int main (int argc, char **argv)
{
    try
    {
        g_options = new Bpp::Options(argv[0]);
        GetOptions().Parse(argc, argv);
        if (GetOptions().GetAbort())
            return 1;
        else if (GetOptions().GetIsHelpRequested())
        {
            GetOptions().PrintHelpMessage(cerr);
            return 0;
        }

        Preprocessor::Parser parser;
        parser.ScannerDebugSpew(Bpp::Options::V_PRIMARY_SOURCE_SCANNER);
        parser.DebugSpew(GetOptions().GetIsVerbose(Bpp::Options::V_PRIMARY_SOURCE_PARSER));

        if (GetOptions().GetInputFilename() == "-" || GetOptions().GetInputFilename().empty())
        {
            GetOptions().SetInputFilename("<stdin>");
            parser.OpenUsingStream(&cin, GetOptions().GetInputFilename(), true);
        }

        else if (!parser.OpenFile(GetOptions().GetInputFilename()))
        {
            EmitError("file not found: \"" + GetOptions().GetInputFilename() + "\"");
            return 2;
        }

        ostream *out = NULL;
        ofstream out_fstream;
        if (GetOptions().GetOutputFilename() == "-" || GetOptions().GetOutputFilename().empty())
        {
            GetOptions().SetOutputFilename("<stdout>");
            out = &cout;
        }
        else
        {
            out_fstream.open(GetOptions().GetOutputFilename().c_str(), ofstream::out|ofstream::trunc);
            if (!out_fstream.is_open())
            {
                EmitError("unable to open file \"" + GetOptions().GetOutputFilename() + "\" for writing");
                return 3;
            }
            out = &out_fstream;
        }
        assert(out != NULL);

        Ast::Base *parsed_tree_root = NULL;
        if (parser.Parse(&parsed_tree_root) != Preprocessor::Parser::PRC_SUCCESS)
        {
            EmitError("general preprocessor parse error", FiLoc(GetOptions().GetInputFilename()));
            return 4;
        }

        if (g_errors_encountered)
            return 5;

        Preprocessor::Body const *body = Dsc<Preprocessor::Body const *>(parsed_tree_root);
        assert(body != NULL);

        if (GetOptions().GetIsVerbose(Bpp::Options::V_PRIMARY_SOURCE_AST))
            body->Print(cerr);

        Preprocessor::Textifier textifier(*out, GetOptions().GetOutputFilename());
        textifier.SetGeneratesLineDirectives(false);
        Preprocessor::SymbolTable symbol_table;
        body->Execute(textifier, symbol_table);
        if (g_errors_encountered)
            return 6;

        delete body;
    }
    catch (string const &exception)
    {
        cerr << exception << endl;
        return 7;
    }

    return 0;
}
