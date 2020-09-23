// 2016.08.14 - Victor Dods

#include "barftest/sem/BinaryOperation.hpp"
#include "barftest/sem/Identifier.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

//
// testing higher lookahead count parses
//

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><><> y <><><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::TIN,
            sem::make_binary_operation(
                sem::BinaryOperationType::TIN,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        7,
        4, // It must see the whole <><><><> in <>y <><><><> z<>
        5
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><><> y <><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::TIN,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::DINSDALE,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        7,
        4, // It must see one past the <><><> in <>y <><><> z<>
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><><> y <><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::TIN,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::SPLUNGE,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        6,
        3, // It must see one past the <><> in <>y <><> z<>
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><><> y <> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::TIN,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::WOOD,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        5,
        2, // It must see one past the <> in <>y <> z<>
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><> y <><><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::TIN,
            sem::make_binary_operation(
                sem::BinaryOperationType::DINSDALE,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        4,
        1 // Since <><><> has higher precedence than all the others
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__005)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><> y <><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::DINSDALE,
            sem::make_binary_operation(
                sem::BinaryOperationType::DINSDALE,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        4,
        1 // Since <><><> has higher precedence than all the others
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__006)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><> y <><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::SPLUNGE,
            sem::make_binary_operation(
                sem::BinaryOperationType::DINSDALE,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        4,
        1 // Since <><><> has higher precedence than all the others
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__007)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><><> y <> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::WOOD,
            sem::make_binary_operation(
                sem::BinaryOperationType::DINSDALE,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        4,
        1 // Since <><><> has higher precedence than all the others
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__008)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><> y <><><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::TIN,
            sem::make_binary_operation(
                sem::BinaryOperationType::SPLUNGE,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        7,
        4, // Because it has to "see past" the possibility of <>y <><><> z<>
        5
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__009)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><> y <><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::SPLUNGE,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::DINSDALE,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        7,
        4, // Because it has to see far enough to rule out <>y <><><><> z<>
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__010)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><> y <><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::SPLUNGE,
            sem::make_binary_operation(
                sem::BinaryOperationType::SPLUNGE,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        6,
        3,
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__011)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <><> y <> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::SPLUNGE,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::WOOD,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        5,
        2,
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__012)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <> y <><><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::TIN,
            sem::make_binary_operation(
                sem::BinaryOperationType::WOOD,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        7,
        4, // Because it has to "see past" the possibility of <>y <><><> z<>
        5
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__013)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <> y <><><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::WOOD,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::DINSDALE,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        7,
        4, // Because it has to rule out <>y <><><><> z<>
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__014)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <> y <><> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::SPLUNGE,
            sem::make_binary_operation(
                sem::BinaryOperationType::WOOD,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        6,
        3, // Because it has to rule out <>y <><><> z<>
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__03__lookahead_test_expression__015)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x <> y <> z"},
        Parser::Nonterminal::lookahead_test_expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::WOOD,
            sem::make_binary_operation(
                sem::BinaryOperationType::WOOD,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        5,
        2,
        3
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
