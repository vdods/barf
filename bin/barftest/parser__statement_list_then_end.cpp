// 2016.08.14 - Victor Dods

#include "barftest/sem/FunctionEvaluation.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__10__statement_list_then_end__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f(x);g(y);", "f(x); g(y);", "f(x)\ng(y);", "f(x);\ng(y);", "f(x); g(y)\n"},
        Parser::Nonterminal::statement_list_then_end,
        sem::make_statement_list(
            sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list(sem::make_identifier("x"))),
            sem::make_function_evaluation(sem::make_identifier("g"), sem::make_parameter_list(sem::make_identifier("y")))
        ),
        7
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
