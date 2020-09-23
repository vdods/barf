// 2019.12.22 - Copyright Victor Dods - Licensed under Apache 2.0

#include <fstream>
#include <ios>
#include <sstream>
#include "using_unique_ptr_ast.hpp"
#include "using_unique_ptr_parser.hpp"
#include "using_unique_ptr_scanner.hpp"
#include <vector>

int parse_stream (std::istream &in, bool &recoverable_error_encountered) {
    Parser parser;
    parser.set_istream_iterator(std::istream_iterator<char>(in));
    parser.SetActiveDebugSpewFlags(Parser::DSF__MINIMAL);
//     parser.SetDebugSpewStream(&std::cerr);
//     parser.scanner().SetDebugSpewStream(&std::cerr);
    std::unique_ptr<Base> parsed_root;
    Parser::ParserReturnCode return_code = parser.Parse(&parsed_root);
    recoverable_error_encountered = parser.recoverable_error_encountered();
    switch (return_code)
    {
        case Parser::PRC_SUCCESS:
            std::cout << "Parse succeeded:\n\n";
            parsed_root->print(std::cout, 1);
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
        std::vector<std::string> positive_test_strings{
            "",
            " ",
            "a",
            "_",
            " a ",
            "a b",
            "aaa bbb",
            "()",
            "()()",
            "()()()",
            "(())",
            "(a)",
            "(___)",
            "((a))",
            "(a b)",
            "(a)(b)",
            "(a (b))",
            "(a (b) c)",
            "(a b (c) d (e))",
            "a b (c d (e) (f g ())) h (i)",
        };
        for (auto const &s : positive_test_strings) {
            std::cout << "Parsing string \"" << s << "\" ...\n\n";
            std::istringstream in(s);
            in.unsetf(std::ios_base::skipws); // in skips whitespace by default; turn that off.
            bool recoverable_error_encountered = false;
            auto ret = parse_stream(in, recoverable_error_encountered);
            if (ret != 0)
                return ret;
            if (recoverable_error_encountered) {
                std::cerr << "error: recoverable error encountered, but it was not expected to.\n";
                return -1;
            }
        }
        std::vector<std::string> negative_test_strings{
            "#",
            "(#)",
            "(",
            "(#",
            "(3",
            "((",
            "((#",
            "((3",
            "(#(",
            "(3(",
            "(#(#",
            "(3(#",
            "(3(3",
            "(#(3",
            ")",
            ")#",
            ")3",
            "#)",
            "3)",
            "#)#",
            "3)#",
            "3)3",
            "#)3",
            "))",
            ")#)",
            ")3)",
            "))#",
            "))3",
            ")#)#",
            ")3)#",
            ")3)3",
            ")#)3",
            "#)#)#",
            "3)#)#",
            "3)3)#",
            "3)3)3",
            ")(",
            ")#(",
            ")3(",
            ")(#",
            ")(3",
            "#)(#",
            "3)(#",
            "3)(3",
            "#)(3",
        };
        for (auto const &s : negative_test_strings) {
            std::cout << "Parsing string \"" << s << "\" (recoverable error is expected)...\n\n";
            std::istringstream in(s);
            in.unsetf(std::ios_base::skipws); // in skips whitespace by default; turn that off.
            bool recoverable_error_encountered = false;
            auto ret = parse_stream(in, recoverable_error_encountered);
            if (ret != 0)
                return ret;
            if (!recoverable_error_encountered) {
                std::cerr << "error: no recoverable error encountered, but it was expected to.\n";
                return -1;
            }
        }
        std::cout << "\nALL UNIT TESTS PASSED (" << positive_test_strings.size() << " positive tests, " << negative_test_strings.size() << " negative tests)\n";
        return 0;
    } else if (argc == 2) {
        bool recoverable_error_encountered = false;
        int ret = 0;
        if (std::string(argv[1]) == "-") {
            // Use std::cin
            std::cout << "Parsing from stdin ...\n\n";
            std::cin.unsetf(std::ios_base::skipws); // std::cin skips whitespace by default; turn that off.
            ret = parse_stream(std::cin, recoverable_error_encountered);
        } else {
            // The arg is a filename to be read in and parsed.
            std::cout << "Parsing file \"" << argv[1] << "\" ...\n\n";
            std::ifstream in(argv[1]);
            in.unsetf(std::ios_base::skipws); // in skips whitespace by default; turn that off.
            ret = parse_stream(in, recoverable_error_encountered);
        }
        if (ret != 0)
            return ret;
        if (recoverable_error_encountered) {
            std::cerr << "error: recoverable error encountered, but it was not expected to.\n";
            return -1;
        }
    } else {
        std::cerr << "Usage: " << argv[0] << " [input-filename]\n\nIf [input-filename] is omitted, unit tests are run.  If [input-filename] is - then stdin is used.\n";
        return -1;
    }
}
