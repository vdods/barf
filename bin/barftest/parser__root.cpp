// 2016.08.14 - Victor Dods

#include "sem/Declaration.hpp"
#include "sem/Definition.hpp"
#include "sem/Identifier.hpp"
#include "sem/Initialization.hpp"
#include "sem/PointerType.hpp"
#include "sem/ReferenceType.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "sem/TypeIdentifier.hpp"
#include "sem/UnaryOperation.hpp"
#include "sem/Value.hpp"
#include "sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__12__root__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"", " ", "\t"},
        Parser::Nonterminal::root,
        sem::make_statement_list(),
        2,
        1,
        1
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__001)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {";", "\n", ";\n", "\n;", ";\n\n", ";\n;", "\n;;", ";;;;;;;;"},
        Parser::Nonterminal::root,
        sem::make_statement_list(),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__002)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"{}", "{;}", "{\n}", "{;\n}", "{\n;}", "{;\n\n}", "{;\n;}", "{\n;;}", "{;;;;;;;;}"},
        Parser::Nonterminal::root,
        sem::make_statement_list(sem::make_statement_list()),
        2
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__003)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"x ::= 2;", "\nx ::= 2;", "\nx ::= 2\n", "x ::=\n2\n", "x ::= 2\n", "x ::= 2\n"},
        Parser::Nonterminal::root,
        sem::make_statement_list(sem::make_definition(sem::make_symbol_specifier(sem::make_identifier("x")), sem::make_sint64_value(2))),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__004)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "{ x ::= 2\n }", "\n{ x ::= 2; }", "{\nx ::= 2\n }", "{ x ::=\n2\n }", "{ x ::= 2; }",
            "{ x ::= 2; }", "\n{ x ::= 2; }", "{\nx ::= 2; }", "{ x ::=\n2; }", "{ x ::= 2;\n}", "{ x ::= 2; }",
            "{ ;x ::= 2; }", "\n{ ;x ::= 2; }", "{\n;x ::= 2; }", "{ ;x ::=\n2; }", "{ ;x ::= 2;\n}", "{ ;x ::= 2; }",
            "{ ;x ::= 2\n }", "\n{ ;x ::= 2\n }", "{\n;x ::= 2\n }", "{ ;x ::=\n2\n }", "{ ;x ::= 2\n}", "{ ;x ::= 2\n }",
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(sem::make_statement_list(sem::make_definition(sem::make_symbol_specifier(sem::make_identifier("x")), sem::make_sint64_value(2)))),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__005)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "T ::= Float64@", "T ::= Float64 @", "T ::=\nFloat64@",
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(
            sem::make_definition(
                sem::make_symbol_specifier(sem::make_identifier("T")),
                sem::make_pointer_type(FiRange::INVALID, sem::make_float64())
            )
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__006)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "T ::= Float64@@", "T ::= Float64 @ @", "T ::=\nFloat64@@",
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(
            sem::make_definition(
                sem::make_symbol_specifier(sem::make_identifier("T")),
                sem::make_pointer_type(FiRange::INVALID, sem::make_pointer_type(FiRange::INVALID, sem::make_float64()))
            )
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__007)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "T ::= Float64#", "T ::= Float64 #", "T ::=\nFloat64#",
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(
            sem::make_definition(
                sem::make_symbol_specifier(sem::make_identifier("T")),
                sem::make_reference_type(FiRange::INVALID, sem::make_float64())
            )
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__008)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "T ::= Float64##", "T ::= Float64 # #", "T ::=\nFloat64##",
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(
            sem::make_definition(
                sem::make_symbol_specifier(sem::make_identifier("T")),
                sem::make_reference_type(FiRange::INVALID, sem::make_reference_type(FiRange::INVALID, sem::make_float64()))
            )
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__009)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "p : T@", "p :\nT@",
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(
            sem::make_declaration(
                sem::make_symbol_specifier(sem::make_identifier("p")),
                sem::make_pointer_type(FiRange::INVALID, sem::make_type_identifier(sem::make_identifier("T")))
            )
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__010)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "x : Sint64\np : Sint64@;\np := x#"
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(
            sem::make_declaration(
                sem::make_symbol_specifier(sem::make_identifier("x")),
                sem::make_sint64()
            ),
            sem::make_declaration(
                sem::make_symbol_specifier(sem::make_identifier("p")),
                sem::make_pointer_type(FiRange::INVALID, sem::make_sint64())
            ),
            sem::make_initialization(
                sem::make_identifier("p"),
                sem::make_unary_operation(FiRange::INVALID, sem::UnaryOperationType::HASH_SYMBOL, sem::make_identifier("x"))
            )
        ),
        9
    );
LVD_TEST_END

LVD_TEST_BEGIN(00__parser__12__root__011)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "x : Void\ny ::= void\nz : Void@"
        },
        Parser::Nonterminal::root,
        sem::make_statement_list(
            sem::make_declaration(
                sem::make_symbol_specifier(sem::make_identifier("x")),
                sem::make_void_type()
            ),
            sem::make_definition(
                sem::make_symbol_specifier(sem::make_identifier("y")),
                sem::make_void_value()
            ),
            sem::make_declaration(
                sem::make_symbol_specifier(sem::make_identifier("z")),
                sem::make_pointer_type(FiRange::INVALID, sem::make_void_type())
            )
        ),
        9
    );
LVD_TEST_END

#if 0
LVD_TEST_BEGIN(00__parser__12__root__012)
    // Apparent bug(s) in trison npda C++ target
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"\
square_Float64   ::=\n\
    (x:Float64) -> Float64 {\n\
        return x*x\n\
    }\n\
y : Float64\n\
__init__ ::= () -> Sint64 {\n\
    if TRUE then { // Using this to start the block causes: cbz: ../cbz/text/Parser.cpp:5051: text::Parser::ParseTreeNode_* text::Parser::TakeHypotheticalActionOnHPS_(const text::Parser::ParseTreeNode_&, text::Parser::ParseTreeNode_::Type, uint32_t): Assertion <>existing_reduce_action_node->m_child_nodes.begin()->second.size() == 1' failed.\n\
        y = square_Float64(10.0)\n\
    }\n\
    return 0\n\
}\n"},
        Parser::Nonterminal::root,
        sem::make_statement_list(),
        2
    );
LVD_TEST_END
#endif

} // end namespace text
} // end namespace cbz
