// 2016.08.14 - Victor Dods

#include "barftest/sem/Assignment.hpp"
#include "barftest/sem/BinaryOperation.hpp"
#include "barftest/sem/Break.hpp"
#include "barftest/sem/Conditional.hpp"
#include "barftest/sem/Definition.hpp"
#include "barftest/sem/Function.hpp"
#include "barftest/sem/FunctionEvaluation.hpp"
#include "barftest/sem/FunctionPrototype.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/Initialization.hpp"
#include "barftest/sem/Loop.hpp"
#include "barftest/sem/Return.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"
#include "barftest/sem/Value.hpp"
#include "barftest/sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__07__statement__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x := y;", "x :=\ny;", "x := y\n", "x :=\ny\n"},
        Parser::Nonterminal::statement,
        sem::make_initialization(sem::make_identifier("x"), sem::make_identifier("y")),
        5
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x ::= y;", "x ::=\ny;", "x ::= y\n", "x ::=\ny\n"},
        Parser::Nonterminal::statement,
        sem::make_definition(sem::make_symbol_specifier(sem::make_identifier("x")), sem::make_identifier("y")),
        5
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x = y;", "x =\ny;", "x = y\n", "x =\ny\n"},
        Parser::Nonterminal::statement,
        sem::make_assignment(sem::make_identifier("x"), sem::make_identifier("y")),
        5
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"if p then { } otherwise { }", "if\np then { } otherwise { }", "if\np then\n{} otherwise { }", "if\np then\n{} otherwise\n{ }"},
        Parser::Nonterminal::statement,
        sem::make_conditional_statement(sem::make_identifier("p"), sem::make_statement_list(), sem::make_statement_list()),
        3,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"while p do { }", "while\np do\n{}"},
        Parser::Nonterminal::statement,
        sem::make_loop(sem::LoopType::WHILE_DO, sem::make_identifier("p"), sem::make_statement_list()),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__005)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"do { }while p", "do{\n}while\np"},
        Parser::Nonterminal::statement,
        sem::make_loop(sem::LoopType::DO_WHILE, sem::make_identifier("p"), sem::make_statement_list()),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__006)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"break;", "break\n"},
        Parser::Nonterminal::statement,
        sem::make_break(),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__007)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"return;", "return\n"},
        Parser::Nonterminal::statement,
        sem::make_return(),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__008)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"return 3;", "return\\\n3\n"},
        Parser::Nonterminal::statement,
        sem::make_return(sem::make_sint64_value(3)),
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__009)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"return n * factorial(n - 1)", "return n *\n factorial(\nn - 1)"},
        Parser::Nonterminal::statement,
        sem::make_return(
            sem::make_binary_operation(
                sem::BinaryOperationType::MUL,
                sem::make_identifier("n"),
                sem::make_function_evaluation(
                    sem::make_identifier("factorial"),
                    sem::make_parameter_list(
                        sem::make_binary_operation(
                            sem::BinaryOperationType::SUB,
                            sem::make_identifier("n"),
                            sem::make_sint64_value(1)
                        )
                    )
                )
            )
        ),
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__07__statement__010)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"square ::= (x:Float64) -> Float64 { return x^2\n };", "square ::=\n(x:Float64) -> Float64 { return x^2; }\n", "square ::= (x:Float64) -> Float64 {\nreturn x^2; };"},
        Parser::Nonterminal::statement,
        sem::make_definition(
            sem::make_symbol_specifier(sem::make_identifier("square")),
            sem::make_function(
                sem::make_function_prototype(
                    sem::make_declaration_tuple(
                        sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("x")), sem::make_float64())
                    ),
                    sem::make_float64()
                ),
                sem::make_statement_list(
                    sem::make_return(
                        sem::make_binary_operation(
                            sem::BinaryOperationType::POW,
                            sem::make_identifier("x"),
                            sem::make_sint64_value(2)
                        )
                    )
                )
            )
        ),
        7
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
