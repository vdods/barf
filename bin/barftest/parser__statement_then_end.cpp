// 2016.08.14 - Victor Dods

#include "sem/Conditional.hpp"
#include "sem/Identifier.hpp"
#include "sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__11__statement_then_end__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {"if p then { }", "if\np then { }", "if\np then\n{}"},
        Parser::Nonterminal::statement_then_end,
        sem::make_conditional_statement(sem::make_identifier("p"), sem::make_statement_list()),
        2
    );
LVD_TEST_END

} // end namespace text
} // end namespace cbz
