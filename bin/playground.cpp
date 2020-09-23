// 2009.06.20 - Copyright Victor Dods - Licensed under Apache 2.0

#include <iostream>

#include "barf_optionsbase.hpp"
#include "barf_regex_parser.hpp"

using namespace Barf;
using namespace std;

// unnamed namespace to hide g_options from other files
namespace {
OptionsBase *g_options = NULL;
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
    // NOTE: It doesn't matter that g_options is left uninitialized, since
    // Regex::Parser doesn't use it.

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
            regex->TopLevelPrint(cerr);
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
        regex->TopLevelPrint(cerr);
        delete regex;
    }

    return 0;
}
