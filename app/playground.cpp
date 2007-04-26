// ///////////////////////////////////////////////////////////////////////////
// playground.cpp by Victor Dods, created 2006/11/12
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf.hpp"

#include "barf_astcommon.hpp"
#include "barf_commonlang_scanner.hpp"
#include "barf_optionsbase.hpp"

using namespace Barf;

// these globals are required by barf_message.h
OptionsBase *g_options = NULL;
bool g_errors_encountered = false;

int main (int argc, char **argv)
{
    CommonLang::Scanner scanner;
//     scanner.SetDebugSpewFlags(CommonLang::Scanner::DEBUG_ALL);
    scanner.OpenUsingStream(&cin, "<stdin>", true);
    CommonLang::Scanner::Token::Type scanner_token_type;
    do
    {
        AstCommon::Ast *token = NULL;
        scanner_token_type = scanner.Scan(&token);
        cout << "*** scanner.Scan() returned " << scanner_token_type << endl;
        if (token != NULL)
            token->Print(cout, AstCommon::GetAstTypeString, 1);
        delete token;
    }
    while (scanner_token_type != CommonLang::Scanner::Token::END_OF_FILE &&
           scanner_token_type != CommonLang::Scanner::Token::BAD_END_OF_FILE);

    return 0;
}
