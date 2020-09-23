// 2016.08.14 - Victor Dods

#include "sem/Declaration.hpp"
#include "sem/Function.hpp"
#include "sem/FunctionEvaluation.hpp"
#include "sem/FunctionPrototype.hpp"
#include "sem/Identifier.hpp"
#include "sem/Return.hpp"
#include "sem/SymbolSpecifier.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__09__function_literal__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"(n:Sint64) -> Boolean { }", "(n:Sint64)->Boolean {\n}", "(n:Sint64,)->Boolean {\n}"},
        Parser::Nonterminal::function_literal,
        sem::make_function(
            sem::make_function_prototype(
                sem::make_declaration_tuple(
                    sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("n")), sem::make_sint64())
                ),
                sem::make_boolean()
            ),
            sem::make_statement_list()
        ),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__09__function_literal__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"(n:Sint64) -> Boolean { f(x); g(y)\n }", "(n:Sint64)->Boolean { f(x)\ng(y); }"},
        Parser::Nonterminal::function_literal,
        sem::make_function(
            sem::make_function_prototype(
                sem::make_declaration_tuple(
                    sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("n")), sem::make_sint64())
                ),
                sem::make_boolean()
            ),
            sem::make_statement_list(
                sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list(sem::make_identifier("x"))),
                sem::make_function_evaluation(sem::make_identifier("g"), sem::make_parameter_list(sem::make_identifier("y")))
            )
        ),
        7
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__09__function_literal__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"() -> Sint64 { return 45954 }", "(\n) ->\nSint64 {\nreturn 45954\n}"},
        Parser::Nonterminal::function_literal,
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
} // end namespace cbz
