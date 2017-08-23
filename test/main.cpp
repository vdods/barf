// ///////////////////////////////////////////////////////////////////////////
// main.cpp by Victor Dods, created 2017/08/17
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "ast.hpp"
#include "CalcParser.hpp"
#include <iostream>
#include "log.hpp"
#include <memory>
#include "Scanner.hpp"
#include <sstream>

void scan_stuff (std::string const &input_string)
{
    Scanner scanner;

    std::istringstream in(input_string);
    scanner.AttachIstream(in);

    TokenId token_id;;
    std::shared_ptr<Ast::Base> token;

    do
    {
        token_id = scanner.Scan(token);
        std::cout << AsString(token_id) << ", ";
        if (token != nullptr)
            std::cout << *token;
        else
            std::cout << "nullptr";
        std::cout << '\n';
    }
    while (token_id != TokenId::END_OF_FILE);

    std::cout << '\n';
    std::cout << "log of errors:\n";
    std::cout << g_log.str() << '\n';
}

CalcParser::ParserReturnCode parse_stuff (std::string const &input_string, std::shared_ptr<Ast::Base> &parsed_value)
{
    std::istringstream in(input_string);
    CalcParser parser;
    parser.AttachIstream(in);
    parser.SetDebugSpewStream(&std::cerr);
    CalcParser::ParserReturnCode return_code = parser.Parse(&parsed_value);
    return return_code;
}

int main (int argc, char **argv)
{
    std::string input_string;
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <input-string>\n";
        return -1;
    }
    // scan_stuff(argv[1]);
    std::shared_ptr<Ast::Base> parsed_value;
    CalcParser::ParserReturnCode return_code = parse_stuff(argv[1], parsed_value);

    switch (return_code)
    {
        case CalcParser::PRC_SUCCESS:               std::cout << "parser returned PRC_SUCCESS\n"; break;
        case CalcParser::PRC_UNHANDLED_PARSE_ERROR: std::cout << "parser returned PRC_UNHANDLED_PARSE_ERROR\n"; break;
        case CalcParser::PRC_INTERNAL_ERROR:        std::cout << "parser returned PRC_INTERNAL_ERROR\n"; break;
    }

    std::cout << "parsed value is:\n";
    if (parsed_value != nullptr)
        parsed_value->print(std::cout, 1);
    else
        std::cout << "    " << parsed_value.get() << '\n';

    return 0;
}
