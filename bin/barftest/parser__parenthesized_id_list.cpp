// 2016.08.14 - Victor Dods

#include "barftest/sem/ErrorDummy.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__16__parenthesized_id_list__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "(a b c)" },
        Parser::Nonterminal::parenthesized_id_list,
        sem::make_statement_list(
            sem::make_identifier("a"),
            sem::make_identifier("b"),
            sem::make_identifier("c")
        ),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__16__parenthesized_id_list__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "(a)" },
        Parser::Nonterminal::parenthesized_id_list,
        sem::make_statement_list(
            sem::make_identifier("a")
        ),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__16__parenthesized_id_list__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "()" },
        Parser::Nonterminal::parenthesized_id_list,
        sem::make_statement_list(),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__16__parenthesized_id_list__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "(,)" },
        Parser::Nonterminal::parenthesized_id_list,
        sem::make_statement_list(sem::make_error_dummy()),
        2,
        1,
        2,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__16__parenthesized_id_list__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        { "(a b ,)" },
        Parser::Nonterminal::parenthesized_id_list,
        sem::make_statement_list(
            sem::make_identifier("a"),
            sem::make_identifier("b"),
            sem::make_error_dummy()
        ),
        2,
        1,
        2,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
