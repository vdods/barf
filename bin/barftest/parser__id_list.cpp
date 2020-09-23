// 2016.08.14 - Victor Dods

#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__15__id_list__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "a b c" },
        Parser::Nonterminal::id_list,
        sem::make_statement_list(
            sem::make_identifier("a"),
            sem::make_identifier("b"),
            sem::make_identifier("c")
        ),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__15__id_list__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "a" },
        Parser::Nonterminal::id_list,
        sem::make_statement_list(
            sem::make_identifier("a")
        ),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__15__id_list__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "" },
        Parser::Nonterminal::id_list,
        sem::make_statement_list(),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__15__id_list__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "," },
        Parser::Nonterminal::id_list,
        sem::make_statement_list(), // No error dummy here because it returns before reducing the error.
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__15__id_list__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "a b ," },
        Parser::Nonterminal::id_list,
        sem::make_statement_list(
            sem::make_identifier("a"),
            sem::make_identifier("b")
             // No error dummy here because it returns before reducing the error.
        ),
        2
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
