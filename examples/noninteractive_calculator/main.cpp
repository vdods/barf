// 2017.08.17 - Copyright Victor Dods - Licensed under Apache 2.0

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
    parser.SetActiveDebugSpewFlags(CalcParser::DSF__MINIMAL_VERBOSE);
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

    std::cout << "parser returned " << return_code << '\n';
    std::cout << "parsed value is:\n";
    if (parsed_value != nullptr)
        parsed_value->print(std::cout, 1);
    else
        std::cout << "    " << parsed_value.get() << '\n';

    return 0;
}
