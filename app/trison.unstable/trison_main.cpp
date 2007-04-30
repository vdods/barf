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

#include "barf_langspec_ast.hpp"
#include "barf_langspec_parser.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_parser.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "trison_ast.hpp"
#include "trison_options.hpp"
#include "trison_parser.hpp"

// these globals are required by barf_message.h
OptionsBase *g_options = NULL;
bool g_errors_encountered = false;

namespace Trison {

// required by trison_message.h
bool g_conflicts_encountered = false;

} // end of namespace Trison

int main (int argc, char **argv)
{
    try
    {
        g_options = new Trison::Options(argv[0]);
        g_options->Parse(argc, argv);
        if (GetOptions()->GetAbort())
            return 1;
        else if (GetOptions()->GetIsHelpRequested())
        {
            GetOptions()->PrintHelpMessage(cerr);
            return 0;
        }

        Trison::Representation *representation = NULL;

        // parse the primary source and check everything possible without
        // having parsed the langspec sources.
        {
            Trison::Parser parser;
            if (GetOptions()->GetShowScanningSpew())
                parser.ScannerDebugSpew(true);
            if (GetOptions()->GetShowParsingSpew())
                parser.SetDebugSpewLevel(2);

            if (!parser.OpenFile(GetOptions()->GetInputFilename()))
                EmitError("file not found: \"" + GetOptions()->GetInputFilename() + "\"");
            else if (parser.Parse() != Trison::Parser::PRC_SUCCESS)
                EmitError(FiLoc(GetOptions()->GetInputFilename()), "general trison parse error");
            else
            {
                representation = Dsc<Trison::Representation *>(parser.GetAcceptedToken());
                assert(representation != NULL);
                if (GetOptions()->GetShowSyntaxTree())
                    representation->Print(cerr);
            }

            if (g_errors_encountered)
                return 3;

            representation->GenerateNpdaAndDpda();
            representation->PrintNpdaGraph(GetOptions()->GetNaDotGraphPath(), "NPDA");
            representation->PrintDpdaGraph(GetOptions()->GetDaDotGraphPath(), "DPDA");

            if (g_errors_encountered)
                return 4;
        }

        // for each language in the primary source, parse the corresponding
        // langspec source, checking for required directives, doing everything
        // possible to check short of running the run-before-code-gen code.
        {
            LangSpec::Parser parser;
            if (GetOptions()->GetShowLangSpecParsingSpew())
                parser.SetDebugSpewLevel(2);

            for (CommonLang::TargetLanguageMap::const_iterator it = representation->m_target_language_map->begin(),
                                                               it_end = representation->m_target_language_map->end();
                 it != it_end;
                 ++it)
            {
                CommonLang::TargetLanguage const *target_language = it->second;
                assert(target_language != NULL);
                target_language->ParseLangSpec("trison", parser);
            }
        }
        if (g_errors_encountered)
            return 5;

        // for each codespec in every target language, parse the
        // codespec and make all checks possible at this time.
        {
            Preprocessor::Parser parser;
            if (GetOptions()->GetShowPreprocessorParsingSpew())
                parser.SetDebugSpewLevel(2);

            for (CommonLang::TargetLanguageMap::const_iterator it = representation->m_target_language_map->begin(),
                                                               it_end = representation->m_target_language_map->end();
                 it != it_end;
                 ++it)
            {
                CommonLang::TargetLanguage const *target_language = it->second;
                assert(target_language != NULL);
                target_language->ParseCodeSpecs("trison", parser);
            }
        }
        if (g_errors_encountered)
            return 6;

        // for each target language, fill in an empty Preprocessor::SymbolTable
        // with the macros to be used by the codespecs, and execute each codespec,
        // each with its own copy of the symbol table.
        {
            Preprocessor::SymbolTable global_symbol_table;
            representation->GenerateAutomatonSymbols(global_symbol_table);

            for (CommonLang::TargetLanguageMap::const_iterator it = representation->m_target_language_map->begin(),
                                                               it_end = representation->m_target_language_map->end();
                 it != it_end;
                 ++it)
            {
                string const &target_language_identifier = it->first;
                CommonLang::TargetLanguage const *target_language = it->second;
                assert(target_language != NULL);

                Preprocessor::SymbolTable local_symbol_table(global_symbol_table);

                representation->GenerateTargetLanguageDependentSymbols(target_language_identifier, local_symbol_table);
                target_language->GenerateCode(local_symbol_table);
            }
        }
        if (g_errors_encountered)
            return 7;

        delete representation;
    }
    catch (string const &exception)
    {
        cerr << exception << endl;
        return 8;
    }

    return 0;
}

