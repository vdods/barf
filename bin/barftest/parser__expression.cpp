// 2016.08.14 - Victor Dods

#include "barftest/sem/BinaryOperation.hpp"
#include "barftest/sem/ElementAccess.hpp"
#include "barftest/sem/ErrorDummy.hpp"
#include "barftest/sem/Function.hpp"
#include "barftest/sem/FunctionEvaluation.hpp"
#include "barftest/sem/FunctionPrototype.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/LLVMCast.hpp"
#include "barftest/sem/NullPtr.hpp"
#include "barftest/sem/PointerType.hpp"
#include "barftest/sem/Return.hpp"
#include "barftest/sem/StringLiteral.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"
#include "barftest/sem/UnaryOperation.hpp"
#include "barftest/sem/Value.hpp"
#include "barftest/sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__00__expression__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"TRUE"},
        Parser::Nonterminal::expression,
        sem::make_boolean_value(true),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"FALSE"},
        Parser::Nonterminal::expression,
        sem::make_boolean_value(false),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"'x'"},
        Parser::Nonterminal::expression,
        sem::make_uint8_value(int8_t('x')),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"'\\xFF'"},
        Parser::Nonterminal::expression,
        sem::make_uint8_value(int8_t('\xFF')),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"'\\073'"},
        Parser::Nonterminal::expression,
        sem::make_uint8_value(int8_t('\073')),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__005)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"'\\0'"},
        Parser::Nonterminal::expression,
        sem::make_uint8_value(int8_t('\0')),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__006)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"\"abc\""},
        Parser::Nonterminal::expression,
        sem::make_string_literal("abc"),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__007)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"\"abc\\xFFpqr\""},
        Parser::Nonterminal::expression,
        sem::make_string_literal("abc\xFFpqr"),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__008)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"123"},
        Parser::Nonterminal::expression,
        sem::make_sint64_value(123),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__009)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"-123"},
        Parser::Nonterminal::expression,
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

LVD_TEST_BEGIN(00__parser__00__expression__010)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"1.25"},
        Parser::Nonterminal::expression,
        sem::make_float64_value(1.25),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__011)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"-1.25"},
        Parser::Nonterminal::expression,
        sem::make_unary_operation(
            FiRange::INVALID,
            sem::UnaryOperationType::NEGATE,
            sem::make_float64_value(1.25)
        ),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__012)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"1.25e10"},
        Parser::Nonterminal::expression,
        sem::make_float64_value(1.25e10),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__013)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"-1.25E10"},
        Parser::Nonterminal::expression,
        sem::make_unary_operation(
            FiRange::INVALID,
            sem::UnaryOperationType::NEGATE,
            sem::make_float64_value(1.25E10)
        ),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__014)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x"},
        Parser::Nonterminal::expression,
        sem::make_identifier("x"),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__015)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"(x)"},
        Parser::Nonterminal::expression,
        sem::make_identifier("x"),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__016)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x+y", "x\\\n+y"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(sem::BinaryOperationType::ADD, sem::make_identifier("x"), sem::make_identifier("y")),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__017)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"-x", "-\nx", "-    x"},
        Parser::Nonterminal::expression,
        sem::make_unary_operation(FiRange::INVALID, sem::UnaryOperationType::NEGATE, sem::make_identifier("x")),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__018)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f(x)", "f(\nx)", "f(\nx\n)"},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list(sem::make_identifier("x"))),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__019)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f(x,y)", "f(\nx,y)", "f(\nx,\ny)", "f(x,\ny\n)", "f(x,y,)"},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list(sem::make_identifier("x"), sem::make_identifier("y"))),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__020)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f()", "f(\n)"},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list()),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__021)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f(x,y"},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list(sem::make_identifier("x"), sem::make_identifier("y"))),
        4,
        1,
        3,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__022)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f("},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list()),
        4,
        1,
        3,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__023)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f(,"},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(sem::make_identifier("f"), sem::make_parameter_list(sem::make_error_dummy())),
        4,
        1,
        3,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__024)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"sin(x^2)", "sin(\nx^2)", "sin(\nx^\n2)", "sin(x^\n2)", "sin(x^2\n)"},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(
            sem::make_identifier("sin"),
            sem::make_parameter_list(
                sem::make_binary_operation(sem::BinaryOperationType::POW, sem::make_identifier("x"), sem::make_sint64_value(2))
            )
        ),
        6
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__025)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"exp(x- 2)", "exp(\nx- 2)", "exp(\nx-\n2)", "exp(x-\n2)", "exp(x-2)", "exp(x - 2\n)"},
        Parser::Nonterminal::expression,
        sem::make_function_evaluation(
            sem::make_identifier("exp"),
            sem::make_parameter_list(
                sem::make_binary_operation(sem::BinaryOperationType::SUB, sem::make_identifier("x"), sem::make_sint64_value(2))
            )
        ),
        6
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__026)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"3 + 4 + 5", "3+ 4+ 5", "3+4+5", "3 + 4 + 5", "3+\n4+\n5", "(3 + 4) + 5"},
        Parser::Nonterminal::expression,
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

LVD_TEST_BEGIN(00__parser__00__expression__027)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"3 - 4 - 5", "3- 4- 5", "3-4-5", "3 - 4 - 5", "3-\n4-\n5", "(3 - 4) - 5"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::SUB,
            sem::make_binary_operation(
                sem::BinaryOperationType::SUB,
                sem::make_sint64_value(3),
                sem::make_sint64_value(4)
            ),
            sem::make_sint64_value(5)
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__028)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"3*4*5", "3 * 4 * 5", "3*\n4*\n5", "(3*4)*5"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::MUL,
            sem::make_binary_operation(
                sem::BinaryOperationType::MUL,
                sem::make_sint64_value(3),
                sem::make_sint64_value(4)
            ),
            sem::make_sint64_value(5)
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__029)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"3*4 + 5", "3 * 4 + 5", "3*4+5", "3*\n4+\n5", "(3*4) + 5"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::ADD,
            sem::make_binary_operation(
                sem::BinaryOperationType::MUL,
                sem::make_sint64_value(3),
                sem::make_sint64_value(4)
            ),
            sem::make_sint64_value(5)
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__030)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x and y", "x and\ny"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::LOGICAL_AND,
            sem::make_identifier("x"),
            sem::make_identifier("y")
        ),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__031)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x and y and z", "x and\ny and z", "(x and y) and z"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::LOGICAL_AND,
            sem::make_binary_operation(
                sem::BinaryOperationType::LOGICAL_AND,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__032)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x or y and z", "x or\ny and z", "x or (y and z)"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::LOGICAL_OR,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::LOGICAL_AND,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__033)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x == y", "x ==\ny", "x==y", "x==\n     y"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::EQUALS,
            sem::make_identifier("x"),
            sem::make_identifier("y")
        ),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__034)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"(x == y) == z"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::EQUALS,
            sem::make_binary_operation(
                sem::BinaryOperationType::EQUALS,
                sem::make_identifier("x"),
                sem::make_identifier("y")
            ),
            sem::make_identifier("z")
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__035)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x < (y < z)"},
        Parser::Nonterminal::expression,
        sem::make_binary_operation(
            sem::BinaryOperationType::LESS_THAN,
            sem::make_identifier("x"),
            sem::make_binary_operation(
                sem::BinaryOperationType::LESS_THAN,
                sem::make_identifier("y"),
                sem::make_identifier("z")
            )
        ),
        4
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__036)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"arr[3]", "arr[\n3]", "arr[\n3,\n]"},
        Parser::Nonterminal::expression,
        sem::make_element_access(sem::make_identifier("arr"), sem::make_sint64_value(3)),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__037)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"[]", "[\n]"},
        Parser::Nonterminal::expression,
        sem::make_tuple(),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__038)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"[3]", "[3,]", "[\n3,\n]"},
        Parser::Nonterminal::expression,
        sem::make_tuple(sem::make_sint64_value(3)),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__039)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"[3,4]", "[3,4,]", "[\n3,\n4,\n]"},
        Parser::Nonterminal::expression,
        sem::make_tuple(sem::make_sint64_value(3), sem::make_sint64_value(4)),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__040)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"[z0[0]+z1[0]]"},
        Parser::Nonterminal::expression,
        sem::make_tuple(
            sem::make_binary_operation(
                sem::BinaryOperationType::ADD,
                sem::make_element_access(
                    sem::make_identifier("z0"),
                    sem::make_sint64_value(0)
                ),
                sem::make_element_access(
                    sem::make_identifier("z1"),
                    sem::make_sint64_value(0)
                )
            )
        ),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__041)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"() -> Sint64 { return 45954 }", "(\n) ->\nSint64 {\nreturn 45954\n}"},
        Parser::Nonterminal::expression,
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

LVD_TEST_BEGIN(00__parser__00__expression__042)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "__nullptr__{Sint64@}", "__nullptr__{\nSint64@}",
        },
        Parser::Nonterminal::expression,
        sem::make_nullptr(
            FiRange::INVALID,
            sem::make_pointer_type(
                FiRange::INVALID,
                sem::make_sint64()
            )
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__043)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "__llvm_cast_trunc__(12345, Sint8)", "__llvm_cast_trunc__(\n12345,\nSint8)",
        },
        Parser::Nonterminal::expression,
        sem::make_llvm_cast(
            FiRange::INVALID,
            sem::LLVMCastInstruction::TRUNC,
            sem::make_sint64_value(12345),
            sem::make_sint8()
        ),
        9
    );
LVD_TEST_END

//
// Error handling
//

LVD_TEST_BEGIN(00__parser__00__expression__044)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "($%)",
            "(foo+)",
        },
        Parser::Nonterminal::expression,
        sem::make_error_dummy(),
        4,
        1,
        2,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__045)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "x+($%)",
            "x+(foo+)",
        },
        Parser::Nonterminal::expression,
        sem::make_binary_operation(sem::BinaryOperationType::ADD, sem::make_identifier("x"), sem::make_error_dummy()),
        6,
        1,
        2,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__00__expression__046)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "(n:Sint64) -> Sint64 { return !@#$% }",
        },
        Parser::Nonterminal::expression,
        sem::make_function(
            sem::make_function_prototype(
                sem::make_declaration_tuple(
                    sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("n")), sem::make_sint64())
                ),
                sem::make_sint64()
            ),
            sem::make_statement_list(
                sem::make_return(sem::make_error_dummy())
            )
        ),
        7,
        1,
        2,
        Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError::EXPECTED
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
