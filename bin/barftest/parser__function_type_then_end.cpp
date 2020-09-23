// 2016.08.14 - Victor Dods

#include "sem/FunctionType.hpp"
#include "sem/Value.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__05__function_type_then_end__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"Sint64 -> Boolean", "Sint64->Boolean", "Sint64->\nBoolean"},
        Parser::Nonterminal::function_type_then_end,
        sem::make_function_type(sem::make_type_tuple(sem::make_sint64()), sem::make_boolean()),
        3
    );
LVD_TEST_END

} // end namespace text
} // end namespace cbz
