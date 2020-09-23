// 2016.08.14 - Victor Dods

#include "sem/Declaration.hpp"
#include "sem/FunctionType.hpp"
#include "sem/Identifier.hpp"
#include "sem/PointerType.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "sem/TypeIdentifier.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__06__declaration_statement__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x:Sint64", "x:\nSint64"},
        Parser::Nonterminal::declaration_statement,
        sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("x")), sem::make_sint64()),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__06__declaration_statement__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f:Sint64 -> Boolean", "f:\nSint64 -> Boolean"},
        Parser::Nonterminal::declaration_statement,
        sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("f")), sem::make_function_type(sem::make_type_tuple(sem::make_sint64()), sem::make_boolean())),
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__06__declaration_statement__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"f:[Sint64,Float64] -> Boolean", "f:\n[Sint64,\nFloat64] -> Boolean"},
        Parser::Nonterminal::declaration_statement,
        sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("f")), sem::make_function_type(sem::make_type_tuple(sem::make_sint64(), sem::make_float64()), sem::make_boolean())),
        3
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__06__declaration_statement__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "p : Sint64@", "p :\nSint64@",
        },
        Parser::Nonterminal::declaration_statement,
        sem::make_declaration(
            sem::make_symbol_specifier(sem::make_identifier("p")),
            sem::make_pointer_type(FiRange::INVALID, sem::make_sint64())
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__06__declaration_statement__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "p : T@", "p :\nT@",
        },
        Parser::Nonterminal::declaration_statement,
        sem::make_declaration(
            sem::make_symbol_specifier(sem::make_identifier("p")),
            sem::make_pointer_type(FiRange::INVALID, sem::make_type_identifier(sem::make_identifier("T")))
        ),
        9
    );
LVD_TEST_END

} // end namespace text
} // end namespace cbz
