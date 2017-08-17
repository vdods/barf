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
#include <iostream>
#include "log.hpp"
#include "Scanner.hpp"
#include <sstream>

void scan_stuff (std::string const &input_string)
{
    Scanner scanner;

    std::istringstream in(input_string);
    scanner.IstreamIterator(std::istream_iterator<char>(in));

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

int main (int argc, char **argv)
{
    std::string input_string;
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <input-string>\n";
        return -1;
    }
    scan_stuff(argv[1]);
    return 0;
}
