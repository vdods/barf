// 2016.08.14 - Victor Dods

#include "barftest/sem/ErrorDummy.hpp"
#include "barftest/sem/Function.hpp"
#include "barftest/sem/FunctionPrototype.hpp"
#include "barftest/sem/Return.hpp"
#include "barftest/sem/Value.hpp"
#include "barftest/sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__02__expression_then_end__000)
    // Testing %nonassoc error handling
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x == y == z"},
        Parser::Nonterminal::expression_then_end,
        sem::make_error_dummy(),
        4,
        1,
        3,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__02__expression_then_end__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x < y < z"},
        Parser::Nonterminal::expression_then_end,
        sem::make_error_dummy(),
        4,
        1,
        3,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__02__expression_then_end__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"() -> Sint64 { return 45954 }", "(\n) ->\nSint64 {\nreturn 45954\n}"},
        Parser::Nonterminal::expression_then_end,
        sem::make_function(
            sem::make_function_prototype(
                sem::make_declaration_tuple(),
                sem::make_sint64()
            ),
            sem::make_statement_list(
                sem::make_return(sem::make_sint64_value(45954))
            )
        ),
        7
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
