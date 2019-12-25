// ///////////////////////////////////////////////////////////////////////////
// using_shared_ptr_main.cpp by Victor Dods, created 2019/12/24
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <ios>
#include <sstream>
#include "using_shared_ptr_ast.hpp"
#include "using_shared_ptr_parser.hpp"
#include "using_shared_ptr_scanner.hpp"
#include <vector>

int parse_stream (std::istream &in) {
    Parser parser;
    parser.set_istream_iterator(std::istream_iterator<char>(in));
//     parser.SetActiveDebugSpewFlags(Parser::DSF__MINIMAL);
//     parser.SetDebugSpewStream(&std::cerr);
//     parser.scanner().SetDebugSpewStream(&std::cerr);
    std::shared_ptr<Base> root;
    Parser::ParserReturnCode return_code = parser.Parse(&root);
    switch (return_code)
    {
        case Parser::PRC_SUCCESS:
            std::cout << "Parse succeeded:\n\n";
            root->print(std::cout, 1);
            std::cout << '\n';
            return 0;

        default:
            std::cerr << "error: " << return_code << '\n';
            return 1;
    }
}

int main (int argc, char **argv) {
    if (argc == 1) {
        // Run unit tests
        std::vector<std::string> test_strings{
            "",
            "a",
            "a b",
            "()",
            "(a)",
            "(a b)",
            "(a)(b)",
            "(a (b))",
            "(a (b) c)",
            "(a b (c) d (e))",
            "a b (c d (e) (f g ())) h (i)",
        };
        for (auto const &test_string : test_strings) {
            std::cout << "Parsing string \"" << test_string << "\" ...\n\n";
            std::istringstream in(test_string);
            in.unsetf(std::ios_base::skipws); // in skips whitespace by default; turn that off.
            auto ret = parse_stream(in);
            if (ret != 0)
                return ret;
        }
    } else if (argc == 2) {
        if (std::string(argv[1]) == "-") {
            // Use std::cin
            std::cout << "Parsing from stdin ...\n\n";
            std::cin.unsetf(std::ios_base::skipws); // std::cin skips whitespace by default; turn that off.
            return parse_stream(std::cin);
        } else {
            // The arg is a filename to be read in and parsed.
            std::cout << "Parsing file \"" << argv[1] << "\" ...\n\n";
            std::ifstream in(argv[1]);
            in.unsetf(std::ios_base::skipws); // in skips whitespace by default; turn that off.
            return parse_stream(in);
        }
    } else {
        std::cerr << "Usage: " << argv[0] << " [input-filename]\n\nIf [input-filename] is omitted, unit tests are run.  If [input-filename] is - then stdin is used.\n";
        return -1;
    }
}
