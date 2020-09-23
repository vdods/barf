// 2016.08.14 - Victor Dods

#include "sem/ErrorDummy.hpp"
#include "sem/Identifier.hpp"
#include "sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__14__id_list_then_end__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "a b c" },
        Parser::Nonterminal::id_list_then_end,
        sem::make_statement_list(
            sem::make_identifier("a"),
            sem::make_identifier("b"),
            sem::make_identifier("c")
        ),
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__14__id_list_then_end__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "a" },
        Parser::Nonterminal::id_list_then_end,
        sem::make_statement_list(
            sem::make_identifier("a")
        ),
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__14__id_list_then_end__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "" },
        Parser::Nonterminal::id_list_then_end,
        sem::make_statement_list(),
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__14__id_list_then_end__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "," },
        Parser::Nonterminal::id_list_then_end,
        sem::make_statement_list(sem::make_error_dummy()),
        3,
        1,
        2,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__14__id_list_then_end__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "a b ," },
        Parser::Nonterminal::id_list_then_end,
        sem::make_statement_list(
            sem::make_identifier("a"),
            sem::make_identifier("b"),
            sem::make_error_dummy()
        ),
        3,
        1,
        2,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

} // end namespace text
} // end namespace cbz
