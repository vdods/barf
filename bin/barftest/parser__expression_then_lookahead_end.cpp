// 2016.08.14 - Victor Dods

#include "barftest/sem/BinaryOperation.hpp"
#include "barftest/sem/ErrorDummy.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/UnaryOperation.hpp"
#include "barftest/sem/Value.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__01__expression_then_lookahead_end__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"TRUE"},
        Parser::Nonterminal::expression_then_lookahead_end,
        sem::make_boolean_value(true),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__01__expression_then_lookahead_end__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"-123"},
        Parser::Nonterminal::expression_then_lookahead_end,
        sem::make_unary_operation(
            FiRange::INVALID,
            sem::UnaryOperationType::NEGATE,
            sem::make_sint64_value(123)
        ),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__01__expression_then_lookahead_end__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x+y", "x\\\n+y"},
        Parser::Nonterminal::expression_then_lookahead_end,
        sem::make_binary_operation(sem::BinaryOperationType::ADD, sem::make_identifier("x"), sem::make_identifier("y")),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__01__expression_then_lookahead_end__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"3 + 4 + 5", "3+ 4+ 5", "3+4+5", "3 + 4 + 5", "3+\n4+\n5", "(3 + 4) + 5"},
        Parser::Nonterminal::expression_then_lookahead_end,
        sem::make_binary_operation(
            sem::BinaryOperationType::ADD,
            sem::make_binary_operation(
                sem::BinaryOperationType::ADD,
                sem::make_sint64_value(3),
                sem::make_sint64_value(4)
            ),
            sem::make_sint64_value(5)
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__01__expression_then_lookahead_end__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x == y == z"},
        Parser::Nonterminal::expression_then_lookahead_end,
        sem::make_error_dummy(),
        4,
        1,
        3,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__01__expression_then_lookahead_end__005)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x < y < z"},
        Parser::Nonterminal::expression_then_lookahead_end,
        sem::make_error_dummy(),
        4,
        1,
        3,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
