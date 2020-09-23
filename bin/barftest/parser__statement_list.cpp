// 2016.08.14 - Victor Dods

#include "sem/Declaration.hpp"
#include "sem/Identifier.hpp"
#include "sem/PointerType.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "sem/TypeIdentifier.hpp"
#include "sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace cbz {
namespace text {

LVD_TEST_BEGIN(00__parser__13__statement_list__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "p : T@", "p :\nT@",
        },
        Parser::Nonterminal::statement_list,
        sem::make_statement_list(
            sem::make_declaration(
                sem::make_symbol_specifier(sem::make_identifier("p")),
                sem::make_pointer_type(FiRange::INVALID, sem::make_type_identifier(sem::make_identifier("T")))
            )
        ),
        9
    );
LVD_TEST_END

} // end namespace text
} // end namespace cbz
