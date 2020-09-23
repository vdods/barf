// 2016.08.14 - Victor Dods

#include "barftest/sem/PointerType.hpp"
#include "barftest/sem/TypeArray.hpp"
#include "barftest/sem/Value.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__04__type_expression__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"Boolean"},
        Parser::Nonterminal::type_expression,
        sem::make_boolean(),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__04__type_expression__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"Boolean"},
        Parser::Nonterminal::type_expression,
        sem::make_boolean(),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__04__type_expression__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"Sint64"},
        Parser::Nonterminal::type_expression,
        sem::make_sint64(),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__04__type_expression__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"Float64"},
        Parser::Nonterminal::type_expression,
        sem::make_float64(),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__04__type_expression__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"[Float64]", "[Float64,]", "[\nFloat64,\n]", "[Float64]*1"},
        Parser::Nonterminal::type_expression,
        sem::make_type_array(FiRange::INVALID, sem::make_float64(), 1),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__04__type_expression__005)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"[Float64,Sint64]", "[Float64,Sint64,]", "[\nFloat64,\nSint64,\n]"},
        Parser::Nonterminal::type_expression,
        sem::make_type_tuple(sem::make_float64(), sem::make_sint64()),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__04__type_expression__006)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "Sint64@",
        },
        Parser::Nonterminal::type_expression,
        sem::make_pointer_type(FiRange::INVALID, sem::make_sint64()),
        9
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
