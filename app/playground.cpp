// ///////////////////////////////////////////////////////////////////////////
// playground.cpp by Victor Dods, created 2009/06/20
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "barf_regex_parser.hpp"

using namespace Barf;
using namespace std;

int main (int argc, char **argv)
{
    Regex::Parser parser;
    Regex::RegularExpression *regex = NULL;
    
    string input;
    getline(cin, input);
    parser.OpenString(input, "stdin");
    
    try {
        if (parser.Parse(&regex) != Regex::Parser::PRC_SUCCESS)
            cerr << "unhandled parse error" << endl;
        else
        {
            regex->Print(cerr);
            delete regex;
        }
    } catch (std::string const &exception) {
        cerr << exception << endl;
    }

    // try again to see if the parser was left in a good state
    parser.OpenString("no problem", "stdin");
    if (parser.Parse(&regex) != Regex::Parser::PRC_SUCCESS)
        cerr << "unhandled parse error (on \"no problem\")" << endl;
    else
    {
        regex->Print(cerr);
        delete regex;
    }

    return 0;
}
