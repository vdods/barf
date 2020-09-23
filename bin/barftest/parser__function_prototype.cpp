// 2016.08.14 - Victor Dods

#include "sem/Declaration.hpp"
#include "sem/FunctionPrototype.hpp"
#include "sem/Identifier.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "sem/Value.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__08__function_prototype__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"(n:Sint64) -> Boolean", "(\nn:Sint64)->Boolean", "(n:\nSint64) -> Boolean", "(n:Sint64,) -> Boolean"},
        Parser::Nonterminal::function_prototype,
        sem::make_function_prototype(
            sem::make_declaration_tuple(sem::make_declaration(sem::make_symbol_specifier(sem::make_identifier("n")), sem::make_sint64())),
            sem::make_boolean()
        ),
        2
    );
LVD_TEST_END

} // end namespace text
} // end namespace cbz
